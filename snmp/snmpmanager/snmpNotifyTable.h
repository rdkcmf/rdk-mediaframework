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
#ifndef SNMPNOTIFYTABLE_H
#define SNMPNOTIFYTABLE_H
#include "ocStbHostMibModule.h"

/* function declarations */
void init_snmpNotifyTable(void);
void initialize_table_snmpNotifyTable(void);
extern "C" void snmpNotifyTableEventhandling(char *SnmpAddt);
Netsnmp_Node_Handler snmpNotifyTable_handler;
int snmpNotifyTable_load(netsnmp_cache * cache, void *vmagic);
void snmpNotifyTable_free(netsnmp_cache * cache, void *vmagic);
int snmpNotifyTable_createEntry_allData(netsnmp_tdata * table_data, int eNotifyType, int count);
#define SNMPNOTIFYTABLE_TIMEOUT  2

/*
 * column number definitions for table snmpNotifyTable
 */
       #define COLUMN_SNMPNOTIFYNAME        1
       #define COLUMN_SNMPNOTIFYTAG        2
       #define COLUMN_SNMPNOTIFYTYPE        3
       #define COLUMN_SNMPNOTIFYSTORAGETYPE        4
       #define COLUMN_SNMPNOTIFYROWSTATUS        5


    
/*
 * enums for column snmpNotifyType
 */
#define SNMPNOTIFYTYPE_TRAP     1
#define SNMPNOTIFYTYPE_INFORM   2

struct snmpNotifyTable_entry {
    /* Index values */
    char snmpNotifyName[CHRMAX];
    size_t snmpNotifyName_len;

    /* Column values */
    char snmpNotifyTag[CHRMAX];
    size_t snmpNotifyTag_len;
    char old_snmpNotifyTag[CHRMAX];
    size_t old_snmpNotifyTag_len;
    long snmpNotifyType;
    long old_snmpNotifyType;
    long snmpNotifyStorageType;
    long old_snmpNotifyStorageType;
    long snmpNotifyRowStatus;

    int   valid;
};

#endif /* SNMPNOTIFYTABLE_H */


