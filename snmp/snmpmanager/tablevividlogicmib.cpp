/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2009 RDK Management
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
/*
 * Note: this file originally auto-generated by mib2c using
 *  : mib2c.table_data.conf 15999 2007-03-25 22:32:02Z dts12 $
 */

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "vl_ocStbHost_GetData.h"
#include "tablevividlogicmib.h"
#include "SnmpIORM.h"
/** Initializes the vividlogicmib module */
void
init_Certinfomib(void)
{
    /*
     * here we initialize all the tables we're planning on supporting 
     */
    initialize_table_Ieee1394certificationTable();
    initialize_table_McardcertificationTable();
}

//# Determine the first/last column names

/** Initialize the Ieee1394certificationTable table by defining its contents and how it's structured */
void
initialize_table_Ieee1394certificationTable(void)
{
    static oid      Ieee1394certificationTable_oid[] =
        { 1, 3, 6, 1, 4, 1, 31800, 3, 1, 1 };
    size_t          Ieee1394certificationTable_oid_len =
        OID_LENGTH(Ieee1394certificationTable_oid);
    netsnmp_handler_registration *reg;
    netsnmp_tdata  *table_data;
    netsnmp_table_registration_info *table_info;
    netsnmp_cache  *cache;

    reg =
        netsnmp_create_handler_registration("Ieee1394certificationTable",
                                            Ieee1394certificationTable_handler,
                                            Ieee1394certificationTable_oid,
                                            Ieee1394certificationTable_oid_len,
                                            HANDLER_CAN_RONLY);

    table_data =
        netsnmp_tdata_create_table("Ieee1394certificationTable", 0);
    table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info);
    netsnmp_table_helper_add_indexes(table_info, ASN_UNSIGNED,  /* index: VividlogicCEFInterfaceIndex */
                                     0);

    table_info->min_column = COLUMN_VIVIDLOGICSZDISPSTRING;
    table_info->max_column = COLUMN_VIVIDLOGICBBISPRODUCTION;

    netsnmp_tdata_register(reg, table_data, table_info);
    cache = netsnmp_cache_create(IEEE1394CERTIFICATIONTABLE_TIMEOUT,
                                 Ieee1394certificationTable_load,
                                 Ieee1394certificationTable_free,
                                 Ieee1394certificationTable_oid,
                                 Ieee1394certificationTable_oid_len);
        cache->magic = (void *) table_data;
    netsnmp_inject_handler_before(reg, netsnmp_cache_handler_get(cache),
                                  "Ieee1394certificationTable");

        /*
         * Initialise the contents of the table here 
         */
         if(0 == Ieee1394certificationTable_load(cache, table_data))
	{
			//vl_ocStbHostAVInterfaceTable_getdata"  
	  SNMP_DEBUGPRINT(" ERROR:: Not ye Ieee1394certificationTable_load table initialise here \n");
	}
}

/*
 * Example cache handling - set up table_data list from a suitable file 
 */
int
    Ieee1394certificationTable_load(netsnmp_cache * cache, void *vmagic) {
    netsnmp_tdata * table = (netsnmp_tdata *) vmagic;
    // if( 0 ==  vl_Ieee1394certification_getdata(table))
    // {
     //    SNMP_DEBUGPRINT(" ERROR:: Not ye Ieee1394certificationTable_load table initialise here \n");
    // }
     return 1;
}

