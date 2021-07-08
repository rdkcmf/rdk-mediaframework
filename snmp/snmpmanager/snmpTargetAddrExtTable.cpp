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
#include "snmpTargetAddrExtTable.h"
#include "snmpAccessInclude.h"
#include "vl_ocStbHost_GetData.h"
#include "utilityMacros.h"
#include "vlMutex.h"
#include "Tlvevent.h"
#include "cardManagerIf.h"

static vlMutex & vlg_TlvEventDblock = TlvConfig::vlGetTlvEventDbLock();
#ifdef AUTO_LOCKING
static void auto_lock(rmf_osal_Mutex *mutex)
{

               if(!mutex) 
			   	RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","\n\n\n\n %s: Mutex is NULL \n\n\n", __FUNCTION__);
		 rmf_osal_mutexAcquire(*mutex);
}

static void auto_unlock(rmf_osal_Mutex *mutex)
{
         if(mutex)
		 rmf_osal_mutexRelease(*mutex);
}
#endif
/** Initializes the snmpTargetAddrExtTable module */
void
init_snmpTargetAddrExtTable(void)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    /*
     * here we initialize all the tables we're planning on supporting
     */
    initialize_table_snmpTargetAddrExtTable();
  //auto_unlock(&vlg_TlvEventDblock);
}


/** Initialize the snmpTargetAddrExtTable table by defining its contents and how it's structured */
void
initialize_table_snmpTargetAddrExtTable(void)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    static oid      snmpTargetAddrExtTable_oid[] =
        { 1, 3, 6, 1, 6, 3, 18, 1, 2 };
    size_t          snmpTargetAddrExtTable_oid_len =
        OID_LENGTH(snmpTargetAddrExtTable_oid);
    netsnmp_handler_registration *reg;
    netsnmp_tdata  *table_data;
    netsnmp_table_registration_info *table_info;
    netsnmp_cache  *cache;

    reg =
        netsnmp_create_handler_registration("snmpTargetAddrExtTable",
                                            snmpTargetAddrExtTable_handler,
                                            snmpTargetAddrExtTable_oid,
                                            snmpTargetAddrExtTable_oid_len,
                                            HANDLER_CAN_RWRITE);

    table_data = netsnmp_tdata_create_table("snmpTargetAddrExtTable", 0);
    table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info);
    netsnmp_table_helper_add_indexes(table_info, ASN_PRIV_IMPLIED_OCTET_STR/*ASN_OCTET_STR*/, /* index: snmpTargetAddrName */
                                     0);

    table_info->min_column = COLUMN_SNMPTARGETADDRTMASK;
    table_info->max_column = COLUMN_SNMPTARGETADDRMMS;

    netsnmp_tdata_register(reg, table_data, table_info);
    cache = netsnmp_cache_create(SNMPTARGETADDREXTTABLE_TIMEOUT,
                                 snmpTargetAddrExtTable_load,
                                 snmpTargetAddrExtTable_free,
                                 snmpTargetAddrExtTable_oid,
                                 snmpTargetAddrExtTable_oid_len);
        cache->magic = (void *) table_data;
    cache->flags = NETSNMP_CACHE_DONT_INVALIDATE_ON_SET |
    NETSNMP_CACHE_DONT_FREE_BEFORE_LOAD | NETSNMP_CACHE_DONT_FREE_EXPIRED |
    NETSNMP_CACHE_DONT_AUTO_RELEASE;
    netsnmp_inject_handler_before(reg, netsnmp_cache_handler_get(cache),
                                  "snmpTargetAddrExtTable");

          if(0 == snmpTargetAddrExtTable_load(cache,table_data))
    {
            //snmpTargetAddrExtTable_load"
      SNMP_DEBUGPRINT(" ERROR:: snmpTargetAddrExtTable_load Not yet table initialise here \n");
    }
      //auto_unlock(&vlg_TlvEventDblock);
}

/*
 * create a new row in the table
 */
