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

/** Initialize the ocStbHostVc1ContentTable table by defining its contents and how it's structured */
void
initialize_table_ocStbHostVc1ContentTable(void)
{
    static oid      ocStbHostVc1ContentTable_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 7, 6 };
    size_t          ocStbHostVc1ContentTable_oid_len =
        OID_LENGTH(ocStbHostVc1ContentTable_oid);
    netsnmp_handler_registration *reg;
    netsnmp_tdata  *table_data;
    netsnmp_table_registration_info *table_info;
    netsnmp_cache  *cache;

    reg =
        netsnmp_create_handler_registration("ocStbHostVc1ContentTable",
                                            ocStbHostVc1ContentTable_handler,
                                            ocStbHostVc1ContentTable_oid,
                                            ocStbHostVc1ContentTable_oid_len,
                                            HANDLER_CAN_RONLY);

    table_data =
        netsnmp_tdata_create_table("ocStbHostVc1ContentTable", 0);
    table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info);
    netsnmp_table_helper_add_indexes(table_info, ASN_UNSIGNED,  /* index: ocStbHostVc1ContentIndex */
                                     0);

    table_info->min_column = COLUMN_OCSTBHOSTVC1CONTENTPROGRAMNUMBER;
    table_info->max_column = COLUMN_OCSTBHOSTVC1CONTAINERFORMAT;

    netsnmp_tdata_register(reg, table_data, table_info);
    cache = netsnmp_cache_create(OCSTBHOSTVC1CONTENTTABLE_TIMEOUT,
                                 ocStbHostVc1ContentTable_load,
                                 ocStbHostVc1ContentTable_free,
                                 ocStbHostVc1ContentTable_oid,
                                 ocStbHostVc1ContentTable_oid_len);
        cache->magic = (void *) table_data;
    netsnmp_inject_handler_before(reg, netsnmp_cache_handler_get(cache),
                                  "ocStbHostVc1ContentTable");

        /*
         * Initialise the contents of the table here
         */
    if(0 == ocStbHostVc1ContentTable_load(cache,table_data))
    {
            //vl_ocStbHostAVInterfaceTable_getdata"
      RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP", " ERROR:: Not ye ocStbHostVc1ContentTable_load table initialise here \n");
    }
    return ;//Mamidi:042209

}

/*
 * Example cache handling - set up table_data list from a suitable file
 */
int
    ocStbHostVc1ContentTable_load(netsnmp_cache * cache, void *vmagic) {
    netsnmp_tdata * table_data = (netsnmp_tdata *) vmagic;
    netsnmp_tdata_row *row;

    if(0 == vl_ocStbHostVc1ContentTable_getdata(table_data))
    {
      //vl_ocStbHostAVInterfaceTable_getdata"
      RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP", " ERROR:: Not yet table initialise here \n");
      return 0;//Mamidi:042209
    }

    return 1;//Mamidi:042209

}