void Ieee1394certificationTable_free(netsnmp_cache * cache, void *vmagic) {
//     netsnmp_tdata * table = (netsnmp_tdata *) vmagic;
//     netsnmp_tdata_row *this;
// 
//     while           ((this = netsnmp_tdata_get_first_row(table))) {
//         netsnmp_tdata_remove_and_delete_row(table, this);
// }
}
/** handles requests for the Ieee1394certificationTable table */ 
int
   Ieee1394certificationTable_handler(netsnmp_mib_handler *handler,
                                       netsnmp_handler_registration
                                       *reginfo,
                                       netsnmp_agent_request_info *reqinfo,
                                       netsnmp_request_info *requests) {

    netsnmp_request_info *request;
    netsnmp_table_request_info *table_info;
    netsnmp_tdata * table_data;
    netsnmp_tdata_row *table_row;
    struct Ieee1394certificationTable_entry *table_entry;
    int             ret;

    switch (reqinfo->mode) {
        /*
         * Read-support (also covers GetNext requests)
         */
    case MODE_GET:
        for (request = requests; request; request = request->next) {
            table_entry = (struct Ieee1394certificationTable_entry *)
                netsnmp_tdata_extract_entry(request);
            table_info = netsnmp_extract_table_info(request);

            switch (table_info->colnum) {
            case COLUMN_VIVIDLOGICSZDISPSTRING:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                         (u_char *) table_entry->
                                         VividlogicSzDispString,
                                         table_entry->
                                         VividlogicSzDispString_len);
                break;
            case COLUMN_VIVIDLOGICSZKEYINFOSTRING:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                         (u_char *) table_entry->
                                         VividlogicSzKeyInfoString,
                                         table_entry->
                                         VividlogicSzKeyInfoString_len);
                break;
            case COLUMN_VIVIDLOGICSZMFRNAME:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                         (u_char *) table_entry->
                                         VividlogicSzMfrName,
                                         table_entry->
                                         VividlogicSzMfrName_len);
                break;
            case COLUMN_VIVIDLOGICSZMODELNAME:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                         (u_char *) table_entry->
                                         VividlogicSzModelName,
                                         table_entry->
                                         VividlogicSzModelName_len);
                break;
            case COLUMN_VIVIDLOGICACMODELID:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                         (u_char *) table_entry->
                                         VividlogicAcModelId,
                                         table_entry->
                                         VividlogicAcModelId_len);
                break;
            case COLUMN_VIVIDLOGICAACVENDORID:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                         (u_char *) table_entry->
                                         VividlogicAacVendorId,
                                         table_entry->
                                         VividlogicAacVendorId_len);
                break;
            case COLUMN_VIVIDLOGICAACDEVICEID:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                         (u_char *) table_entry->
                                         VividlogicAacDeviceId,
                                         table_entry->
                                         VividlogicAacDeviceId_len);
                break;
            case COLUMN_VIVIDLOGICAACGUIDORMACADDR:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                         (u_char *) table_entry->
                                         VividlogicAacGUIDorMACADDR,
                                         table_entry->
                                         VividlogicAacGUIDorMACADDR_len);
                break;
            case COLUMN_VIVIDLOGICBCERTAVAILABLE:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->
                                           VividlogicBCertAvailable);
                break;
            case COLUMN_VIVIDLOGICBBCERTVALID:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->
                                           VividlogicBbCertValid);
                break;
            case COLUMN_VIVIDLOGICBBISPRODUCTION:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->
                                           VividlogicBbIsProduction);
                break;
            default:
                netsnmp_set_request_error(reqinfo, request,
                                          SNMP_NOSUCHOBJECT);
                break;
            }
        }
        break;

    }
    return SNMP_ERR_NOERROR;
}

//# Determine the first/last column names

/** Initialize the McardcertificationTable table by defining its contents and how it's structured */
void
                initialize_table_McardcertificationTable(void) {
    static oid      McardcertificationTable_oid[] = {
    1, 3, 6, 1, 4, 1, 31800, 4, 1, 1};
    size_t          McardcertificationTable_oid_len =
        OID_LENGTH(McardcertificationTable_oid);
    netsnmp_handler_registration *reg;
    netsnmp_tdata * table_data;
    netsnmp_table_registration_info *table_info;
    netsnmp_cache * cache;

    reg =
        netsnmp_create_handler_registration("McardcertificationTable",
                                            McardcertificationTable_handler,
                                            McardcertificationTable_oid,
                                            McardcertificationTable_oid_len,
                                            HANDLER_CAN_RONLY);

    table_data = netsnmp_tdata_create_table("McardcertificationTable", 0);
    table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info);
    netsnmp_table_helper_add_indexes(table_info, ASN_UNSIGNED,  /* index: VividlogicMcardInterfaceIndex */
                                     0);

    table_info->min_column = COLUMN_VIVIDLOGICSZDISPSTRING;
    table_info->max_column = COLUMN_VIVIDLOGICMCARDBISPRODUCTION;

    netsnmp_tdata_register(reg, table_data, table_info);
    cache = netsnmp_cache_create(MCARDCERTIFICATIONTABLE_TIMEOUT,
                                 McardcertificationTable_load,
                                 McardcertificationTable_free,
                                 McardcertificationTable_oid,
                                 McardcertificationTable_oid_len);
        cache->magic = (void *) table_data;
        cache->flags = NETSNMP_CACHE_DONT_INVALIDATE_ON_SET |
                NETSNMP_CACHE_DONT_FREE_BEFORE_LOAD |
                NETSNMP_CACHE_DONT_FREE_EXPIRED |
                    NETSNMP_CACHE_DONT_AUTO_RELEASE;
    netsnmp_inject_handler_before(reg, netsnmp_cache_handler_get(cache),
                                  "McardcertificationTable");
        /*
         * Initialise the contents of the table here
         */
         if(0 == McardcertificationTable_load(cache, table_data))
	{
			//vl_ocStbHostAVInterfaceTable_getdata"
    
	  SNMP_DEBUGPRINT(" ERROR:: Not ye Ieee1394certificationTable_load table initialise here \n");
	}
}

/*
 * Example cache handling - set up table_data list from a suitable file
 */
