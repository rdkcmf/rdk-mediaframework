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

#ifndef HOST_H
#define HOST_H

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "SnmpIORM.h"
//#include "ocStbHostMibModule.h"
/*
 * function declarations
 */

void  init_host(void);
void  initialize_table_hrStorageTable(void);
Netsnmp_Node_Handler hrStorageTable_handler;
NetsnmpCacheLoad hrStorageTable_load;
NetsnmpCacheFree hrStorageTable_free;
 /*
     * Typical data structure for a row entry
 */
struct hrStorageTable_entry {
    /*
    * Index values
    */
    long            hrStorageIndex;
    /*
    * Column values
    */
    long            hrStorageIndex_I;
    oid             hrStorageType[API_MAX_IOD_LENGTH];
    size_t          hrStorageType_len;
    char            hrStorageDescr[API_CHRMAX];
    size_t          hrStorageDescr_len;
    long            hrStorageAllocationUnits;
    long            hrStorageSize;
    long            old_hrStorageSize;
    long            hrStorageUsed;
    u_long          hrStorageAllocationFailures;
    int             valid;
};
int hrStorageTable_createEntry_allData(netsnmp_tdata * table_data, struct hrStorageTable_entry *hrSotageE);

#define HRSTORAGETABLE_TIMEOUT  2
void            initialize_table_hrDeviceTable(void);
Netsnmp_Node_Handler hrDeviceTable_handler;
NetsnmpCacheLoad hrDeviceTable_load;
NetsnmpCacheFree hrDeviceTable_free;
/*
     * Typical data structure for a row entry
     */
struct hrDeviceTable_entry {
    /*
    * Index values
    */
    long            hrDeviceIndex;

    /*
    * Column values
    */
    long            hrDeviceIndex_I;
    oid             hrDeviceType[API_MAX_IOD_LENGTH];
    size_t          hrDeviceType_len;
    char            hrDeviceDescr[API_CHRMAX];
    size_t          hrDeviceDescr_len;
    oid             hrDeviceID[API_MAX_IOD_LENGTH];
    size_t          hrDeviceID_len;
    long            hrDeviceStatus;
    u_long          hrDeviceErrors;

    int             valid;
};
int hrDeviceTable_createEntry_allData(netsnmp_tdata * table_data, struct hrDeviceTable_entry *hrSotageE);
#define HRDEVICETABLE_TIMEOUT  2
void            initialize_table_hrProcessorTable(void);
Netsnmp_Node_Handler hrProcessorTable_handler;
NetsnmpCacheLoad hrProcessorTable_load;
NetsnmpCacheFree hrProcessorTable_free;
 /*
     * Typical data structure for a row entry
 */
struct hrProcessorTable_entry {
    /*
    * Index values
    */
    long            hrDeviceIndex;

    /*
    * Column values
    */
    oid             hrProcessorFrwID[API_MAX_IOD_LENGTH];
    size_t          hrProcessorFrwID_len;
    long            hrProcessorLoad;

    int             valid;
};
int hrProcessorTable_createEntry_allData(netsnmp_tdata * table_data, struct hrProcessorTable_entry *hrProcessE);
#define HRPROCESSORTABLE_TIMEOUT  2
void            initialize_table_hrNetworkTable(void);
Netsnmp_Node_Handler hrNetworkTable_handler;
NetsnmpCacheLoad hrNetworkTable_load;
NetsnmpCacheFree hrNetworkTable_free;
#define HRNETWORKTABLE_TIMEOUT  2
void            initialize_table_hrPrinterTable(void);
Netsnmp_Node_Handler hrPrinterTable_handler;
NetsnmpCacheLoad hrPrinterTable_load;
NetsnmpCacheFree hrPrinterTable_free;
#define HRPRINTERTABLE_TIMEOUT  2
void            initialize_table_hrDiskStorageTable(void);
Netsnmp_Node_Handler hrDiskStorageTable_handler;
NetsnmpCacheLoad hrDiskStorageTable_load;
NetsnmpCacheFree hrDiskStorageTable_free;
#define HRDISKSTORAGETABLE_TIMEOUT  2
void            initialize_table_hrPartitionTable(void);
Netsnmp_Node_Handler hrPartitionTable_handler;
NetsnmpCacheLoad hrPartitionTable_load;
NetsnmpCacheFree hrPartitionTable_free;
#define HRPARTITIONTABLE_TIMEOUT  2
void            initialize_table_hrFSTable(void);
Netsnmp_Node_Handler hrFSTable_handler;
NetsnmpCacheLoad hrFSTable_load;
NetsnmpCacheFree hrFSTable_free;
#define HRFSTABLE_TIMEOUT  2
void            initialize_table_hrSWRunTable(void);
Netsnmp_Node_Handler hrSWRunTable_handler;
NetsnmpCacheLoad hrSWRunTable_load;
NetsnmpCacheFree hrSWRunTable_free;
 /*
     * Typical data structure for a row entry
 */
struct hrSWRunTable_entry {
    /*
    * Index values
    */
    long            hrSWRunIndex;

