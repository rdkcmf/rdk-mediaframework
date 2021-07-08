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
#ifndef VACMACCESSTABLE_H
#define VACMACCESSTABLE_H

/*
 * function declarations 
 */
void            init_vacmAccessTable(void);
void            initialize_table_vacmAccessTable(void);
Netsnmp_Node_Handler vacmAccessTable_handler;
NetsnmpCacheLoad vacmAccessTable_load;
NetsnmpCacheFree vacmAccessTable_free;
int vacmAccessTable_createEntry_allData(netsnmp_tdata *table_data, int accessviewtype,char *accviewname, int secmodel, int count);
#define VACMACCESSTABLE_TIMEOUT  10

/*
 * column number definitions for table vacmAccessTable 
 */
#define COLUMN_VACMACCESSCONTEXTPREFIX        1
#define COLUMN_VACMACCESSSECURITYMODEL        2
#define COLUMN_VACMACCESSSECURITYLEVEL        3
#define COLUMN_VACMACCESSCONTEXTMATCH        4
#define COLUMN_VACMACCESSREADVIEWNAME        5
#define COLUMN_VACMACCESSWRITEVIEWNAME        6
#define COLUMN_VACMACCESSNOTIFYVIEWNAME        7
#define COLUMN_VACMACCESSSTORAGETYPE        8
#define COLUMN_VACMACCESSSTATUS        9




//#ifndef VACMACCESSTABLE_ENUMS_H
//#define VACMACCESSTABLE_ENUMS_H
/*
 * enums for column vacmAccessSecurityLevel 
 */
#define VACMACCESSSECURITYLEVEL_NOAUTHNOPRIV        1
#define VACMACCESSSECURITYLEVEL_AUTHNOPRIV        2
#define VACMACCESSSECURITYLEVEL_AUTHPRIV        3

/*
 * enums for column vacmAccessContextMatch 
 */
#define VACMACCESSCONTEXTMATCH_EXACT        1
#define VACMACCESSCONTEXTMATCH_PREFIX        2

/*
 * enums for column vacmAccessStorageType 
 */
#define VACMACCESSSTORAGETYPE_OTHER        1
#define VACMACCESSSTORAGETYPE_VOLATILE        2
#define VACMACCESSSTORAGETYPE_NONVOLATILE        3
#define VACMACCESSSTORAGETYPE_PERMANENT        4
#define VACMACCESSSTORAGETYPE_READONLY        5

/*
 * enums for column vacmAccessStatus 
 */
#define VACMACCESSSTATUS_ACTIVE        1
#define VACMACCESSSTATUS_NOTINSERVICE        2
#define VACMACCESSSTATUS_NOTREADY        3
#define VACMACCESSSTATUS_CREATEANDGO        4
#define VACMACCESSSTATUS_CREATEANDWAIT        5
#define VACMACCESSSTATUS_DESTROY        6
//#endif                          /* VACMACCESSTABLE_ENUMS_H */



#endif                          /* VACMACCESSTABLE_H */