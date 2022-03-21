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

#include "ipcclient_impl.h"
#include "ocStbHostMibModule.h"
#include "vl_ocStbHost_GetData.h"
#include "rdk_debug.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/** Initialize the ocStbHostCCAppInfoTable table by defining its contents and how it's structured */
void
initialize_table_ocStbHostCCAppInfoTable(void)
{
    static oid      ocStbHostCCAppInfoTable_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 4, 5, 1, 1 };
    size_t          ocStbHostCCAppInfoTable_oid_len =
        OID_LENGTH(ocStbHostCCAppInfoTable_oid);
    netsnmp_handler_registration *reg;
    netsnmp_tdata  *table_data;
    netsnmp_table_registration_info *table_info;
    netsnmp_cache  *cache;

    reg =
        netsnmp_create_handler_registration("ocStbHostCCAppInfoTable",
                                            ocStbHostCCAppInfoTable_handler,
                                            ocStbHostCCAppInfoTable_oid,
                                            ocStbHostCCAppInfoTable_oid_len,
                                            HANDLER_CAN_RONLY);

    table_data = netsnmp_tdata_create_table("ocStbHostCCAppInfoTable", 0);
    table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info);
    netsnmp_table_helper_add_indexes(table_info, ASN_UNSIGNED,  /* index: ocStbHostCCApplicationType */
                                     0);

    table_info->min_column = COLUMN_OCSTBHOSTCCAPPINFOINDEX;
    table_info->max_column = COLUMN_OCSTBHOSTCCAPPINFOPAGE;

    netsnmp_tdata_register(reg, table_data, table_info);
    cache = netsnmp_cache_create(OCSTBHOSTCCAPPINFOTABLE_TIMEOUT,
                                 ocStbHostCCAppInfoTable_load,
                                 ocStbHostCCAppInfoTable_free,
                                 ocStbHostCCAppInfoTable_oid,
                                 ocStbHostCCAppInfoTable_oid_len);
        cache->magic = (void *) table_data;
    netsnmp_inject_handler_before(reg, netsnmp_cache_handler_get(cache),
                                 "ocStbHostCCAppInfoTable");

         /*
         * Initialise the contents of the table here
         */
         if(0 == ocStbHostCCAppInfoTable_load(cache, table_data))
    {
            //ocStbHostCCAppInfoTable_load"
      SNMP_DEBUGPRINT(" ERROR:: Not ye ocStbHostCCAppInfoTable_load table initialise here \n");
    }
    return;//Mamidi:042209
}

/*
 * Example cache handling - set up table_data list from a suitable file
 */
int
ocStbHostCCAppInfoTable_load(netsnmp_cache * cache, void *vmagic) {

    netsnmp_tdata * table_data = (netsnmp_tdata *) vmagic;
    netsnmp_tdata_row *row;

     SNMP_DEBUGPRINT("...............Start ocStbHostCCAppInfoTable_load ............\n");
    if(0 == vl_ocStbHostCCAppInfoTable_load_getdata(table_data))
    {
      //vl_ocStbHostAVInterfaceTable_getdata"
      SNMP_DEBUGPRINT(" ERROR:: Not yet table initialise here \n");
        return 0;//Mamidi:042209
    }

       SNMP_DEBUGPRINT("................End ocStbHostCCAppInfoTable_load \n");
        return 1;//Mamidi:042209
}

void
ocStbHostCCAppInfoTable_free(netsnmp_cache * cache, void *vmagic) {
//     netsnmp_tdata * table = (netsnmp_tdata *) vmagic;
//     netsnmp_tdata_row *this;
//
//     while           ((this = netsnmp_tdata_get_first_row(table))) {
//         netsnmp_tdata_remove_and_delete_row(table, this);}
        return ;//Mamidi:042209

}

extern "C" int vlGetCcAppInfoPage(int iIndex, const char ** ppszPage, int * pnBytes);

