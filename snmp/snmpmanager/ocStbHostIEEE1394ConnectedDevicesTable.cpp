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


/*
 * Note: this file originally auto-generated by mib2c using
 *  : mib2c.table_data.conf 15999 2007-03-25 22:32:02Z dts12 $
 */

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "ocStbHostMibModule.h"
#include "vl_ocStbHost_GetData.h"
#include "rdk_debug.h"
/** Initialize the ocStbHostIEEE1394ConnectedDevicesTable table by defining its contents and how it's structured */
void
initialize_table_ocStbHostIEEE1394ConnectedDevicesTable(void)
{
    static oid      ocStbHostIEEE1394ConnectedDevicesTable_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 3, 2 };
    size_t          ocStbHostIEEE1394ConnectedDevicesTable_oid_len =
        OID_LENGTH(ocStbHostIEEE1394ConnectedDevicesTable_oid);
    netsnmp_handler_registration *reg;
    netsnmp_tdata  *table_data;
    netsnmp_table_registration_info *table_info;
    netsnmp_cache  *cache;

    reg =
        netsnmp_create_handler_registration
        ("ocStbHostIEEE1394ConnectedDevicesTable",
         ocStbHostIEEE1394ConnectedDevicesTable_handler,
         ocStbHostIEEE1394ConnectedDevicesTable_oid,
         ocStbHostIEEE1394ConnectedDevicesTable_oid_len,
         HANDLER_CAN_RONLY);

    table_data =
        netsnmp_tdata_create_table
        ("ocStbHostIEEE1394ConnectedDevicesTable", 0);
    table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info);
    netsnmp_table_helper_add_indexes(table_info, ASN_UNSIGNED,  /* index: ocStbHostIEEE1394ConnectedDevicesIndex */
                                     0);

    table_info->min_column =
        COLUMN_OCSTBHOSTIEEE1394CONNECTEDDEVICESAVINTERFACEINDEX;
    table_info->max_column =
        COLUMN_OCSTBHOSTIEEE1394CONNECTEDDEVICESADSOURCESELECTSUPPORT;

    netsnmp_tdata_register(reg, table_data, table_info);
    cache =
        netsnmp_cache_create
        (OCSTBHOSTIEEE1394CONNECTEDDEVICESTABLE_TIMEOUT,
         ocStbHostIEEE1394ConnectedDevicesTable_load,
         ocStbHostIEEE1394ConnectedDevicesTable_free,
         ocStbHostIEEE1394ConnectedDevicesTable_oid,
         ocStbHostIEEE1394ConnectedDevicesTable_oid_len);
    cache->magic = (void *) table_data;
    netsnmp_inject_handler_before(reg, netsnmp_cache_handler_get(cache),
                                  "ocStbHostIEEE1394ConnectedDevicesTable");

        /*
         * Initialise the contents of the table here
         */
   if(0 == ocStbHostIEEE1394ConnectedDevicesTable_load(cache, table_data))
    {
            //vl_ocStbHostAVInterfaceTable_getdata"
      SNMP_DEBUGPRINT(" ERROR:: Not ye ocStbHostIEEE1394Table_load table initialise here \n");
    }
}

/*
 * Example cache handling - set up table_data list from a suitable file
 */
int
ocStbHostIEEE1394ConnectedDevicesTable_load(netsnmp_cache * cache, void *vmagic)
{
     netsnmp_tdata * table_data = (netsnmp_tdata *) vmagic;
     netsnmp_tdata_row *row;
     SNMP_DEBUGPRINT("...............Start ocStbHostIEEE1394ConnectedDevicesTable_load ............\n");
    if(0 == vl_ocStbHostIEEE1394ConnectedDevicesTable_getdata(table_data))
    {
      //vl_ocStbHostAVInterfaceTable_getdata"
      SNMP_DEBUGPRINT(" ERROR:: Not yet table initialise here \n");
    }

       SNMP_DEBUGPRINT("................End ocStbHostIEEE1394ConnectedDevicesTable_load ............\n");

    return 1;
}

void
ocStbHostIEEE1394ConnectedDevicesTable_free(netsnmp_cache * cache,
                                                void *vmagic)
{
    netsnmp_tdata * table = (netsnmp_tdata *) vmagic;
/*    netsnmp_tdata_row *this;

    while           ((this = netsnmp_tdata_get_first_row(table))) {
        netsnmp_tdata_remove_and_delete_row(table, this);
}*/

}
/** handles requests for the ocStbHostIEEE1394ConnectedDevicesTable table */


int
ocStbHostIEEE1394ConnectedDevicesTable_handler(netsnmp_mib_handler
                                                   *handler,
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
    struct ocStbHostIEEE1394ConnectedDevicesTable_entry *table_entry;
    int             ret;

    vl_row = NULL;
    switch (reqinfo->mode) {
        /*
         * Read-support (also covers GetNext requests)
         */
    case MODE_GET:
        for (request = requests; request; request = request->next) {
           // table_entry = (struct ocStbHostIEEE1394ConnectedDevicesTable_entry *)              netsnmp_tdata_extract_entry(request);
            /*changed due to wrong values in retriving first element of the Table*/
            VL_SNMP_PREPARE_AND_CHECK_TABLE_GET_REQUEST(ocStbHostIEEE1394ConnectedDevicesTable_entry);


            switch (table_info->colnum) {
            case COLUMN_OCSTBHOSTIEEE1394CONNECTEDDEVICESAVINTERFACEINDEX:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb,
                                           ASN_UNSIGNED,
                                           table_entry->
                                           ocStbHostIEEE1394ConnectedDevicesAVInterfaceIndex);
                break;
            case COLUMN_OCSTBHOSTIEEE1394CONNECTEDDEVICESSUBUNITTYPE:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->
                                           ocStbHostIEEE1394ConnectedDevicesSubUnitType);
                break;
            case COLUMN_OCSTBHOSTIEEE1394CONNECTEDDEVICESEUI64:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                         (u_char *) table_entry->
                                         ocStbHostIEEE1394ConnectedDevicesEui64,
                                         table_entry->
                                         ocStbHostIEEE1394ConnectedDevicesEui64_len);
                break;
            case COLUMN_OCSTBHOSTIEEE1394CONNECTEDDEVICESADSOURCESELECTSUPPORT:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->
                                           ocStbHostIEEE1394ConnectedDevicesADSourceSelectSupport);
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
