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
#include "ifTable.h"
#include "vl_ocStbHost_GetData.h"
#include "SnmpIORM.h"
#include "xchanResApi.h"
#include "rdk_debug.h"
#include "ipcclient_impl.h"
#ifdef YOCTO_BUILD
#include "secure_wrapper.h"
#endif
/** Initializes the ifTable module */
// void
// init_ifTable(void)
// {
//     /*
//      * here we initialize all the tables we're planning on supporting
//      */
//     initialize_table_ifTable();
// }
#define IF_TABLE_DESC_SIZE 3
extern char ifTableDesc[IF_TABLE_DESC_SIZE][API_CHRMAX];

int HOST_STB_ifAdminStatus_Disable = -1;
int CABLE_CARD_ifAdminStatus_Disable = -1;
long vlg_tcMocaIpLastChange = get_uptime();

/** Initialize the ifTable table by defining its contents and how it's structured */
void
initialize_table_ifTable(void)
{
    static oid      ifTable_oid[] = { 1, 3, 6, 1, 2, 1, 2, 2 };
    size_t          ifTable_oid_len = OID_LENGTH(ifTable_oid);
    netsnmp_handler_registration *reg;
    netsnmp_tdata  *table_data;
    netsnmp_table_registration_info *table_info;
    netsnmp_cache  *cache;

    reg = netsnmp_create_handler_registration("ifTable", ifTable_handler,
                                              ifTable_oid, ifTable_oid_len,
                                              HANDLER_CAN_RWRITE);

    table_data = netsnmp_tdata_create_table("ifTable", 0);
    table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info);
    netsnmp_table_helper_add_indexes(table_info, ASN_INTEGER,   /* index: ifIndex */  0);

    table_info->min_column = COLUMN_IFINDEX;
    table_info->max_column = COLUMN_IFSPECIFIC;

    netsnmp_tdata_register(reg, table_data, table_info);
    cache = netsnmp_cache_create(IFTABLE_TIMEOUT,
                                 ifTable_load, ifTable_free,
                                 ifTable_oid, ifTable_oid_len);
    cache->magic = (void *) table_data;
 cache->flags = NETSNMP_CACHE_DONT_INVALIDATE_ON_SET |
NETSNMP_CACHE_DONT_FREE_BEFORE_LOAD | NETSNMP_CACHE_DONT_FREE_EXPIRED |
NETSNMP_CACHE_DONT_AUTO_RELEASE;



    netsnmp_inject_handler_before(reg, netsnmp_cache_handler_get(cache),
                                  "ifTable");

        /*
         * Initialise the contents of the table here
         */
         if(0 == ifTable_load(cache,table_data))
    {
            //ifTable_load"
      SNMP_DEBUGPRINT(" ERROR:: Not yet table initialise here \n");
    }

}

/*
 * Example cache handling - set up table_data list from a suitable file
 */
int ifTable_load(netsnmp_cache * cache, void *vmagic)
{
    netsnmp_tdata * table_data = (netsnmp_tdata *) vmagic;
    netsnmp_tdata_row *row;
    /**
     *ifTable_load module will  Initialise the contents of the table here
     */
     SNMP_DEBUGPRINT("................Start .ifTable_load ............\n");

    if(0 == vl_ifTable_getdata(table_data))
    {
    SNMP_DEBUGPRINT(" ERROR:: Not yet table initialise here \n");
    }

    SNMP_DEBUGPRINT("................END .ifTable_load ............\n");
    return 1;

}