    /*
    * Column values
    */
    long            hrSWRunIndex_I;
    char            hrSWRunName[API_CHRMAX];
    size_t          hrSWRunName_len;
    oid             hrSWRunID[API_MAX_IOD_LENGTH];
    size_t          hrSWRunID_len;
    char            hrSWRunPath[API_CHRMAX];
    size_t          hrSWRunPath_len;
    char            hrSWRunParameters[API_CHRMAX];
    size_t          hrSWRunParameters_len;
    long            hrSWRunType;
    long            hrSWRunStatus;
    long            old_hrSWRunStatus;

    int             valid;
};
int hrSWRunTable_createEntry_allData(netsnmp_tdata * table_data, struct hrSWRunTable_entry *hrSWRunE);

#define HRSWRUNTABLE_TIMEOUT  2
void            initialize_table_hrSWRunPerfTable(void);
Netsnmp_Node_Handler hrSWRunPerfTable_handler;
NetsnmpCacheLoad hrSWRunPerfTable_load;
NetsnmpCacheFree hrSWRunPerfTable_free;
  /*
     * Typical data structure for a row entry
  */
struct hrSWRunPerfTable_entry {
    /*
    * Index values
    */
    long            hrSWRunIndex;

    /*
    * Column values
    */
    long            hrSWRunPerfCPU;
    long            hrSWRunPerfMem;

    int             valid;
};
int hrSWRunPerfTable_createEntry_allData(netsnmp_tdata * table_data, struct hrSWRunPerfTable_entry *hrSWRunPerfE);
#define HRSWRUNPERFTABLE_TIMEOUT  2
void            initialize_table_hrSWInstalledTable(void);
Netsnmp_Node_Handler hrSWInstalledTable_handler;
NetsnmpCacheLoad hrSWInstalledTable_load;
NetsnmpCacheFree hrSWInstalledTable_free;
#define HRSWINSTALLEDTABLE_TIMEOUT  2

/*
 * column number definitions for table hrStorageTable
 */
#define COLUMN_HRSTORAGEINDEX        1
#define COLUMN_HRSTORAGETYPE        2
#define COLUMN_HRSTORAGEDESCR        3
#define COLUMN_HRSTORAGEALLOCATIONUNITS        4
#define COLUMN_HRSTORAGESIZE        5
#define COLUMN_HRSTORAGEUSED        6
#define COLUMN_HRSTORAGEALLOCATIONFAILURES        7

/*
 * column number definitions for table hrDeviceTable
 */
#define COLUMN_HRDEVICEINDEX        1
#define COLUMN_HRDEVICETYPE        2
#define COLUMN_HRDEVICEDESCR        3
#define COLUMN_HRDEVICEID        4
#define COLUMN_HRDEVICESTATUS        5
#define COLUMN_HRDEVICEERRORS        6

/*
 * column number definitions for table hrProcessorTable
 */
#define COLUMN_HRPROCESSORFRWID        1
#define COLUMN_HRPROCESSORLOAD        2

/*
 * column number definitions for table hrNetworkTable
 */
#define COLUMN_HRNETWORKIFINDEX        1

/*
 * column number definitions for table hrPrinterTable
 */
#define COLUMN_HRPRINTERSTATUS        1
#define COLUMN_HRPRINTERDETECTEDERRORSTATE        2

/*
 * column number definitions for table hrDiskStorageTable
 */
#define COLUMN_HRDISKSTORAGEACCESS        1
#define COLUMN_HRDISKSTORAGEMEDIA        2
#define COLUMN_HRDISKSTORAGEREMOVEBLE        3
#define COLUMN_HRDISKSTORAGECAPACITY        4

/*
 * column number definitions for table hrPartitionTable
 */
#define COLUMN_HRPARTITIONINDEX        1
#define COLUMN_HRPARTITIONLABEL        2
#define COLUMN_HRPARTITIONID        3
#define COLUMN_HRPARTITIONSIZE        4
#define COLUMN_HRPARTITIONFSINDEX        5

/*
 * column number definitions for table hrFSTable
 */
#define COLUMN_HRFSINDEX        1
#define COLUMN_HRFSMOUNTPOINT        2
#define COLUMN_HRFSREMOTEMOUNTPOINT        3
#define COLUMN_HRFSTYPE        4
#define COLUMN_HRFSACCESS        5
#define COLUMN_HRFSBOOTABLE        6
#define COLUMN_HRFSSTORAGEINDEX        7
#define COLUMN_HRFSLASTFULLBACKUPDATE        8
#define COLUMN_HRFSLASTPARTIALBACKUPDATE        9

/*
 * column number definitions for table hrSWRunTable
 */
#define COLUMN_HRSWRUNINDEX        1
#define COLUMN_HRSWRUNNAME        2
#define COLUMN_HRSWRUNID        3
#define COLUMN_HRSWRUNPATH        4
#define COLUMN_HRSWRUNPARAMETERS        5
#define COLUMN_HRSWRUNTYPE        6
#define COLUMN_HRSWRUNSTATUS        7