netsnmp_tdata_row *snmpTargetAddrExtTable_createEntry(netsnmp_tdata *
                                                      table_data,
                                                      char
                                                      *snmpTargetAddrName,
                                                      size_t
                                                      snmpTargetAddrName_len)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    struct snmpTargetAddrExtTable_entry *entry;
    netsnmp_tdata_row *row;
          entry = SNMP_MALLOC_TYPEDEF(struct snmpTargetAddrExtTable_entry);
    if(!entry){
	 //auto_unlock(&vlg_TlvEventDblock);
        return NULL;
    	}

                    row = netsnmp_tdata_create_row();
    if(!row) {
        SNMP_FREE(entry);
	 //auto_unlock(&vlg_TlvEventDblock);
        return NULL;
    }
    row->           data = entry;
    memcpy(entry->snmpTargetAddrName, snmpTargetAddrName,
           snmpTargetAddrName_len);
    entry->snmpTargetAddrName_len = snmpTargetAddrName_len;
    netsnmp_tdata_row_add_index(row, ASN_OCTET_STR,
                                entry->snmpTargetAddrName,
                                snmpTargetAddrName_len);
    netsnmp_tdata_add_row(table_data, row);
    //auto_unlock(&vlg_TlvEventDblock);
    return row;
}

/*
 * remove a row from the table
 */
void
    snmpTargetAddrExtTable_removeEntry(netsnmp_tdata * table_data,
                                       netsnmp_tdata_row * row)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    struct snmpTargetAddrExtTable_entry *entry;

    if (!row){
	 //auto_unlock(&vlg_TlvEventDblock);
        return;                 /* Nothing to remove */
    	}
    entry = (struct snmpTargetAddrExtTable_entry *)
        netsnmp_tdata_remove_and_delete_row(table_data, row);
    if (entry)
        SNMP_FREE(entry);       /* XXX - release any other internal resources */
    //auto_unlock(&vlg_TlvEventDblock);
}
#if 1
// shared from snmpTargetAddrTable.cpp
extern vector<VL_TLV_TRANSPORT_ADDR_AND_MASK> vlg_snmp_agent_TargetAddrList;
// shared from snmpTargetParamsTable.cpp
extern vector<v3NotificationReceiver_t> vlg_snmp_agent_NotificationReceiverList;
#endif //if 0

/*
 * Example cache handling - set up table_data list from a suitable file
 */
int snmpTargetAddrExtTable_load(netsnmp_cache * cache, void *vmagic)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    netsnmp_tdata * table_data = (netsnmp_tdata *) vmagic;
    netsnmp_tdata_row *row;

    SNMP_DEBUGPRINT("\n snmpTargetAddrExtTable_load :: Start snmpTargetAddrExtTable_load   :::  \n");
    bool replace_table = false;
    int count = 0;

    if(netsnmp_tdata_row_first(table_data))
    {
        replace_table = true;
    }
    for(count = 0; count < vlg_snmp_agent_TargetAddrList.size(); count++)
    {
        if(replace_table)
        {
            replace_table = false;
            Table_free(table_data);
        }

        snmpTargetAddrExtTable_createEntry_allData(table_data, vlg_snmp_agent_TargetAddrList[count], count);

    }
    for(count = 0; count < vlg_snmp_agent_NotificationReceiverList.size(); count++)
    {
        if(replace_table)
        {
            replace_table = false;
            Table_free(table_data);
        }

        snmpTargetAddrExtTable_createEntry_allData(table_data, vlg_snmp_agent_NotificationReceiverList[count], count);

    }
    if((vlg_snmp_agent_TargetAddrList.size() == 0) && (0 == vlg_snmp_agent_NotificationReceiverList.size()))
    {
        Table_free(table_data);
        VL_TLV_TRANSPORT_ADDR_AND_MASK targetAddr;
        snmpTargetAddrExtTable_createEntry_allData(table_data, targetAddr, count);
    }
    SNMP_DEBUGPRINT("\n snmpTargetAddrExtTable_load :: End snmpTargetAddrExtTable_load   :::  \n");
    //auto_unlock(&vlg_TlvEventDblock);
    return 1;
}

