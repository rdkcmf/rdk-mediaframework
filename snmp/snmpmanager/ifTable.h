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
#ifndef IFTABLE_H
#define IFTABLE_H
#include "ocStbHostMibModule.h"
/*
 * function declarations 
 */
//void            init_ifTable(void);
void            initialize_table_ifTable(void);
Netsnmp_Node_Handler ifTable_handler;
NetsnmpCacheLoad ifTable_load;
NetsnmpCacheFree ifTable_free;
#define IFTABLE_TIMEOUT  2

/*
 * column number definitions for table ifTable 
 */
#define COLUMN_IFINDEX        1
#define COLUMN_IFDESCR        2
#define COLUMN_IFTYPE        3
#define COLUMN_IFMTU        4
#define COLUMN_IFSPEED        5
#define COLUMN_IFPHYSADDRESS        6
#define COLUMN_IFADMINSTATUS        7
#define COLUMN_IFOPERSTATUS        8
#define COLUMN_IFLASTCHANGE        9
#define COLUMN_IFINOCTETS        10
#define COLUMN_IFINUCASTPKTS        11
#define COLUMN_IFINNUCASTPKTS        12
#define COLUMN_IFINDISCARDS        13
#define COLUMN_IFINERRORS        14
#define COLUMN_IFINUNKNOWNPROTOS        15
#define COLUMN_IFOUTOCTETS        16
#define COLUMN_IFOUTUCASTPKTS        17
#define COLUMN_IFOUTNUCASTPKTS        18
#define COLUMN_IFOUTDISCARDS        19
#define COLUMN_IFOUTERRORS        20
#define COLUMN_IFOUTQLEN        21
#define COLUMN_IFSPECIFIC        22

 /*
     * Typical data structure for a row entry 
     */
struct ifTable_entry {
    /*
     * Index values 
     */
    long            ifIndex;

    /*
     * Column values 
     */
    long            ifindex;
    char            ifDescr[CHRMAX];
    size_t          ifDescr_len;
    long            ifType;
    long            ifMtu;
    u_long          ifSpeed;
    char            ifPhysAddress[CHRMAX];
    size_t          ifPhysAddress_len;
    long            ifAdminStatus;
    long            old_ifAdminStatus;
    long            ifOperStatus;
    u_long          ifLastChange;
    u_long          ifInOctets;
    u_long          ifInUcastPkts;
    u_long          ifInNUcastPkts;
    u_long          ifInDiscards;
    u_long          ifInErrors;
    u_long          ifInUnknownProtos;
    u_long          ifOutOctets;
    u_long          ifOutUcastPkts;
    u_long          ifOutNUcastPkts;
    u_long          ifOutDiscards;
    u_long          ifOutErrors;
    u_long          ifOutQLen;
    oid             ifSpecific[CHRMAX];
    size_t          ifSpecific_len;

    int             valid;
};



#endif                          /* IFTABLE_H */