/*
 * column number definitions for table hrSWRunPerfTable
 */
#define COLUMN_HRSWRUNPERFCPU        1
#define COLUMN_HRSWRUNPERFMEM        2

/*
 * column number definitions for table hrSWInstalledTable
 */
#define COLUMN_HRSWINSTALLEDINDEX        1
#define COLUMN_HRSWINSTALLEDNAME        2
#define COLUMN_HRSWINSTALLEDID        3
#define COLUMN_HRSWINSTALLEDTYPE        4
#define COLUMN_HRSWINSTALLEDDATE        5


/**   HOST-RESOURCES-TYPES  **/

static oid HR_StorageTypes[10][10] = {
    {  1, 3, 6, 1, 2, 1, 25, 2, 1,1  }, // hrStorageOther
    {  1, 3, 6, 1, 2, 1, 25, 2, 1,2 },// hrStorageRam
    {  1, 3, 6, 1, 2, 1, 25, 2, 1,3  },// hrStorageVirtualMemory
    {  1, 3, 6, 1, 2, 1, 25, 2, 1,4  },  //hrStorageFixedDisk
    {  1, 3, 6, 1, 2, 1, 25, 2, 1,5  },  // hrStorageRemovableDisk
    {  1, 3, 6, 1, 2, 1, 25, 2, 1,6  },  // hrStorageFloppyDisk
    {  1, 3, 6, 1, 2, 1, 25, 2, 1,7 },  // hrStorageCompactDisc
    {  1, 3, 6, 1, 2, 1, 25, 2, 1,8  },  // hrStorageRamDisk
    {  1, 3, 6, 1, 2, 1, 25, 2, 1,9  },  //hrStorageFlashMemory
    {  1, 3, 6, 1, 2, 1, 25, 2, 1,10  }  // hrStorageNetworkDisk
};
typedef enum {
    hrStorageOther,
    hrStorageRam,
    hrStorageVirtualMemory,
    hrStorageFixedDisk,
    hrStorageRemovableDisk,
    hrStorageFloppyDisk,
    hrStorageCompactDisc,
    hrStorageRamDisk,
    hrStorageFlashMemory,
    hrStorageNetworkDisk
}HrSType_t;

static oid HR_DeviceTypes[20][10] = {
    {  1, 3, 6, 1, 2, 1, 25, 3, 1,1  }, // hrDeviceOther
    {  1, 3, 6, 1, 2, 1, 25, 3, 1,2  },// hrDeviceUnknown
    {  1, 3, 6, 1, 2, 1, 25, 3, 1,3  },// hrDeviceProcessor
    {  1, 3, 6, 1, 2, 1, 25, 3, 1,4  },  //hrDeviceNetwork
    {  1, 3, 6, 1, 2, 1, 25, 3, 1,5  },  // hrDevicePrinter
    {  1, 3, 6, 1, 2, 1, 25, 3, 1,6  },  // hrDeviceDiskStorage
    {  1, 3, 6, 1, 2, 1, 25, 3, 1,10  },  // hrDeviceVideo
    {  1, 3, 6, 1, 2, 1, 25, 3, 1,11  },  // hrDeviceAudio
    {  1, 3, 6, 1, 2, 1, 25, 3, 1,12  },  //hrDeviceCoprocessor
    {  1, 3, 6, 1, 2, 1, 25, 3, 1,13 },  // hrDeviceKeyboard
    {  1, 3, 6, 1, 2, 1, 25, 3, 1,14 },  // hrDeviceModem
    {  1, 3, 6, 1, 2, 1, 25, 3, 1,15 },  // hrDeviceParallelPort
    {  1, 3, 6, 1, 2, 1, 25, 3, 1,16 },  // hrDevicePointing
    {  1, 3, 6, 1, 2, 1, 25, 3, 1,17 },  // hrDeviceSerialPort
    {  1, 3, 6, 1, 2, 1, 25, 3, 1,18 },  // hrDeviceTape
    {  1, 3, 6, 1, 2, 1, 25, 3, 1,19 },  // hrDeviceClock
    {  1, 3, 6, 1, 2, 1, 25, 3, 1,20 },  // hrDeviceVolatileMemory
    {  1, 3, 6, 1, 2, 1, 25, 3, 1,20 }  //hrDeviceNonVolatileMemory
};

typedef enum{
     hrDevice_Other,
     hrDeviceUnknown,
     hrDeviceProcessor,
     hrDeviceNetwork,
     hrDevicePrinter,
     hrDeviceDiskStorage,
     hrDeviceVideo,
     hrDeviceAudio,
     hrDeviceCoprocessor,
     hrDeviceKeyboard,
     hrDeviceModem,
     hrDeviceParallelPort,
     hrDevicePointing,
     hrDeviceSerialPort,
     hrDeviceTape,
     hrDeviceClock,
     hrDeviceVolatileMemory,
     hrDeviceNonVolatileMemory,
}HrDType_t;


#endif                          /* HOST_H */