int McardcertificationTable_load(netsnmp_cache * cache,
                                      void *vmagic) {

     netsnmp_tdata * table_data = (netsnmp_tdata *) vmagic;
     netsnmp_tdata_row *row;


     SNMP_DEBUGPRINT("..........VividlogiccertificationTable....Start .table initialise here ............\n");
    if( 0 ==  vl_VividlogiccertificationTable_getdata(table_data))
     {
         SNMP_DEBUGPRINT(" ERROR:: Not ye VividlogiccertificationTable table initialise here \n");
             return 0;
     }
    SNMP_DEBUGPRINT("........End of ..VividlogiccertificationTable....initialise ...........\n");
    
     return 1;

}

void  McardcertificationTable_free(netsnmp_cache * cache, void *vmagic) {
//     netsnmp_tdata * table = (netsnmp_tdata *) vmagic;
//     netsnmp_tdata_row *this;
// 
//     while ((this = netsnmp_tdata_get_first_row(table))) {
//         netsnmp_tdata_remove_and_delete_row(table, this);
//     }
}

/** handles requests for the McardcertificationTable table */
int McardcertificationTable_handler(netsnmp_mib_handler *handler,
                                    netsnmp_handler_registration *reginfo,
                                    netsnmp_agent_request_info *reqinfo,
                                    netsnmp_request_info *requests) {

  
        netsnmp_request_info *request;
        netsnmp_table_request_info *table_info;
        netsnmp_tdata * table_data;
        netsnmp_tdata_row *table_row;
        netsnmp_tdata * vl_table_data;
        netsnmp_tdata_row *vl_row;
   

    struct McardcertificationTable_entry *table_entry;
    int             ret;

    switch (reqinfo->mode) {
        /*
         * Read-support (also covers GetNext requests)
         */
    case MODE_GET:
        for (request = requests; request; request = request->next) {
            //table_entry = (struct McardcertificationTable_entry *)
             //   netsnmp_tdata_extract_entry(request);
            //table_info = netsnmp_extract_table_info(request);
            VL_SNMP_PREPARE_AND_CHECK_TABLE_GET_REQUEST(McardcertificationTable_entry);

    
            switch (table_info->colnum) {
            case COLUMN_VIVIDLOGICSZDISPSTRING:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                         (u_char *) table_entry->
                                         VividlogicSzDispString,
                                         table_entry->
                                         VividlogicSzDispString_len);
                break;
            case COLUMN_VIVIDLOGICMCARDSZDEVICECERTSUBJECTNAME:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                         (u_char *) table_entry->
                                         VividlogicMcardSzDeviceCertSubjectName,
                                         table_entry->
                                         VividlogicMcardSzDeviceCertSubjectName_len);
                break;
            case COLUMN_VIVIDLOGICMCARDSZDEVICECERTISSUERNAME:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                         (u_char *) table_entry->
                                         VividlogicMcardSzDeviceCertIssuerName,
                                         table_entry->
                                         VividlogicMcardSzDeviceCertIssuerName_len);
                break;
            case COLUMN_VIVIDLOGICMCARDSZMANCERTSUBJECTNAME:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                         (u_char *) table_entry->
                                         VividlogicMcardSzManCertSubjectName,
                                         table_entry->
                                         VividlogicMcardSzManCertSubjectName_len);
                break;
            case COLUMN_VIVIDLOGICMCARDSZMANCERTISSUERNAME:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                         (u_char *) table_entry->
                                         VividlogicMcardSzManCertIssuerName,
                                         table_entry->
                                         VividlogicMcardSzManCertIssuerName_len);
                break;
            case COLUMN_VIVIDLOGICMCARDACHOSTID:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                         (u_char *) table_entry->
                                         VividlogicMcardAcHostId,
                                         table_entry->
                                         VividlogicMcardAcHostId_len);
                break;
            case COLUMN_VIVIDLOGICMCARDBCERTAVAILABLE:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                           (u_char *)table_entry->
                                           VividlogicMcardbCertAvailable,sizeof(table_entry->
                                           VividlogicMcardbCertAvailable));
                break;
            case COLUMN_VIVIDLOGICMCARDBCERTVALID:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb,ASN_OCTET_STR,
                                           (u_char *)table_entry->VividlogicMcardbCertValid,sizeof(table_entry->VividlogicMcardbCertValid));
                break;
            case COLUMN_VIVIDLOGICMCARDBVERIFIEDWITHCHAIN:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                           (u_char *)table_entry->
                                           VividlogicMcardbVerifiedWithChain,sizeof(table_entry->
                                           VividlogicMcardbVerifiedWithChain));
                break;
            case COLUMN_VIVIDLOGICMCARDBISPRODUCTION:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                           (u_char *)table_entry->
                                           VividlogicMcardbIsProduction,sizeof(table_entry->
                                           VividlogicMcardbIsProduction));
                break;
            default:
                netsnmp_set_request_error(reqinfo, request,
                                          SNMP_NOSUCHOBJECT);
                break;
            }
        }
        break;

    }
    return SNMP_ERR_NOERROR;
}
