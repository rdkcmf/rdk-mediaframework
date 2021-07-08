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

/**All type of decleration will be in ocStbHostMibModule.h*/
#include "ocStbHostMibModule.h"
#include "vl_ocStbHost_GetData.h"
//#include "ocStbHostInBandTunerTable.h"
#include "rdk_debug.h"

/** Initialize the ocStbHostInBandTunerTable table by defining its contents and how it's structured */
void
initialize_table_ocStbHostInBandTunerTable(void)
{
    static oid      ocStbHostInBandTunerTable_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 7, 1 };
    size_t          ocStbHostInBandTunerTable_oid_len =
        OID_LENGTH(ocStbHostInBandTunerTable_oid);
    netsnmp_handler_registration *reg;
    netsnmp_tdata  *table_data;
    netsnmp_table_registration_info *table_info;
    netsnmp_cache  *cache;

    reg =
        netsnmp_create_handler_registration("ocStbHostInBandTunerTable",
                                            ocStbHostInBandTunerTable_handler,
                                            ocStbHostInBandTunerTable_oid,
                                            ocStbHostInBandTunerTable_oid_len,
                                            HANDLER_CAN_RONLY);

    table_data =
        netsnmp_tdata_create_table("ocStbHostInBandTunerTable", 0);
    table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info);
    netsnmp_table_helper_add_indexes(table_info, ASN_UNSIGNED,  /* index: ocStbHostAVInterfaceIndex */
                                     0);

    table_info->min_column = COLUMN_OCSTBHOSTINBANDTUNERMODULATIONMODE;
    table_info->max_column = COLUMN_OCSTBHOSTINBANDTUNERDACID;

    netsnmp_tdata_register(reg, table_data, table_info);
    cache = netsnmp_cache_create(OCSTBHOSTINBANDTUNERTABLE_TIMEOUT,
                                 ocStbHostInBandTunerTable_load,
                                 ocStbHostInBandTunerTable_free,
                                 ocStbHostInBandTunerTable_oid,
                                 ocStbHostInBandTunerTable_oid_len);
        cache->magic = (void *) table_data;

    cache->flags = NETSNMP_CACHE_DONT_INVALIDATE_ON_SET |
     NETSNMP_CACHE_DONT_FREE_BEFORE_LOAD | NETSNMP_CACHE_DONT_FREE_EXPIRED |
     NETSNMP_CACHE_DONT_AUTO_RELEASE;

       netsnmp_inject_handler_before(reg,netsnmp_cache_handler_get(cache), "ocStbHostInBandTunerTable");
        /*
         * Initialise the contents of the table here
        */

        if(0 == ocStbHostInBandTunerTable_load(cache,table_data))
    {
        //vl_ocStbHostAVInterfaceTable_getdata"
      SNMP_DEBUGPRINT(" ERROR:: Not yet table initialise here \n");
    }

}

/*
 * create a new row in the table
 */
netsnmp_tdata_row *ocStbHostInBandTunerTable_createEntry(netsnmp_tdata *
                                                         table_data,
                                                         u_long
                                                         ocStbHostAVInterfaceIndex)
{
    struct ocStbHostInBandTunerTable_entry *entry;
    netsnmp_tdata_row *row;

    entry = SNMP_MALLOC_TYPEDEF(struct ocStbHostInBandTunerTable_entry);
    if (!entry)
        return NULL;

    row = netsnmp_tdata_create_row();
    if (!row) {
        SNMP_FREE(entry);
        return NULL;
    }
    row->data = entry;
    entry->ocStbHostAVInterfaceIndex = ocStbHostAVInterfaceIndex;
    netsnmp_tdata_row_add_index(row, ASN_UNSIGNED,
                                &(entry->ocStbHostAVInterfaceIndex),
                                sizeof(entry->ocStbHostAVInterfaceIndex));
    netsnmp_tdata_add_row(table_data, row);
    return row;
}

/*
 * remove a row from the table
 */
