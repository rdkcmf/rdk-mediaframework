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
#ifndef SNMPTARGETADDRTABLE_H
#define SNMPTARGETADDRTABLE_H
#include "ocStbHostMibModule.h"

/*
 * function declarations 
 */
void            init_snmpTargetAddrTable(void);
void            initialize_table_snmpTargetAddrTable(void);
Netsnmp_Node_Handler snmpTargetAddrTable_handler;
NetsnmpCacheLoad snmpTargetAddrTable_load;
NetsnmpCacheFree snmpTargetAddrTable_free;
extern "C" void snmpTargetAddrTableEventhandling(struct VL_TLV_TRANSPORT_ADDR_AND_MASK & SnmpAddt);//53.2 Transport add access SubTLV ADdrTable 53.2.1
extern "C" void snmpClearTargetAddrTableEvents();
int snmpTargetAddrTable_createEntry_allData(netsnmp_tdata * table_data, struct VL_TLV_TRANSPORT_ADDR_AND_MASK & TargetAddr, int count);
int snmpTargetAddrTable_createEntry_allData(netsnmp_tdata * table_data, struct v3NotificationReceiver_t & rReceiver, int count);
#define SNMPTARGETADDRTABLE_TIMEOUT  2

/*
 * column number definitions for table snmpTargetAddrTable 
 */
#define COLUMN_SNMPTARGETADDRNAME        1
#define COLUMN_SNMPTARGETADDRTDOMAIN        2
#define COLUMN_SNMPTARGETADDRTADDRESS        3
#define COLUMN_SNMPTARGETADDRTIMEOUT        4
#define COLUMN_SNMPTARGETADDRRETRYCOUNT        5
#define COLUMN_SNMPTARGETADDRTAGLIST        6
#define COLUMN_SNMPTARGETADDRPARAMS        7
#define COLUMN_SNMPTARGETADDRSTORAGETYPE        8
#define COLUMN_SNMPTARGETADDRROWSTATUS        9
#endif                          /* SNMPTARGETADDRTABLE_H */