/** handles requests for the ocStbHostCCAppInfoTable table */
int
 ocStbHostCCAppInfoTable_handler(netsnmp_mib_handler *handler,
                                    netsnmp_handler_registration *reginfo,
                                    netsnmp_agent_request_info *reqinfo,
                                    netsnmp_request_info *requests) {

    netsnmp_request_info *request;
    netsnmp_table_request_info *table_info;
    netsnmp_tdata * table_data;
    netsnmp_tdata_row *table_row;
    netsnmp_tdata * vl_table_data;
    netsnmp_tdata_row *vl_row;
    struct ocStbHostCCAppInfoTable_entry *table_entry;
    int             ret;

    vl_row = NULL;
    switch (reqinfo->mode) {
        /*
         * Read-support (also covers GetNext requests)
         */
    case MODE_GET:
        for (request = requests; request; request = request->next) {
        //table_entry = (struct ocStbHostCCAppInfoTable_entry *)netsnmp_tdata_extract_entry(request);
            /*changed due to wrong values in retriving first element of the Table*/
            VL_SNMP_PREPARE_AND_CHECK_TABLE_GET_REQUEST(ocStbHostCCAppInfoTable_entry);

            switch (table_info->colnum) {
            case COLUMN_OCSTBHOSTCCAPPINFOINDEX:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb,
                                           ASN_UNSIGNED,
                                           table_entry->
                                           ocStbHostCCAppInfoIndex);
                break;
            case COLUMN_OCSTBHOSTCCAPPLICATIONTYPE:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb,
                                           ASN_UNSIGNED,
                                           table_entry->
                                           ocStbHostCCApplicationType);
                break;
            case COLUMN_OCSTBHOSTCCAPPLICATIONNAME:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                         (u_char *) table_entry->
                                         ocStbHostCCApplicationName,
                                         table_entry->
                                         ocStbHostCCApplicationName_len);
                break;
            case COLUMN_OCSTBHOSTCCAPPLICATIONVERSION:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb,
                                           ASN_UNSIGNED,
                                           table_entry->
                                           ocStbHostCCApplicationVersion);
                break;
            case COLUMN_OCSTBHOSTCCAPPINFOPAGE:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                else
                {
                    const char * pszPage = NULL; int nBytes = SNMP_MAX_MSG_SIZE;
                    IPC_CLIENT_vlGetCcAppInfoPage(table_entry->ocStbHostCCAppInfoIndex, &pszPage, &nBytes);
                    if(NULL != pszPage)
                    {
                        struct stat st;
                        memset(&st, 0, sizeof(st));
                        if(0 == stat(pszPage, &st))
                        {
                            FILE * fp = fopen(pszPage, "r");
                            if(NULL != fp)
                            {
                                char * pszPageData = NULL;
                                rmf_Error ret = rmf_osal_memAllocPGen(RMF_OSAL_MEM_POD, st.st_size, (void**)(&pszPageData));
                                if((RMF_SUCCESS == ret) && (NULL != pszPageData))
                                {
                                    int nRead = fread(pszPageData, 1, st.st_size, fp);
                                    if(nRead > 0)
                                    {
                                        snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                                                 (u_char *) pszPageData, (size_t)nRead);
                                        rmf_osal_memFreeP( RMF_OSAL_MEM_POD, (void**)pszPageData );
                                        pszPageData = NULL;
                                    }
                                    else
                                    {
                                        const char szTransferFailure[]="File read failure.";
                                        snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                                                 (u_char *) szTransferFailure, (size_t)strlen(szTransferFailure));
                                    }
                                }
                                else
                                {
                                    const char szTransferFailure[]="Memory allocation failure.";
                                    snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                                             (u_char *) szTransferFailure, (size_t)strlen(szTransferFailure));
                                }
                                if(NULL != pszPageData)
                                {
                                   rmf_osal_memFreeP( RMF_OSAL_MEM_POD, (void**)pszPageData );
                                   pszPageData = NULL;
                                }
                                fclose(fp);
                            }
                            else
                            {
                                const char szTransferFailure[]="File not found.";
                                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                                         (u_char *) szTransferFailure, (size_t)strlen(szTransferFailure));
                            }
                        }
                        else
                        {
                            const char szTransferFailure[]="stat() failed.";
                            snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                                     (u_char *) szTransferFailure, (size_t)strlen(szTransferFailure));
                        }
                        rmf_osal_memFreeP( RMF_OSAL_MEM_POD, (void**)pszPage );
                    }
                    else
                    {
                        const char szTransferFailure[]="IPC_CLIENT_vlGetCcAppInfoPage() returned NULL pszPage.";
                        snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                                 (u_char *) szTransferFailure, (size_t)strlen(szTransferFailure));
                    }
                }
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