int snmpTargetAddrExtTable_createEntry_allData(netsnmp_tdata * table_data, struct VL_TLV_TRANSPORT_ADDR_AND_MASK & TargetAddr, int count)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    SNMP_DEBUGPRINT("\n snmpTargetAddrExtTable_createEntry_allData ::Start \n");
    char TargetAddrName[STRLEN_CHAR_ACCESS];
    char TargetAddrTMas[STRLEN_CHAR_ACCESS];
    struct snmpTargetAddrExtTable_entry *entry;
    netsnmp_tdata_row *row;
    entry = SNMP_MALLOC_TYPEDEF(struct snmpTargetAddrExtTable_entry);
    if (!entry){
    //auto_unlock(&vlg_TlvEventDblock);	
    return 0;
    	}

    row = netsnmp_tdata_create_row();
    if (!row) {
    SNMP_FREE(entry);
    //auto_unlock(&vlg_TlvEventDblock);
    return 0;
    }
    row->data = entry;
     /*
     * Index values
     */

    snprintf(TargetAddrName, sizeof(TargetAddrName), "%sconfigTag_%d_%d", TargetAddr.strPrefix.c_str(), TargetAddr.iAccessViewTlv, TargetAddr.iTlvSubEntry);
  //   snprintf(TargetAddrTMas, sizeof(TargetAddrTMas), "@STBconfig_%d",count);//"@STBconfig_n" same as index

    memcpy(entry->snmpTargetAddrName, TargetAddrName,strlen(TargetAddrName));  //@STBconfigTag_0_0 , @STBconfigTag_1_0?
    entry->snmpTargetAddrName_len = strlen(TargetAddrName);
     /*
     * Column values
     */
    SNMP_DEBUGPRINT(" entry->snmpTargetAddrName ::: %s \n", entry->snmpTargetAddrName);
    string strMaskHex = TargetAddr.getMaskStringHex();
    entry->snmpTargetAddrTMask_len = strMaskHex.size();
    memcpy(entry->snmpTargetAddrTMask, strMaskHex.c_str(), entry->snmpTargetAddrTMask_len);     //<empty>FF 00 00 00 00 00
    SNMP_DEBUGPRINT("entry->snmpTargetAddrTMask ::: %s\n", entry->snmpTargetAddrTMask);

    //char            old_snmpTargetAddrTMask[STRLEN_CHAR_ACCESS];
    //size_t          old_snmpTargetAddrTMask_len;
    entry->snmpTargetAddrMMS = 484/* txmms*/;            //484
    //long            old_snmpTargetAddrMMS;
    SNMP_DEBUGPRINT("entry->snmpTargetAddrMMS ::: %d\n", entry->snmpTargetAddrMMS);
//     netsnmp_tdata_row_add_index(row, ASN_UNSIGNED,
//             (count+1),
//             sizeof(count+1));
    netsnmp_tdata_row_add_index(row, ASN_PRIV_IMPLIED_OCTET_STR, // was ASN_OCTET_STR,
                                entry->snmpTargetAddrName,
                                entry->snmpTargetAddrName_len);

      netsnmp_tdata_add_row(table_data, row);
    SNMP_DEBUGPRINT("\n snmpTargetAddrExtTable_createEntry_allData ::END \n");
   //auto_unlock(&vlg_TlvEventDblock);
   return 1;

}