void
ocStbHostInBandTunerTable_removeEntry(netsnmp_tdata * table_data,
                                          netsnmp_tdata_row * row) {
    struct ocStbHostInBandTunerTable_entry *entry;

    if (!row)
        return;                 /* Nothing to remove */
    entry = (struct ocStbHostInBandTunerTable_entry *)
        netsnmp_tdata_remove_and_delete_row(table_data, row);
    if (entry)
        SNMP_FREE(entry);       /* XXX - release any other internal resources */
}

/*
 * Example cache handling - set up table_data list from a suitable file
 */
int
ocStbHostInBandTunerTable_load(netsnmp_cache * cache, void *vmagic) {


     netsnmp_tdata * table_data = (netsnmp_tdata *) vmagic;
    netsnmp_tdata_row *row;



       SNMP_DEBUGPRINT("..........ocStbHostInBandTunerTable_load.....Start .table initialise here ............\n");

     // ocStbHostInBandTunerTable_free(cache,table_data);

     if(0 == vl_ocStbHostInBandTunerTable_getdata(table_data))
    {
            //vl_ocStbHostAVInterfaceTable_getdata"
      SNMP_DEBUGPRINT(" ERROR:: Not yet table initialise here \n");
    }

       SNMP_DEBUGPRINT(".......End...ocStbHostInBandTunerTable_load........\n");
    return 1;

}

void
ocStbHostInBandTunerTable_free(netsnmp_cache * cache, void *vmagic){
    netsnmp_tdata * table = (netsnmp_tdata *) vmagic;
    netsnmp_tdata_row *temprow;
 //    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "\nFree the initialised Table here \n ");
    // netsnmp_tdata_delete_table(table);
//      while((temprow = netsnmp_tdata_row_first(table))) {
//          netsnmp_tdata_remove_and_delete_row(table, temprow);
//                }
}

/** handles requests for the ocStbHostInBandTunerTable table */

