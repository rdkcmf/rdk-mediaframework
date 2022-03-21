/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2011 RDK Management
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



#include "ocStbHostMibModule.h"
#include "vl_ocStbHost_GetData.h"

/** Initialize the ocStbHostSystemMemoryReportTable table by defining its contents and how it's structured */
void
initialize_table_ocStbHostSystemMemoryReportTable(void)
{
    static oid      ocStbHostSystemMemoryReportTable_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 3, 3 };
    size_t          ocStbHostSystemMemoryReportTable_oid_len =
        OID_LENGTH(ocStbHostSystemMemoryReportTable_oid);
    netsnmp_handler_registration *reg;
    netsnmp_tdata  *table_data;
    netsnmp_table_registration_info *table_info;
    netsnmp_cache  *cache;

    reg =
        netsnmp_create_handler_registration
        ("ocStbHostSystemMemoryReportTable",
         ocStbHostSystemMemoryReportTable_handler,
         ocStbHostSystemMemoryReportTable_oid,
         ocStbHostSystemMemoryReportTable_oid_len, HANDLER_CAN_RONLY);

    table_data =
        netsnmp_tdata_create_table("ocStbHostSystemMemoryReportTable", 0);
    table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info);
    netsnmp_table_helper_add_indexes(table_info, ASN_UNSIGNED,  /* index: ocStbHostSystemMemoryReportIndex */
                                     0);

    table_info->min_column = COLUMN_OCSTBHOSTSYSTEMMEMORYREPORTMEMORYTYPE;
    table_info->max_column = COLUMN_OCSTBHOSTSYSTEMMEMORYREPORTMEMORYSIZE;

    netsnmp_tdata_register(reg, table_data, table_info);
    cache = netsnmp_cache_create(OCSTBHOSTSYSTEMMEMORYREPORTTABLE_TIMEOUT,
                                 ocStbHostSystemMemoryReportTable_load,
                                 ocStbHostSystemMemoryReportTable_free,
                                 ocStbHostSystemMemoryReportTable_oid,
                                 ocStbHostSystemMemoryReportTable_oid_len);
        cache->magic = (void *) table_data;
    netsnmp_inject_handler_before(reg, netsnmp_cache_handler_get(cache),
                                  "ocStbHostSystemMemoryReportTable");
        /*
         * Initialise the contents of the table here
         */
         if(0 == ocStbHostSystemMemoryReportTable_load(cache, table_data))
    {
    //ocStbHostSystemMemoryReportTable_load"
      SNMP_DEBUGPRINT(" ERROR:: Not ye ocStbHostSystemMemoryReportTable_load table initialise here \n");
    }
}

//     /*
//      * Typical data structure for a row entry
//      */
// struct ocStbHostSystemMemoryReportTable_entry {
//     /*
//      * Index values
//      */
//     u_long          ocStbHostSystemMemoryReportIndex;
//
//     /*
//      * Column values
//      */
//     long            ocStbHostSystemMemoryReportMemoryType;
//     long            ocStbHostSystemMemoryReportMemorySize;
//
//     int             valid;
// };
//
// /*
//  * create a new row in the table
//  */
// netsnmp_tdata_row
//     *ocStbHostSystemMemoryReportTable_createEntry(netsnmp_tdata *
//                                                   table_data,
//                                                   u_long
//                                                   ocStbHostSystemMemoryReportIndex)
// {
//     struct ocStbHostSystemMemoryReportTable_entry *entry;
//     netsnmp_tdata_row *row;
//
//     entry =
//         SNMP_MALLOC_TYPEDEF(struct ocStbHostSystemMemoryReportTable_entry);
//     if (!entry)
//         return NULL;
//
//     row = netsnmp_tdata_create_row();
//     if (!row) {
//         SNMP_FREE(entry);
//         return NULL;
//     }
//     row->data = entry;
//     entry->ocStbHostSystemMemoryReportIndex =
//         ocStbHostSystemMemoryReportIndex;
//     netsnmp_tdata_row_add_index(row, ASN_UNSIGNED,
//                                 &(entry->ocStbHostSystemMemoryReportIndex),
//                                 sizeof(entry->
//                                        ocStbHostSystemMemoryReportIndex));
//     netsnmp_tdata_add_row(table_data, row);
//     return row;
// }
//
// /*
//  * remove a row from the table
//  */
// void
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//     ocStbHostSystemMemoryReportTable_removeEntry(netsnmp_tdata *
//                                                  table_data,
//                                                  netsnmp_tdata_row * row) {
//     struct ocStbHostSystemMemoryReportTable_entry *entry;
//
//     if (!row)
//         return;                 /* Nothing to remove */
//     entry = (struct ocStbHostSystemMemoryReportTable_entry *)
//         netsnmp_tdata_remove_and_delete_row(table_data, row);
//     if (entry)
//         SNMP_FREE(entry);       /* XXX - release any other internal resources */
// }

/*
 * Example cache handling - set up table_data list from a suitable file
 */
int
  ocStbHostSystemMemoryReportTable_load(netsnmp_cache * cache,
                                          void *vmagic) {
    netsnmp_tdata * table_data = (netsnmp_tdata *) vmagic;
    netsnmp_tdata_row *row;

     SNMP_DEBUGPRINT("...............Start  ocStbHostSystemMemoryReportTable_load ............\n");
   if(0 == vl_ocStbHostSystemMemoryReportTable_getdata(table_data))
      {

      SNMP_DEBUGPRINT(" ERROR:: Not yet table initialise here \n");
      }

       SNMP_DEBUGPRINT("................End  ocStbHostSystemMemoryReportTable_load ............\n");
       return 1;
                                          }

void
    ocStbHostSystemMemoryReportTable_free(netsnmp_cache * cache,
                                          void *vmagic) {
//     netsnmp_tdata * table = (netsnmp_tdata *) vmagic;
//     netsnmp_tdata_row *this;
//
//     while           ((this = netsnmp_tdata_get_first_row(table))) {
//         netsnmp_tdata_remove_and_delete_row(table, this);
// }
}
/** handles requests for the ocStbHostSystemMemoryReportTable table */
int
 ocStbHostSystemMemoryReportTable_handler(netsnmp_mib_handler *handler,
                                             netsnmp_handler_registration
                                             *reginfo,
                                             netsnmp_agent_request_info
                                             *reqinfo,
                                             netsnmp_request_info
                                             *requests) {

    netsnmp_request_info *request;
    netsnmp_table_request_info *table_info;
    netsnmp_tdata * table_data;
    netsnmp_tdata_row *table_row;
    netsnmp_tdata * vl_table_data;
    netsnmp_tdata_row *vl_row;
    struct ocStbHostSystemMemoryReportTable_entry *table_entry;
    int             ret;
    vl_row = NULL;

    switch (reqinfo->mode) {
        /*
         * Read-support (also covers GetNext requests)
         */
    case MODE_GET:
        for (request = requests; request; request = request->next) {
//             table_entry = (struct ocStbHostSystemMemoryReportTable_entry *)                netsnmp_tdata_extract_entry(request);
            /*changed due to wrong values in retriving first element of the Table*/
            VL_SNMP_PREPARE_AND_CHECK_TABLE_GET_REQUEST(ocStbHostSystemMemoryReportTable_entry);

            switch (table_info->colnum) {
            case COLUMN_OCSTBHOSTSYSTEMMEMORYREPORTMEMORYTYPE:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->
                                           ocStbHostSystemMemoryReportMemoryType);
                break;
            case COLUMN_OCSTBHOSTSYSTEMMEMORYREPORTMEMORYSIZE:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->
                                           ocStbHostSystemMemoryReportMemorySize);
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