int snmpTargetAddrExtTable_createEntry_allData(netsnmp_tdata * table_data, struct v3NotificationReceiver_t & rReceiver, int count)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    SNMP_DEBUGPRINT("\n snmpTargetAddrExtTable_createEntry_allData ::Start \n");
    char TargetAddrName[STRLEN_CHAR_ACCESS] = {0};
    char TargetAddrTMas[STRLEN_CHAR_ACCESS];
    struct snmpTargetAddrExtTable_entry *entry;
    netsnmp_tdata_row *row;
    entry = SNMP_MALLOC_TYPEDEF(struct snmpTargetAddrExtTable_entry);
    if (!entry){
	 //auto_unlock(&vlg_TlvEventDblock);
        return 0;
    	}
    row = netsnmp_tdata_create_row();
    if (!row) {
        SNMP_FREE(entry);
	 //auto_unlock(&vlg_TlvEventDblock);
        return 0;
    }
    row->data = entry;
     /*
    * Index values
     */

    if(rReceiver.v3ipV4address.size() > 0)
    {
        snprintf(TargetAddrName, sizeof(TargetAddrName), "@STBnotifyconfig_%d_IPv4",count);
    }
    if(rReceiver.v3ipV6address.size() > 0)
    {
        snprintf(TargetAddrName, sizeof(TargetAddrName), "@STBnotifyconfig_%d_IPv6",count);
    }
    //   snprintf(TargetAddrTMas, sizeof(TargetAddrTMas), "@STBconfig_%d",count);//"@STBconfig_n" same as index

    memcpy(entry->snmpTargetAddrName, TargetAddrName,strlen(TargetAddrName));  //@STBconfigTag_0_0 , @STBconfigTag_1_0?
    entry->snmpTargetAddrName_len = strlen(TargetAddrName);
     /*
    * Column values
     */
    SNMP_DEBUGPRINT(" entry->snmpTargetAddrName ::: %s \n", entry->snmpTargetAddrName);
    entry->snmpTargetAddrTMask_len = 0;

    //char            old_snmpTargetAddrTMask[STRLEN_CHAR_ACCESS];
    //size_t          old_snmpTargetAddrTMask_len;
    entry->snmpTargetAddrMMS = 484/* txmms*/;            //484
    //long            old_snmpTargetAddrMMS;
    SNMP_DEBUGPRINT("entry->snmpTargetAddrMMS ::: %d\n", entry->snmpTargetAddrMMS);
    //     netsnmp_tdata_row_add_index(row, ASN_UNSIGNED,
    //          (count+1),
    //          sizeof(count+1));
    netsnmp_tdata_row_add_index(row, ASN_PRIV_IMPLIED_OCTET_STR, // was ASN_OCTET_STR,
                                entry->snmpTargetAddrName,
                                entry->snmpTargetAddrName_len);

    netsnmp_tdata_add_row(table_data, row);
    SNMP_DEBUGPRINT("\n snmpTargetAddrExtTable_createEntry_allData ::END \n");
    //auto_unlock(&vlg_TlvEventDblock);
    return 1;

}

