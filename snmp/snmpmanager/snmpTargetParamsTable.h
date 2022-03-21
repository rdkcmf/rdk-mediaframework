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
#ifndef SNMPTARGETPARAMSTABLE_H
#define SNMPTARGETPARAMSTABLE_H
#include "ocStbHostMibModule.h"
/*
 * function declarations
 */
//void            init_snmpTargetParamsTable(void);
void            initialize_table_snmpTargetParamsTable(void);
extern "C" void snmpTargetParamsTableEventhandling(struct v3NotificationReceiver_t & rReceiver);
extern "C" void snmpClearTargetParamsTableEvents();
Netsnmp_Node_Handler snmpTargetParamsTable_handler;
int snmpTargetParamsTable_load(netsnmp_cache * cache, void *vmagic);
void snmpTargetParamsTable_free(netsnmp_cache * cache, void *vmagic);
int snmpTargetParamsTable_createEntry_allData(netsnmp_tdata * table_data, struct v3NotificationReceiver_t & rReceiver, int count);
#define SNMPTARGETPARAMSTABLE_TIMEOUT  2

/*
 * column number definitions for table snmpTargetParamsTable
 */
#define COLUMN_SNMPTARGETPARAMSNAME        1
#define COLUMN_SNMPTARGETPARAMSMPMODEL        2
#define COLUMN_SNMPTARGETPARAMSSECURITYMODEL        3
#define COLUMN_SNMPTARGETPARAMSSECURITYNAME        4
#define COLUMN_SNMPTARGETPARAMSSECURITYLEVEL        5
#define COLUMN_SNMPTARGETPARAMSSTORAGETYPE        6
#define COLUMN_SNMPTARGETPARAMSROWSTATUS        7



 /*
 * enums for column snmpTargetParamsSecurityLevel
 */
#define SNMPTARGETPARAMSSECURITYLEVEL_NOAUTHNOPRIV              1
#define SNMPTARGETPARAMSSECURITYLEVEL_AUTHNOPRIV                2
#define SNMPTARGETPARAMSSECURITYLEVEL_AUTHPRIV          3

/*
 * enums for column snmpTargetParamsStorageType
 */
#define SNMPTARGETPARAMSSTORAGETYPE_OTHER               1
#define SNMPTARGETPARAMSSTORAGETYPE_VOLATILE            2
#define SNMPTARGETPARAMSSTORAGETYPE_NONVOLATILE         3
#define SNMPTARGETPARAMSSTORAGETYPE_PERMANENT           4
#define SNMPTARGETPARAMSSTORAGETYPE_READONLY            5

/*
 * enums for column snmpTargetParamsRowStatus
 */
#define SNMPTARGETPARAMSROWSTATUS_ACTIVE                1
#define SNMPTARGETPARAMSROWSTATUS_NOTINSERVICE          2
#define SNMPTARGETPARAMSROWSTATUS_NOTREADY              3
#define SNMPTARGETPARAMSROWSTATUS_CREATEANDGO           4
#define SNMPTARGETPARAMSROWSTATUS_CREATEANDWAIT         5
#define SNMPTARGETPARAMSROWSTATUS_DESTROY               6

 /*
     * Typical data structure for a row entry
 */
struct snmpTargetParamsTable_entry {
    /*
    * Index values
    */
    char            snmpTargetParamsName[CHRMAX];
    size_t          snmpTargetParamsName_len;

    /*
    * Column values
    */
    long            snmpTargetParamsMPModel;
    long            old_snmpTargetParamsMPModel;
    long            snmpTargetParamsSecurityModel;
    long            old_snmpTargetParamsSecurityModel;
    char            snmpTargetParamsSecurityName[CHRMAX];
    size_t          snmpTargetParamsSecurityName_len;
    char            old_snmpTargetParamsSecurityName[CHRMAX];
    size_t          old_snmpTargetParamsSecurityName_len;
    long            snmpTargetParamsSecurityLevel;
    long            old_snmpTargetParamsSecurityLevel;
    long            snmpTargetParamsStorageType;
    long            old_snmpTargetParamsStorageType;
    long            snmpTargetParamsRowStatus;

    int             valid;
};
#endif                          /* SNMPTARGETPARAMSTABLE_H */