int
ocStbHostInBandTunerTable_handler(netsnmp_mib_handler *handler,
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
    struct ocStbHostInBandTunerTable_entry *table_entry;
    int             ret;

    vl_row = NULL;
    switch (reqinfo->mode) {
        /*
         * Read-support (also covers GetNext requests)
         */
    case MODE_GET:

        for (request = requests; request; request = request->next)
        {
            /*changed due to wrong values in retriving first element of the Table*/
            //table_entry = (struct ocStbHostInBandTunerTable_entry *)              netsnmp_tdata_extract_entry(request);
            //row3 =  netsnmp_tdata_extract_row(request); -Not working proprely
            /*changed due to wrong values in retriving first element of the Table*/
            VL_SNMP_PREPARE_AND_CHECK_TABLE_GET_REQUEST(ocStbHostInBandTunerTable_entry);

            switch (table_info->colnum){
                case COLUMN_OCSTBHOSTINBANDTUNERMODULATIONMODE:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request,
                                                SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    SNMP_DEBUGPRINT("\nocStbHostInBandTunerTable_handler::TunerModulationMode::: %d\n",                table_entry->ocStbHostInBandTunerModulationMode);
                    snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,                                           table_entry->ocStbHostInBandTunerModulationMode);
                    break;
                case COLUMN_OCSTBHOSTINBANDTUNERFREQUENCY:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request,
                                                SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    SNMP_DEBUGPRINT("\nocStbHostInBandTunerTable_handler::ocStbHostInBandTunerFrequency::: %d\n",                table_entry->ocStbHostInBandTunerFrequency);
                    snmp_set_var_typed_integer(request->requestvb,
                                            ASN_UNSIGNED,
                                            table_entry->
                                            ocStbHostInBandTunerFrequency);
                    break;
                case COLUMN_OCSTBHOSTINBANDTUNERINTERLEAVER:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request,
                                                SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                            table_entry->
                                            ocStbHostInBandTunerInterleaver);
                    break;
                case COLUMN_OCSTBHOSTINBANDTUNERPOWER:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request,
                                                SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                            table_entry->
                                            ocStbHostInBandTunerPower);
                    break;
                case COLUMN_OCSTBHOSTINBANDTUNERAGCVALUE:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request,
                                                SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_integer(request->requestvb,
                                            ASN_UNSIGNED,
                                            table_entry->
                                            ocStbHostInBandTunerAGCValue);
                    break;
                case COLUMN_OCSTBHOSTINBANDTUNERSNRVALUE:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request,
                                                SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                            table_entry->
                                            ocStbHostInBandTunerSNRValue);
                    break;
                case COLUMN_OCSTBHOSTINBANDTUNERUNERROREDS:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request,
                                                SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_integer(request->requestvb, ASN_COUNTER,
                                            table_entry->
                                            ocStbHostInBandTunerUnerroreds);
                    break;
                case COLUMN_OCSTBHOSTINBANDTUNERCORRECTEDS:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request,
                                                SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_integer(request->requestvb, ASN_COUNTER,
                                            table_entry->
                                            ocStbHostInBandTunerCorrecteds);
                    break;
                case COLUMN_OCSTBHOSTINBANDTUNERUNCORRECTABLES:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request,
                                                SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_integer(request->requestvb, ASN_COUNTER,
                                            table_entry->
                                            ocStbHostInBandTunerUncorrectables);
                    break;
                case COLUMN_OCSTBHOSTINBANDTUNERCARRIERLOCKLOST:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request,
                                                SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_integer(request->requestvb, ASN_COUNTER,
                                            table_entry->
                                            ocStbHostInBandTunerCarrierLockLost);
                    break;
                case COLUMN_OCSTBHOSTINBANDTUNERPCRERRORS:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request,
                                                SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_integer(request->requestvb, ASN_COUNTER,
                                            table_entry->
                                            ocStbHostInBandTunerPCRErrors);
                    break;
                case COLUMN_OCSTBHOSTINBANDTUNERPTSERRORS:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request,
                                                SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_integer(request->requestvb, ASN_COUNTER,
                                            table_entry->
                                            ocStbHostInBandTunerPTSErrors);
                    break;
                case COLUMN_OCSTBHOSTINBANDTUNERSTATE:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request,
                                                SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                            table_entry->
                                            ocStbHostInBandTunerState);
                    break;
                case COLUMN_OCSTBHOSTINBANDTUNERBER:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request,
                                                SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                            table_entry->
                                            ocStbHostInBandTunerBER);
                    break;
                case COLUMN_OCSTBHOSTINBANDTUNERSECSSINCELOCK:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request,
                                                SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_integer(request->requestvb,
                                            ASN_UNSIGNED,
                                            table_entry->
                                            ocStbHostInBandTunerSecsSinceLock);
                    break;
                case COLUMN_OCSTBHOSTINBANDTUNEREQGAIN:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request,
                                                SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                            table_entry->
                                            ocStbHostInBandTunerEqGain);
                    break;
                case COLUMN_OCSTBHOSTINBANDTUNERMAINTAPCOEFF:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request,
                                                SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                            table_entry->
                                            ocStbHostInBandTunerMainTapCoeff);
                    break;
                case COLUMN_OCSTBHOSTINBANDTUNERTOTALTUNECOUNT:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request,
                                                SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_integer(request->requestvb, ASN_COUNTER,
                                            table_entry->
                                            ocStbHostInBandTunerTotalTuneCount);
                    break;
                case COLUMN_OCSTBHOSTINBANDTUNERTUNEFAILURECOUNT:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request,
                                                SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_integer(request->requestvb, ASN_COUNTER,
                                            table_entry->
                                            ocStbHostInBandTunerTuneFailureCount);
                    break;
                case COLUMN_OCSTBHOSTINBANDTUNERTUNEFAILFREQ:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request,
                                                SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_integer(request->requestvb,
                                            ASN_UNSIGNED,
                                            table_entry->
                                            ocStbHostInBandTunerTuneFailFreq);
                    break;
                case COLUMN_OCSTBHOSTINBANDTUNERBANDWIDTH:
                  if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                      continue;
                    }
                   snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->
                                           ocStbHostInBandTunerBandwidth);
                break;
                case COLUMN_OCSTBHOSTINBANDTUNERCHANNELMAPID:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request,
                                                SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_integer(request->requestvb,ASN_UNSIGNED,table_entry->ocStbHostInBandTunerChannelMapId);
		break;
                case COLUMN_OCSTBHOSTINBANDTUNERDACID:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request,
                                                SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_integer(request->requestvb,ASN_UNSIGNED,table_entry->ocStbHostInBandTunerDacId);

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