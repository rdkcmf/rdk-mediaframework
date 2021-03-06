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
#ifndef IPNETTOPHYSICALTABLE_H
#define IPNETTOPHYSICALTABLE_H
#include "ocStbHostMibModule.h"
/*
 * function declarations 
 */
//void            init_ipNetToPhysicalTable(void);
void            initialize_table_ipNetToPhysicalTable(void);
Netsnmp_Node_Handler ipNetToPhysicalTable_handler;
NetsnmpCacheLoad ipNetToPhysicalTable_load;
NetsnmpCacheFree ipNetToPhysicalTable_free;
#define IPNETTOPHYSICALTABLE_TIMEOUT  2

/*
 * column number definitions for table ipNetToPhysicalTable 
 */
#define COLUMN_IPNETTOPHYSICALIFINDEX        1
#define COLUMN_IPNETTOPHYSICALNETADDRESSTYPE        2
#define COLUMN_IPNETTOPHYSICALNETADDRESS        3
#define COLUMN_IPNETTOPHYSICALPHYSADDRESS        4
#define COLUMN_IPNETTOPHYSICALLASTUPDATED        5
#define COLUMN_IPNETTOPHYSICALTYPE        6
#define COLUMN_IPNETTOPHYSICALSTATE        7
#define COLUMN_IPNETTOPHYSICALROWSTATUS        8

 /*
     * Typical data structure for a row entry 
     */
struct ipNetToPhysicalTable_entry {
    /*
     * Index values 
     */
    long            ipNetToPhysicalIfIndex;

    /*
     * Column values
     */
    long            ipNetToPhysicalNetAddressType;
    char            ipNetToPhysicalNetAddress[CHRMAX];
    size_t          ipNetToPhysicalNetAddress_len;
    char            ipNetToPhysicalPhysAddress[CHRMAX];
    size_t          ipNetToPhysicalPhysAddress_len;
    u_long          ipNetToPhysicalLastUpdated;
    long            ipNetToPhysicalType;
    long            ipNetToPhysicalState;
    long            ipNetToPhysicalRowStatus;

    int             valid;
};


#endif                          /* IPNETTOPHYSICALTABLE_H */