void
   ocStbHostVc1ContentTable_free(netsnmp_cache * cache, void *vmagic)
{
    netsnmp_tdata * table = (netsnmp_tdata *) vmagic;
//    netsnmp_tdata_row *this;

   // netsnmp_tdata_delete_table(table);
    return;
}
/** handles requests for the ocStbHostVc1ContentTable table */
int
ocStbHostVc1ContentTable_handler(netsnmp_mib_handler *handler,
                                       netsnmp_handler_registration
                                       *reginfo,
                                       netsnmp_agent_request_info *reqinfo,
                                       netsnmp_request_info *requests) {

    netsnmp_request_info *request;
    netsnmp_table_request_info *table_info;
    netsnmp_tdata * table_data;
    netsnmp_tdata_row *table_row;
    netsnmp_tdata * vl_table_data;
    netsnmp_tdata_row *vl_row;
    struct ocStbHostVc1ContentTable_entry *table_entry;
    int             ret;

    vl_row = NULL;
    switch (reqinfo->mode) {
        /*
         * Read-support (also covers GetNext requests)
         */
    case MODE_GET:
        for (request = requests; request; request = request->next) {
            /*table_entry = (struct ocStbHostVc1ContentTable_entry *)                netsnmp_tdata_extract_entry(request);
                table_info = netsnmp_extract_table_info(request);*/
            /*changed due to wrong values in retriving first element of the Table*/
            VL_SNMP_PREPARE_AND_CHECK_TABLE_GET_REQUEST(ocStbHostVc1ContentTable_entry);

            switch (table_info->colnum) {
            case COLUMN_OCSTBHOSTVC1CONTENTPROGRAMNUMBER:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb,
                                           ASN_UNSIGNED,
                                           table_entry->
                                           ocStbHostVc1ContentProgramNumber);
                break;
/*                
            case COLUMN_OCSTBHOSTVC1CONTENTTRANSPORTSTREAMID:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb,
                                           ASN_UNSIGNED,
                                           table_entry->
                                           ocStbHostVc1ContentTransportStreamID);
                break;
            case COLUMN_OCSTBHOSTVC1CONTENTTOTALSTREAMS:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb,
                                           ASN_UNSIGNED,
                                           table_entry->
                                           ocStbHostVc1ContentTotalStreams);
                break;
            case COLUMN_OCSTBHOSTVC1CONTENTSELECTEDVIDEOPID:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->
                                           ocStbHostVc1ContentSelectedVideoPID);
                break;
            case COLUMN_OCSTBHOSTVC1CONTENTSELECTEDAUDIOPID:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->
                                           ocStbHostVc1ContentSelectedAudioPID);
                break;
            case COLUMN_OCSTBHOSTVC1CONTENTOTHERAUDIOPIDS:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->
                                           ocStbHostVc1ContentOtherAudioPIDs);
                break;
            case COLUMN_OCSTBHOSTVC1CONTENTCCIVALUE:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->
                                           ocStbHostVc1ContentCCIValue);
                break;
            case COLUMN_OCSTBHOSTVC1CONTENTAPSVALUE:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->
                                           ocStbHostVc1ContentAPSValue);
                break;
            case COLUMN_OCSTBHOSTVC1CONTENTCITSTATUS:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->
                                           ocStbHostVc1ContentCITStatus);
                break;
            case COLUMN_OCSTBHOSTVC1CONTENTBROADCASTFLAGSTATUS:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->
                                           ocStbHostVc1ContentBroadcastFlagStatus);
                break;
            case COLUMN_OCSTBHOSTVC1CONTENTEPNSTATUS:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->
                                           ocStbHostVc1ContentEPNStatus);
                break;
            case COLUMN_OCSTBHOSTVC1CONTENTPCRPID:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->
                                           ocStbHostVc1ContentPCRPID);
                break;
            case COLUMN_OCSTBHOSTVC1CONTENTPCRLOCKSTATUS:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->
                                           ocStbHostVc1ContentPCRLockStatus);
                break;
            case COLUMN_OCSTBHOSTVC1CONTENTDECODERPTS:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->
                                           ocStbHostVc1ContentDecoderPTS);
                break;
            case COLUMN_OCSTBHOSTVC1CONTENTDISCONTINUITIES:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_COUNTER,
                                           table_entry->
                                           ocStbHostVc1ContentDiscontinuities);
                break;
            case COLUMN_OCSTBHOSTVC1CONTENTPKTERRORS:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_COUNTER,
                                           table_entry->
                                           ocStbHostVc1ContentPktErrors);
                break;
            case COLUMN_OCSTBHOSTVC1CONTENTPIPELINEERRORS:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_COUNTER,
                                           table_entry->
                                           ocStbHostVc1ContentPipelineErrors);
                break;
            case COLUMN_OCSTBHOSTVC1CONTENTDECODERRESTARTS:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_COUNTER,
                                           table_entry->
                                           ocStbHostVc1ContentDecoderRestarts);
                break;
*/ 
            case COLUMN_OCSTBHOSTVC1CONTAINERFORMAT:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->
                                           ocStbHostVc1ContainerFormat);
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