void   snmpTargetAddrExtTable_free(netsnmp_cache * cache, void *vmagic)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    netsnmp_tdata * table = (netsnmp_tdata *) vmagic;
    /*netsnmp_tdata_row *this;

    while           ((this = netsnmp_tdata_get_first_row(table))) {
        netsnmp_tdata_remove_and_delete_row(table, this);
}*/
//auto_unlock(&vlg_TlvEventDblock);
}
/** handles requests for the snmpTargetAddrExtTable table */
int
    snmpTargetAddrExtTable_handler(netsnmp_mib_handler *handler,
                                   netsnmp_handler_registration *reginfo,
                                   netsnmp_agent_request_info *reqinfo,
                                   netsnmp_request_info *requests)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    netsnmp_request_info *request;
    netsnmp_table_request_info *table_info;
    netsnmp_tdata * table_data;
    netsnmp_tdata_row *table_row;
    netsnmp_tdata * vl_table_data;
    netsnmp_tdata_row *vl_row;
    struct snmpTargetAddrExtTable_entry *table_entry;
    int             ret;
    vl_row = NULL;

    switch (reqinfo->mode) {
        /*
         * Read-support (also covers GetNext requests)
         */
    case MODE_GET:
        for (request = requests; request; request = request->next) {
            /*changed due to wrong values in retriving first element of the Table*/
            VL_SNMP_PREPARE_AND_CHECK_TABLE_GET_REQUEST(snmpTargetAddrExtTable_entry);

            switch (table_info->colnum) {
            case COLUMN_SNMPTARGETADDRTMASK:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                         (u_char *) table_entry->
                                         snmpTargetAddrTMask,
                                         table_entry->
                                         snmpTargetAddrTMask_len);
                break;
            case COLUMN_SNMPTARGETADDRMMS:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->snmpTargetAddrMMS);
                break;
            default:
                netsnmp_set_request_error(reqinfo, request,
                                          SNMP_NOSUCHOBJECT);
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP", "%s(): returning SNMP_NOSUCHOBJECT\n", __FUNCTION__);
                break;
            }
        }
        break;

        /*
         * Write-support
         */
    case MODE_SET_RESERVE1:
        for (request = requests; request; request = request->next) {
                netsnmp_tdata_extract_entry(request);
            table_info = netsnmp_extract_table_info(request);

            switch (table_info->colnum) {
            case COLUMN_SNMPTARGETADDRTMASK:
                /*
                 * or possiblc 'netsnmp_check_vb_type_and_size'
                 */
                ret =
                    netsnmp_check_vb_type_and_max_size(request->requestvb,
                                                       ASN_OCTET_STR,
                                                       sizeof(table_entry->
                                                              snmpTargetAddrTMask));
                if (ret != SNMP_ERR_NOERROR) {
                    netsnmp_set_request_error(reqinfo, request, ret);
		      //auto_unlock(&vlg_TlvEventDblock);
                    return SNMP_ERR_NOERROR;
                }
                break;
            case COLUMN_SNMPTARGETADDRMMS:
                /*
                 * or possibly 'netsnmp_check_vb_int_range'
                 */
                ret = netsnmp_check_vb_int(request->requestvb);
                if (ret != SNMP_ERR_NOERROR) {
                    netsnmp_set_request_error(reqinfo, request, ret);
		      //auto_unlock(&vlg_TlvEventDblock);
                    return SNMP_ERR_NOERROR;
                }
                break;
            default:
                netsnmp_set_request_error(reqinfo, request,
                                          SNMP_ERR_NOTWRITABLE);
		  //auto_unlock(&vlg_TlvEventDblock);
                return SNMP_ERR_NOERROR;
            }
        }
        break;

    case MODE_SET_RESERVE2:
        for (request = requests; request; request = request->next) {
            table_row = netsnmp_tdata_extract_row(request);
            table_data = netsnmp_tdata_extract_table(request);
            table_info = netsnmp_extract_table_info(request);

            switch (table_info->colnum) {
            case COLUMN_SNMPTARGETADDRTMASK:
            case COLUMN_SNMPTARGETADDRMMS:
                if (!table_row) {
                    table_row =
                        snmpTargetAddrExtTable_createEntry(table_data,
                                                           (char*)table_info->
                                                           indexes->val.
                                                           string,
                                                           table_info->
                                                           indexes->
                                                           val_len);
                    if (table_row) {
                        netsnmp_insert_tdata_row(request, table_row);
                    } else {
                        netsnmp_set_request_error(reqinfo, request,
                                                  SNMP_ERR_RESOURCEUNAVAILABLE);
			   //auto_unlock(&vlg_TlvEventDblock);
                        return SNMP_ERR_NOERROR;
                    }
                }
                break;
            }
        }
        break;

    case MODE_SET_FREE:
        for (request = requests; request; request = request->next) {
            table_entry = (struct snmpTargetAddrExtTable_entry *)
                netsnmp_tdata_extract_entry(request);
            table_row = netsnmp_tdata_extract_row(request);
            table_data = netsnmp_tdata_extract_table(request);
            table_info = netsnmp_extract_table_info(request);

            switch (table_info->colnum) {
            case COLUMN_SNMPTARGETADDRTMASK:
            case COLUMN_SNMPTARGETADDRMMS:
                if (table_entry && !table_entry->valid) {
                    snmpTargetAddrExtTable_removeEntry(table_data,
                                                       table_row);
                }
                break;
            }
        }
        break;

    case MODE_SET_ACTION:
        for (request = requests; request; request = request->next) {
            table_entry = (struct snmpTargetAddrExtTable_entry *)
                netsnmp_tdata_extract_entry(request);
            table_info = netsnmp_extract_table_info(request);

            switch (table_info->colnum) {
            case COLUMN_SNMPTARGETADDRTMASK:
                memcpy(table_entry->old_snmpTargetAddrTMask,
                       table_entry->snmpTargetAddrTMask,
                       sizeof(table_entry->snmpTargetAddrTMask));
                table_entry->old_snmpTargetAddrTMask_len =
                    table_entry->snmpTargetAddrTMask_len;
                memset(table_entry->snmpTargetAddrTMask, 0,
                       sizeof(table_entry->snmpTargetAddrTMask));
                memcpy(table_entry->snmpTargetAddrTMask,
                       request->requestvb->val.string,
                       request->requestvb->val_len);
                table_entry->snmpTargetAddrTMask_len =
                    request->requestvb->val_len;
                break;
            case COLUMN_SNMPTARGETADDRMMS:
                table_entry->old_snmpTargetAddrMMS =
                    table_entry->snmpTargetAddrMMS;
                table_entry->snmpTargetAddrMMS =
                    *request->requestvb->val.integer;
                break;
            }
        }
        break;

    case MODE_SET_UNDO:
        for (request = requests; request; request = request->next) {
            table_entry = (struct snmpTargetAddrExtTable_entry *)
                netsnmp_tdata_extract_entry(request);
            table_row = netsnmp_tdata_extract_row(request);
            table_data = netsnmp_tdata_extract_table(request);
            table_info = netsnmp_extract_table_info(request);

            switch (table_info->colnum)
            {
                case COLUMN_SNMPTARGETADDRTMASK:
                {
                    if(NULL == table_entry)
                        break;

                    if (!table_entry->valid)
                    {
                        snmpTargetAddrExtTable_removeEntry(table_data,
                                                        table_row);
                    }
                    else
                    {
                        memcpy(table_entry->snmpTargetAddrTMask,
                            table_entry->old_snmpTargetAddrTMask,
                            sizeof(table_entry->snmpTargetAddrTMask));
                        memset(table_entry->old_snmpTargetAddrTMask, 0,
                            sizeof(table_entry->snmpTargetAddrTMask));
                        table_entry->snmpTargetAddrTMask_len =
                            table_entry->old_snmpTargetAddrTMask_len;
                    }

                    break;
                }
                case COLUMN_SNMPTARGETADDRMMS:
                {
                    if(NULL == table_entry)
                        break;

                    if (!table_entry->valid)
                    {
                        snmpTargetAddrExtTable_removeEntry(table_data,
                                                        table_row);
                    }
                    else
                    {
                        table_entry->snmpTargetAddrMMS =
                            table_entry->old_snmpTargetAddrMMS;
                        table_entry->old_snmpTargetAddrMMS = 0;
                    }
                    break;
                }
            }
        }
        break;

    case MODE_SET_COMMIT:
        for (request = requests; request; request = request->next) {
            table_entry = (struct snmpTargetAddrExtTable_entry *)
                netsnmp_tdata_extract_entry(request);
            table_info = netsnmp_extract_table_info(request);

            switch (table_info->colnum) {
            case COLUMN_SNMPTARGETADDRTMASK:
            case COLUMN_SNMPTARGETADDRMMS:
                if (table_entry && !table_entry->valid) {
                    table_entry->valid = 1;
                }
            }
        }
        break;
    }
    //auto_unlock(&vlg_TlvEventDblock);
    return SNMP_ERR_NOERROR;
}