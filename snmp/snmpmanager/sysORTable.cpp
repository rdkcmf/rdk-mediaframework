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
#include "sysORTable.h"
#include "vl_ocStbHost_GetData.h"

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

#define MAX_sysORTABLE 10
/** Initializes the sysORTable module */
void
init_sysORTable(void)
{
    /*
     * here we initialize all the tables we're planning on supporting
     */
    initialize_table_sysORTable();
}

extern u_long  gsysORLastTimeTick;
/** Initialize the sysORTable table by defining its contents and how it's structured */
void
initialize_table_sysORTable(void)
{
    static oid      sysORTable_oid[] = { 1, 3, 6, 1, 2, 1, 1, 9 };
    size_t          sysORTable_oid_len = OID_LENGTH(sysORTable_oid);
    netsnmp_handler_registration *reg;
    netsnmp_tdata  *table_data;
    netsnmp_table_registration_info *table_info;
    netsnmp_cache  *cache;

    reg =
        netsnmp_create_handler_registration("sysORTable",
                                            sysORTable_handler,
                                            sysORTable_oid,
                                            sysORTable_oid_len,
                                            HANDLER_CAN_RONLY);

    table_data = netsnmp_tdata_create_table("sysORTable", 0);
    table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info);
    netsnmp_table_helper_add_indexes(table_info, ASN_INTEGER,   /* index: sysORIndex */
                                     0);

    table_info->min_column = COLUMN_SYSORID;
    table_info->max_column = COLUMN_SYSORUPTIME;

    netsnmp_tdata_register(reg, table_data, table_info);
    cache = netsnmp_cache_create(SYSORTABLE_TIMEOUT,
                                 sysORTable_load, sysORTable_free,
                                 sysORTable_oid, sysORTable_oid_len);
     cache->magic = (void *) table_data;
     cache->flags = NETSNMP_CACHE_DONT_INVALIDATE_ON_SET |
                NETSNMP_CACHE_DONT_FREE_BEFORE_LOAD | NETSNMP_CACHE_DONT_FREE_EXPIRED |
                NETSNMP_CACHE_DONT_AUTO_RELEASE;
    netsnmp_inject_handler_before(reg, netsnmp_cache_handler_get(cache),
                                  "sysORTable");

        /*
         * Initialise the contents of the table here
         */

    if(0 == sysORTable_load(cache,table_data))
    {
        //snmpTargetParamsTable_load"
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP", " ERROR:: hrSWRunTable_load Not yet table initialise here \n");
    }
}
/*
 * Example cache handling - set up table_data list from a suitable file
 */