void ifTable_free(netsnmp_cache * cache, void *vmagic) {
    netsnmp_tdata * table = (netsnmp_tdata *) vmagic;
//    netsnmp_tdata_row *this;

//     while           ((this = netsnmp_tdata_get_first_row(table))) {
//         netsnmp_tdata_remove_and_delete_row(table, this);
// }

}
/** handles requests for the ifTable table */
int
  ifTable_handler(netsnmp_mib_handler *handler,
                    netsnmp_handler_registration *reginfo,
                    netsnmp_agent_request_info *reqinfo,
                    netsnmp_request_info *requests) {

    netsnmp_request_info *request;
    netsnmp_table_request_info *table_info;
    netsnmp_tdata * vl_table_data;
    netsnmp_tdata_row *vl_row;
    struct ifTable_entry *table_entry;
    int             ret;

    vl_row = NULL;
    switch (reqinfo->mode) {
        /*
         * Read-support (also covers GetNext requests)
         */
    case MODE_GET:
        for (request = requests; request; request = request->next) {
            //table_entry = (struct ifTable_entry *)             netsnmp_tdata_extract_entry(request);
            /*changed due to wrong values in retriving first element of the Table*/
            VL_SNMP_PREPARE_AND_CHECK_TABLE_GET_REQUEST(ifTable_entry);

            switch (table_info->colnum) {
            case COLUMN_IFINDEX:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->ifIndex);
                break;
            case COLUMN_IFDESCR:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                         (u_char *) table_entry->ifDescr,
                                         table_entry->ifDescr_len);
                break;
            case COLUMN_IFTYPE:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->ifType);
                break;
            case COLUMN_IFMTU:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->ifMtu);
                break;
            case COLUMN_IFSPEED:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_GAUGE,
                                           table_entry->ifSpeed);
                break;
            case COLUMN_IFPHYSADDRESS:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                         (u_char *) table_entry->
                                         ifPhysAddress,
                                         table_entry->ifPhysAddress_len);
                break;
            case COLUMN_IFADMINSTATUS:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->ifAdminStatus);
                break;
            case COLUMN_IFOPERSTATUS:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->ifOperStatus);
                break;
            case COLUMN_IFLASTCHANGE:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb,
                                           ASN_TIMETICKS,
                                           table_entry->ifLastChange);
                break;
            case COLUMN_IFINOCTETS:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_COUNTER,
                                           table_entry->ifInOctets);
                break;
            case COLUMN_IFINUCASTPKTS:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_COUNTER,
                                           table_entry->ifInUcastPkts);
                break;
            case COLUMN_IFINNUCASTPKTS:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_COUNTER,
                                           table_entry->ifInNUcastPkts);
                break;
            case COLUMN_IFINDISCARDS:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_COUNTER,
                                           table_entry->ifInDiscards);
                break;
            case COLUMN_IFINERRORS:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_COUNTER,
                                           table_entry->ifInErrors);
                break;
            case COLUMN_IFINUNKNOWNPROTOS:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_COUNTER,
                                           table_entry->ifInUnknownProtos);
                break;
            case COLUMN_IFOUTOCTETS:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_COUNTER,
                                           table_entry->ifOutOctets);
                break;
            case COLUMN_IFOUTUCASTPKTS:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_COUNTER,
                                           table_entry->ifOutUcastPkts);
                break;
            case COLUMN_IFOUTNUCASTPKTS:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_COUNTER,
                                           table_entry->ifOutNUcastPkts);
                break;
            case COLUMN_IFOUTDISCARDS:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_COUNTER,
                                           table_entry->ifOutDiscards);
                break;
            case COLUMN_IFOUTERRORS:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_COUNTER,
                                           table_entry->ifOutErrors);
                break;
            case COLUMN_IFOUTQLEN:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_GAUGE,
                                           table_entry->ifOutQLen);
                break;
            case COLUMN_IFSPECIFIC:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OBJECT_ID,
                                         (u_char *) table_entry->
                                         ifSpecific,
                                         table_entry->ifSpecific_len);
                break;
            default:
                netsnmp_set_request_error(reqinfo, request,
                                          SNMP_NOSUCHOBJECT);
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
            case COLUMN_IFADMINSTATUS:
                /*
                 * or possibly 'netsnmp_check_vb_int_range'
                 */
                ret = netsnmp_check_vb_int(request->requestvb);
                if (ret != SNMP_ERR_NOERROR) {
                    netsnmp_set_request_error(reqinfo, request, ret);
                    return SNMP_ERR_NOERROR;
                }
                break;
            default:
                netsnmp_set_request_error(reqinfo, request,
                                          SNMP_ERR_NOTWRITABLE);
                return SNMP_ERR_NOERROR;
            }
        }
        break;

    case MODE_SET_RESERVE2:
        break;

    case MODE_SET_FREE:
        break;

    case MODE_SET_ACTION:
        for (request = requests; request; request = request->next) {
            table_entry = (struct ifTable_entry *)
                netsnmp_tdata_extract_entry(request);
            table_info = netsnmp_extract_table_info(request);

            switch (table_info->colnum) {
            case COLUMN_IFADMINSTATUS:
            {
                table_entry->old_ifAdminStatus =
                    table_entry->ifAdminStatus;
                table_entry->ifAdminStatus =
                    *request->requestvb->val.integer;

            //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "\ncmp table_entry->ifDescr = '%s':: %d\n", table_entry->ifDescr, strcmp(table_entry->ifDescr,"OCHD2 Embedded IP 2-way Interface"));
                if(0 == strncmp(table_entry->ifDescr,ifTableDesc[0], API_CHRMAX))
                {
                     //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "\nHOST::::: %s  \n",table_entry->ifDescr);
                     if(IFADMINSTATUS_DOWN == *request->requestvb->val.integer)
                     {
                         HOST_STB_ifAdminStatus_Disable = IFADMINSTATUS_DOWN;
                         {
                             char cmd_buffer[256];
                             VL_HOST_IP_INFO Aadm_phostipinfo;
                             IPC_CLIENT_vlXchanGetDsgEthName(&Aadm_phostipinfo);
                             RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "VL_HOST_IP_INFO :::::::: %s\n",Aadm_phostipinfo.szIfName);
                             snprintf(cmd_buffer, sizeof(cmd_buffer), "ifconfig %s down",Aadm_phostipinfo.szIfName);
                             RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s\n",cmd_buffer);
#ifdef YOCTO_BUILD
                             v_secure_system("ifconfig %s down", Aadm_phostipinfo.szIfName);
#else
                             system((const char *)&cmd_buffer);
#endif
                         }
                     }
                     else
                     {
                         HOST_STB_ifAdminStatus_Disable = IFADMINSTATUS_UP;
                         {
                             char cmd_buffer[256];
                             VL_HOST_IP_INFO Aadm_phostipinfo;
                             IPC_CLIENT_vlXchanGetDsgEthName(&Aadm_phostipinfo);
                             RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "VL_HOST_IP_INFO :::::::: %s\n",Aadm_phostipinfo.szIfName);
                             snprintf(cmd_buffer, sizeof(cmd_buffer), "ifconfig %s up",Aadm_phostipinfo.szIfName);
                             RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s\n",cmd_buffer);
#ifdef YOCTO_BUILD
                             v_secure_system("ifconfig %s up",Aadm_phostipinfo.szIfName);
#else
                             system((const char *)&cmd_buffer);
#endif
                         }
                     }
                }
                //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "\n cmp table_entry->ifDescr = '%s':: %d\n ", table_entry->ifDescr, strcmp(table_entry->ifDescr,"CableCARD Unicast IP Flow"));

                if(0 == strncmp(table_entry->ifDescr,ifTableDesc[1], API_CHRMAX))
                {
                    //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "\n Card:::::: %s  \n",table_entry->ifDescr);
                    if(IFADMINSTATUS_DOWN == *request->requestvb->val.integer)
                     {
                         CABLE_CARD_ifAdminStatus_Disable = IFADMINSTATUS_DOWN;
                         {
                             IPC_CLIENT_vlXchanIndicateLostDsgIPUFlow();
                         }
                     }
                     else
                     {
                         CABLE_CARD_ifAdminStatus_Disable = IFADMINSTATUS_UP;
                         // nothing to be done here
                     }
                }
                if(0 == strncmp(table_entry->ifDescr,ifTableDesc[2], API_CHRMAX))
                {
                     vlg_tcMocaIpLastChange = get_uptime();
                     if(IFADMINSTATUS_DOWN == *request->requestvb->val.integer)
                     {
                         HOST_STB_ifAdminStatus_Disable = IFADMINSTATUS_DOWN;
                         {
                             char cmd_buffer[256];
                             RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "VL_HOST_IP_INFO :::::::: %s\n","moca");
                             //snprintf(cmd_buffer, sizeof(cmd_buffer), "ifconfig %s down \n",pszIfName);
                             snprintf(cmd_buffer, sizeof(cmd_buffer), "sh /lib/rdk/mocaAdminDown.sh","moca");
                             RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s\n",cmd_buffer);
#ifdef YOCTO_BUILD
                             v_secure_system("sh /lib/rdk/mocaAdminDown.sh");
#else
                             system((const char *)&cmd_buffer);
#endif
                         }
                     }
                     else
                     {
                         HOST_STB_ifAdminStatus_Disable = IFADMINSTATUS_UP;
                         {
                             char cmd_buffer[256];
                             RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "VL_HOST_IP_INFO :::::::: %s\n","moca");
                             //snprintf(cmd_buffer, sizeof(cmd_buffer), "ifconfig %s up \n",pszIfName);
                             snprintf(cmd_buffer, sizeof(cmd_buffer), "sh /lib/rdk/mocaAdminUp.sh","moca");
                             RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s\n",cmd_buffer);
#ifdef YOCTO_BUILD
                             v_secure_system("sh /lib/rdk/mocaAdminUp.sh");
#else
                             system((const char *)&cmd_buffer);
#endif
                         }
                     }
                }
                break;   /* break from the switch case, to iterate through requests*/
            } /*Case COLUMN_IFADMINSTATUS Ends*/

            }

        }
        break;

    case MODE_SET_UNDO:
        for (request = requests; request; request = request->next) {
            table_entry = (struct ifTable_entry *)
                netsnmp_tdata_extract_entry(request);
            netsnmp_tdata_extract_row(request);
            netsnmp_tdata_extract_table(request);
            table_info = netsnmp_extract_table_info(request);

            switch (table_info->colnum) {
            case COLUMN_IFADMINSTATUS:
                table_entry->ifAdminStatus =
                    table_entry->old_ifAdminStatus;
                table_entry->old_ifAdminStatus = 0;
                break;
            }
        }
        break;

    case MODE_SET_COMMIT:
        break;
    }
    return SNMP_ERR_NOERROR;
}
