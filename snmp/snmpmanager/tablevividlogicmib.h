/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2009 RDK Management
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
#ifndef VIVIDLOGICMIB_H
#define VIVIDLOGICMIB_H
#include "ocStbHostMibModule.h"
/**SNMP certification*/
#define SNMP_CT_STR_SIZE    128
#define MODEL_IDSIZE 4
#define VENDOR_IDSIZE 3
#define DEVICE_IDSIZE 5
#define GUIDMAC_ADDSIZE 8
/**CARD certification*/
#define CARD_SIZE 512
#define HOSTID_SIZE 512

/*
 * function declarations 
 */
void            nit_Certinfomib(void);
void            initialize_table_Ieee1394certificationTable(void);
Netsnmp_Node_Handler Ieee1394certificationTable_handler;
NetsnmpCacheLoad Ieee1394certificationTable_load;
NetsnmpCacheFree Ieee1394certificationTable_free;
#define IEEE1394CERTIFICATIONTABLE_TIMEOUT  2
void            initialize_table_McardcertificationTable(void);
Netsnmp_Node_Handler McardcertificationTable_handler;
NetsnmpCacheLoad McardcertificationTable_load;
NetsnmpCacheFree McardcertificationTable_free;
#define MCARDCERTIFICATIONTABLE_TIMEOUT  2

/*
 * column number definitions for table Ieee1394certificationTable 
 */
#define COLUMN_VIVIDLOGICCEFINTERFACEINDEX		1
#define COLUMN_VIVIDLOGICSZDISPSTRING		2
#define COLUMN_VIVIDLOGICSZKEYINFOSTRING		3
#define COLUMN_VIVIDLOGICSZMFRNAME		4
#define COLUMN_VIVIDLOGICSZMODELNAME		5
#define COLUMN_VIVIDLOGICACMODELID		6
#define COLUMN_VIVIDLOGICAACVENDORID		7
#define COLUMN_VIVIDLOGICAACDEVICEID		8
#define COLUMN_VIVIDLOGICAACGUIDORMACADDR		9
#define COLUMN_VIVIDLOGICBCERTAVAILABLE		10
#define COLUMN_VIVIDLOGICBBCERTVALID		11
#define COLUMN_VIVIDLOGICBBISPRODUCTION		12

/*
 * column number definitions for table McardcertificationTable 
 */
#define COLUMN_VIVIDLOGICMCARDINTERFACEINDEX		1
#define COLUMN_VIVIDLOGICSZDISPSTRING		2
#define COLUMN_VIVIDLOGICMCARDSZDEVICECERTSUBJECTNAME		3
#define COLUMN_VIVIDLOGICMCARDSZDEVICECERTISSUERNAME		4
#define COLUMN_VIVIDLOGICMCARDSZMANCERTSUBJECTNAME		5
#define COLUMN_VIVIDLOGICMCARDSZMANCERTISSUERNAME		6
#define COLUMN_VIVIDLOGICMCARDACHOSTID		7
#define COLUMN_VIVIDLOGICMCARDBCERTAVAILABLE		8
#define COLUMN_VIVIDLOGICMCARDBCERTVALID		9
#define COLUMN_VIVIDLOGICMCARDBVERIFIEDWITHCHAIN		10
#define COLUMN_VIVIDLOGICMCARDBISPRODUCTION		11

    /*
     * Typical data structure for a row entry 
     */
struct Ieee1394certificationTable_entry {
    /*
     * Index values 
     */
    u_long          VividlogicCEFInterfaceIndex;

    /*
     * Column values 
     */
    char            VividlogicSzDispString[SNMP_CT_STR_SIZE];
    size_t          VividlogicSzDispString_len;
    char            VividlogicSzKeyInfoString[SNMP_CT_STR_SIZE];
    size_t          VividlogicSzKeyInfoString_len;
    char            VividlogicSzMfrName[SNMP_CT_STR_SIZE];
    size_t          VividlogicSzMfrName_len;
    char            VividlogicSzModelName[SNMP_CT_STR_SIZE];
    size_t          VividlogicSzModelName_len;
    char            VividlogicAcModelId[MODEL_IDSIZE];
    size_t          VividlogicAcModelId_len;
    char            VividlogicAacVendorId[VENDOR_IDSIZE];
    size_t          VividlogicAacVendorId_len;
    char            VividlogicAacDeviceId[DEVICE_IDSIZE];
    size_t          VividlogicAacDeviceId_len;
    char            VividlogicAacGUIDorMACADDR[GUIDMAC_ADDSIZE];
    size_t          VividlogicAacGUIDorMACADDR_len;
    long            VividlogicBCertAvailable;
    long            VividlogicBbCertValid;
    long            VividlogicBbIsProduction;

    int             valid;
};

  /*
     * Typical data structure for a row entry 
     */
struct McardcertificationTable_entry {
    /*
     * Index values 
     */
    u_long          VividlogicMcardInterfaceIndex;

    /*
     * Column values 
     */
    char            VividlogicSzDispString[CARD_SIZE];
    size_t          VividlogicSzDispString_len;
    char            VividlogicMcardSzDeviceCertSubjectName[CARD_SIZE];
    size_t          VividlogicMcardSzDeviceCertSubjectName_len;
    char            VividlogicMcardSzDeviceCertIssuerName[CARD_SIZE];
    size_t          VividlogicMcardSzDeviceCertIssuerName_len;
    char            VividlogicMcardSzManCertSubjectName[CARD_SIZE];
    size_t          VividlogicMcardSzManCertSubjectName_len;
    char            VividlogicMcardSzManCertIssuerName[CARD_SIZE];
    size_t          VividlogicMcardSzManCertIssuerName_len;
    char            VividlogicMcardAcHostId[HOSTID_SIZE];
    size_t          VividlogicMcardAcHostId_len;
    char            VividlogicMcardbCertAvailable[4];
    char            VividlogicMcardbCertValid[4];
    char            VividlogicMcardbVerifiedWithChain[4];
    char            VividlogicMcardbIsProduction[4];

    int             valid;
};


#endif                          /* VIVIDLOGICMIB_H */