#ifdef __cplusplus
extern "C" {
#endif
u_long  gsysORUpTime_OcSTB_hostMib;
u_long  gsysORUpTime_DocsDevieMib;
u_long  gsysORUpTime_HostResourcesMib;
u_long  gsysORUpTime_ipNetToPhysicalTableMib;
u_long  gsysORUpTime_ifTableMib;
u_long  gsysORUpTime_SNMPv2Mib;
#ifdef __cplusplus
}
#endif
int
   sysORTable_load(netsnmp_cache * cache, void *vmagic) {

    netsnmp_tdata * table_data = (netsnmp_tdata *) vmagic;
    netsnmp_tdata_row *row;

    SNMP_DEBUGPRINT("\n sysORTable_load :: Start  \n");
    bool replace_table = false;
    int count = 0;
    if(netsnmp_tdata_row_first(table_data))
    {
        replace_table = true;
    }
    struct timeval  LastOR_uptime;
    gsysORLastTimeTick = get_uptime();

    SNMP_DEBUGPRINT("%d ---------gsysORLastTimeTick ---------::::: \n",gsysORLastTimeTick);
    /** Data dec .considering all MIB supported by the Sub_Agent **/

    struct sysORTable_entry  *sysORE;
    rmf_osal_memAllocP(RMF_OSAL_MEM_POD, MAX_sysORTABLE* sizeof(sysORTable_entry), (void **)&sysORE);//taken as 10

    //OC-STB-HOST-MIB 1.3.6.1.4.1.4491.2.3.1.1
    apioid OcSTB_hostMib[10] =
    { 1,3,6,1,4,1,4491,2,3,1};
    sysORE[0].sysORIndex = 1;
    sysORE[0].sysORID_len = sizeof(OcSTB_hostMib)/sizeof(apioid);
    memcpy(sysORE[0].sysORID, OcSTB_hostMib,
                     sysORE[0].sysORID_len * sizeof(oid));
    sysORE[0].sysORDescr_len= strlen("OC-STB-HOST-MIB module contains the management objects for the  management of OpenCable Set-top Host Device");
    memcpy(sysORE[0].sysORDescr,"OC-STB-HOST-MIB module contains the management objects for the  management of OpenCable Set-top Host Device",  sysORE[0].sysORDescr_len);
     sysORE[0].sysORUpTime = gsysORUpTime_OcSTB_hostMib;


     apioid DocsDevie_Mib[7] =
     { 1,3,6,1,2,1,69};
     sysORE[1].sysORIndex = 2;
     sysORE[1].sysORID_len = sizeof(DocsDevie_Mib)/sizeof(apioid);
     memcpy(sysORE[1].sysORID, DocsDevie_Mib,
              sysORE[1].sysORID_len * sizeof(oid));
     sysORE[1].sysORDescr_len= strlen("DOCS-CABLE-DEVICE-MIB Module for MCNS-compliant cable modems and cable-modem termination systems");
     memcpy(sysORE[1].sysORDescr,"DOCS-CABLE-DEVICE-MIB Module for MCNS-compliant cable modems and cable-modem termination systems", sysORE[1].sysORDescr_len);
     sysORE[1].sysORUpTime = gsysORUpTime_DocsDevieMib;


     apioid HostResources_Mib[7] =
     {1,3,6,1,2,1,25};
     sysORE[2].sysORIndex = 3;
     sysORE[2].sysORID_len = sizeof(HostResources_Mib)/sizeof(apioid);
     memcpy(sysORE[2].sysORID, HostResources_Mib,
              sysORE[2].sysORID_len * sizeof(oid));
     sysORE[2].sysORDescr_len= strlen("HOST-RESOURCES-MIB is for use in managing host systems");
     memcpy(sysORE[2].sysORDescr,"HOST-RESOURCES-MIB is for use in managing host systems", sysORE[2].sysORDescr_len);
     sysORE[2].sysORUpTime = gsysORUpTime_HostResourcesMib;


     apioid IP_Mib[7] =
     {1,3,6,1,2,1,4};
     sysORE[3].sysORIndex = 4;
     sysORE[3].sysORID_len = sizeof(IP_Mib)/sizeof(apioid);
     memcpy(sysORE[3].sysORID, IP_Mib,
              sysORE[3].sysORID_len * sizeof(oid));
     sysORE[3].sysORDescr_len= strlen("IP-MIB module for managing IP and ICMP implementations- ipNetToPhysicalTable");
     memcpy(sysORE[3].sysORDescr,"IP-MIB module for managing IP and ICMP implementations- ipNetToPhysicalTable", sysORE[3].sysORDescr_len);
     sysORE[3].sysORUpTime = gsysORUpTime_ipNetToPhysicalTableMib;

     apioid Iftable_Mib[8] =
     {1,3,6,1,2,1,2,2};
     sysORE[4].sysORIndex = 5;
     sysORE[4].sysORID_len = sizeof(Iftable_Mib)/sizeof(apioid);
     memcpy(sysORE[4].sysORID, Iftable_Mib,
              sysORE[4].sysORID_len * sizeof(oid));
     sysORE[4].sysORDescr_len= strlen("IF-MIB module to describe generic objects for network   interface sub-layers");
     memcpy(sysORE[4].sysORDescr,"IF-MIB module to describe generic objects for network  interface sub-layers", sysORE[4].sysORDescr_len);
     sysORE[4].sysORUpTime = gsysORUpTime_ifTableMib;

     apioid SNMPv2_Mib[7] =
     {1,3,6,1,6,3,1};
     sysORE[5].sysORIndex = 6;
     sysORE[5].sysORID_len = sizeof(SNMPv2_Mib)/sizeof(apioid);
     memcpy(sysORE[5].sysORID, SNMPv2_Mib,
              sysORE[5].sysORID_len * sizeof(oid));
     sysORE[5].sysORDescr_len= strlen("SNMPv2-MIB module for SNMP entities.");
     memcpy(sysORE[5].sysORDescr,"SNMPv2-MIB module for SNMP entities.", sysORE[5].sysORDescr_len);
     sysORE[5].sysORUpTime = gsysORUpTime_SNMPv2Mib;
    //SNMPv2-MIB::sysORID.7 = OID: SNMP-VIEW-BASED-ACM-MIB::vacmBasicGroup

    /** May be these values will change , with my understanding I am fillng with the Subagent supproting MIB items....**/
     int Maxsize =6; //CAN BE changed later
    for( int nsizelist = 0; nsizelist < Maxsize; nsizelist++)
    {
        if(replace_table)
        {
            replace_table = false;
            Table_free(table_data);

        }
        sysORTable_createEntry_allData(table_data, &sysORE[nsizelist]);
    }
    rmf_osal_memFreeP(RMF_OSAL_MEM_POD, sysORE); //free the memory 

#if 0
    if(Maxsize == 0)
    {
        Table_free(table_data);
        struct sysORTable_entry  sysORE;
        memset(&sysORE, 0, sizeof(struct sysORTable_entry));
        sysORTable_createEntry_allData(table_data, &sysORE);
    }
#endif    
    SNMP_DEBUGPRINT("\n sysORTable_load:: End   :::  \n");
    return 1;
   }

int sysORTable_createEntry_allData(netsnmp_tdata * table_data, struct sysORTable_entry *sysORE)
{
       SNMP_DEBUGPRINT("\n sysORTable_createEntry_allData ::Start \n");
       struct sysORTable_entry *entry;
       netsnmp_tdata_row *row;
       entry = SNMP_MALLOC_TYPEDEF(struct sysORTable_entry);
       if (!entry)
           return 0;

       row = netsnmp_tdata_create_row();
       if (!row) {
           SNMP_FREE(entry);
           return 0;
       }
       row->data = entry;
    /*
       * Index values
    */
       entry->sysORIndex = sysORE->sysORIndex;
       entry->sysORID_len = sysORE->sysORID_len;
       memcpy(entry->sysORID, sysORE->sysORID,
                entry->sysORID_len * sizeof(oid));
       entry->sysORDescr_len= sysORE->sysORDescr_len;
       memcpy(entry->sysORDescr, sysORE->sysORDescr ,  entry->sysORDescr_len);
       entry->sysORUpTime = sysORE->sysORUpTime;
       netsnmp_tdata_row_add_index(row, ASN_UNSIGNED,
                                   &(entry->sysORIndex),
                                   sizeof(entry->sysORIndex));
       netsnmp_tdata_add_row(table_data, row);

       SNMP_DEBUGPRINT("\n sysORTable_createEntry_allData ::END \n");
       return 1;
}


void
  sysORTable_free(netsnmp_cache * cache, void *vmagic) {
    netsnmp_tdata * table = (netsnmp_tdata *) vmagic;
/*    netsnmp_tdata_row *this;

    while           ((this = netsnmp_tdata_get_first_row(table))) {
        netsnmp_tdata_remove_and_delete_row(table, this);
}*/
}
/** handles requests for the sysORTable table */
int
  sysORTable_handler(netsnmp_mib_handler *handler,
                       netsnmp_handler_registration *reginfo,
                       netsnmp_agent_request_info *reqinfo,
                       netsnmp_request_info *requests) {

    netsnmp_request_info *request;
    netsnmp_table_request_info *table_info;
    netsnmp_tdata * table_data;
    netsnmp_tdata_row *table_row;
    netsnmp_tdata * vl_table_data; //add
    netsnmp_tdata_row *vl_row;  //add
    vl_row = NULL;
    struct sysORTable_entry *table_entry;
    int             ret;

    switch (reqinfo->mode) {
        /*
         * Read-support (also covers GetNext requests)
         */
    case MODE_GET:
        for (request = requests; request; request = request->next) {
            //table_entry = (struct sysORTable_entry *)               netsnmp_tdata_extract_entry(request);
            /*changed due to wrong values in retriving first element of the Table*/
            VL_SNMP_PREPARE_AND_CHECK_TABLE_GET_REQUEST(sysORTable_entry);

            switch (table_info->colnum) {
            case COLUMN_SYSORID:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OBJECT_ID,
                                         (u_char *) table_entry->sysORID,
                                         table_entry->sysORID_len * sizeof(oid));
                break;
            case COLUMN_SYSORDESCR:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                         (u_char *) table_entry->
                                         sysORDescr,
                                         table_entry->sysORDescr_len);
                break;
            case COLUMN_SYSORUPTIME:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb,
                                           ASN_TIMETICKS,
                                           table_entry->sysORUpTime);
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
