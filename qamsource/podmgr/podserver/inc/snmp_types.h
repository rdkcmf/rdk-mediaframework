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

 

#ifndef __VL_SNMP_TYPES_H__
#define __VL_SNMP_TYPES_H__
/*-----------------------------------------------------------------*/
#define VL_SNMP_STR_SIZE         128

typedef struct _VL_SNMP_CERTIFICATE_INFO
{
    char szDispString[VL_SNMP_STR_SIZE];      //e.g. "xyz Certificate Info" e.g "1394 Certificate Info"
    char szKeyInfoString[VL_SNMP_STR_SIZE];   // usually empty
    char szMfrName[VL_SNMP_STR_SIZE];
    char szModelName[VL_SNMP_STR_SIZE];
    unsigned char acModelId[4];
    unsigned char acVendorId[3];
    unsigned char acDeviceId[5];
    unsigned char acGUIDorMACADDR[8];
    unsigned char bCertAvailable;
    unsigned char bCertValid;
    unsigned char bVerifiedWithChain;
    unsigned char bIsProduction;

}VL_SNMP_CERTIFICATE_INFO;

typedef enum _VL_SNMP_HOST_POWER_STATUS
{
    VL_SNMP_HOST_POWER_STATUS_POWERON = 1,
    VL_SNMP_HOST_POWER_STATUS_STANDBY = 2,

} VL_SNMP_HOST_POWER_STATUS;

typedef enum _VL_SNMP_HostMibMemoryType
{
    VL_SNMP_HOST_MIB_MEMORY_TYPE_ROM =1,
    VL_SNMP_HOST_MIB_MEMORY_TYPE_DRAM,
    VL_SNMP_HOST_MIB_MEMORY_TYPE_SRAM,
    VL_SNMP_HOST_MIB_MEMORY_TYPE_FLASH,
    VL_SNMP_HOST_MIB_MEMORY_TYPE_NVM,

    VL_SNMP_HOST_MIB_MEMORY_TYPE_VIDEOMEMORY = 7,
    VL_SNMP_HOST_MIB_MEMORY_TYPE_OTHERMEMORY,
    VL_SNMP_HOST_MIB_MEMORY_TYPE_RESERVED,
    VL_SNMP_HOST_MIB_MEMORY_TYPE_INTERNAL_HARDDRIVE,
    VL_SNMP_HOST_MIB_MEMORY_TYPE_EXTERNAL_HARDDRIVE,
    VL_SNMP_HOST_MIB_MEMORY_TYPE_OPTICALMEDIA,
    VL_SNMP_HOST_MIB_MEMORY_TYPE_MAX

}VL_SNMP_HostMibMemoryType;

typedef struct __VL_SNMP_hostMemoryInfo
{
    unsigned int largestAvailableBlock; //In KiloBytes
    unsigned int totalVideoMemory;      //In KiloBytes
    unsigned int availableVideoMemory;  //In KiloBytes
}VL_SNMP_HostMemoryInfo;

typedef struct __VL_SNMP_systemMemoryReportEntry
{
    VL_SNMP_HostMibMemoryType     memoryType;
    unsigned int                  memSize;    //In KiloBytes

}VL_SNMP_SystemMemoryReportEntry;

typedef struct __VL_SNMP_systemMemoryReportTable
{
    unsigned int                    nEntries;
    VL_SNMP_SystemMemoryReportEntry aEntries[VL_SNMP_HOST_MIB_MEMORY_TYPE_MAX];

}VL_SNMP_SystemMemoryReportTable;

typedef enum tag_VL_SNMP_PANIC_REPORT_TYPE
{
    VL_SNMP_PANIC_REPORT_TYPE_EXCEPTION = 1,
    VL_SNMP_PANIC_REPORT_TYPE_FORCED    = 2,
}VL_SNMP_PANIC_REPORT_TYPE;

typedef struct _VL_SNMP_Panic_Dump_Report
{
    VL_SNMP_PANIC_REPORT_TYPE   eReportType;
    unsigned int                nReportBytes;
    unsigned char *             pReportBytes;

}VL_SNMP_Panic_Dump_Report;

#define VL_SNMP_MODULE_REQUEST(module, req)  ((module) | (req))

typedef enum tag_VL_SNMP_MODULE
{
    VL_SNMP_MODULE_SYS              = 0x00000000,
    VL_SNMP_MODULE_PLATFORM         = 0x00010000,

    VL_SNMP_MODULE_INVALID          = 0x7FFF0000
}VL_SNMP_MODULE;

typedef enum tag_VL_SNMP_REQUEST
{
    VL_SNMP_REQUEST_GET_SYS_HOST_MEMORY_INFO          =   VL_SNMP_MODULE_REQUEST(VL_SNMP_MODULE_SYS, 0x0000),
    VL_SNMP_REQUEST_GET_SYS_SYSTEM_MEMORY_REPORT      =   VL_SNMP_MODULE_REQUEST(VL_SNMP_MODULE_SYS, 0x0001),
    VL_SNMP_REQUEST_GET_SYS_SYSTEM_POWER_STATE        =   VL_SNMP_MODULE_REQUEST(VL_SNMP_MODULE_SYS, 0x0002),

    VL_SNMP_REQUEST_SET_SYS_UNINITIALIZE              =   VL_SNMP_MODULE_REQUEST(VL_SNMP_MODULE_SYS, 0x8000),

    VL_SNMP_REQUEST_INVALID                           =   VL_SNMP_MODULE_REQUEST(VL_SNMP_MODULE_INVALID, 0xFFFF)
}VL_SNMP_REQUEST;

typedef enum _VL_SNMP_API_RESULT
{
    VL_SNMP_API_RESULT_SUCCESS                  = 0,
    VL_SNMP_API_RESULT_ERR_TOOBIG               = 1,
    VL_SNMP_API_RESULT_ERR_NOSUCHNAME           = 2,
    VL_SNMP_API_RESULT_ERR_BADVALUE             = 3,
    VL_SNMP_API_RESULT_ERR_READONLY             = 4,
    VL_SNMP_API_RESULT_ERR_GENERR               = 5,

    VL_SNMP_API_RESULT_ERR_NOACCESS             = 6,
    VL_SNMP_API_RESULT_ERR_WRONGTYPE            = 7,
    VL_SNMP_API_RESULT_ERR_WRONGLENGTH          = 8,
    VL_SNMP_API_RESULT_ERR_WRONGENCODING        = 9,
    VL_SNMP_API_RESULT_ERR_WRONGVALUE           = 10,
    VL_SNMP_API_RESULT_ERR_NOCREATION           = 11,
    VL_SNMP_API_RESULT_ERR_INCONSISTENTVALUE    = 12,
    VL_SNMP_API_RESULT_ERR_RESOURCEUNAVAILABLE  = 13,
    VL_SNMP_API_RESULT_ERR_COMMITFAILED         = 14,
    VL_SNMP_API_RESULT_ERR_UNDOFAILED           = 15,
    VL_SNMP_API_RESULT_ERR_AUTHORIZATIONERROR   = 16,
    VL_SNMP_API_RESULT_ERR_NOTWRITABLE          = 17,
    VL_SNMP_API_RESULT_ERR_INCONSISTENTNAME     = 18,
    
    VL_SNMP_API_RESULT_ERR_NOSUCHOBJECT         = 128,
    VL_SNMP_API_RESULT_ERR_NOSUCHINSTANCE       = 129,
    VL_SNMP_API_RESULT_ERR_ENDOFMIBVIEW         = 130,
    
    VL_SNMP_API_RESULT_FAILED                   = 1000000,
    VL_SNMP_API_RESULT_CHECK_ERRNO              = 1000001,
    VL_SNMP_API_RESULT_UNSPECIFIED_ERROR        = 1000002,
    VL_SNMP_API_RESULT_ACCESS_DENIED            = 1000003,
    VL_SNMP_API_RESULT_NOT_IMPLEMENTED          = 1000004,
    VL_SNMP_API_RESULT_NOT_EXISTING             = 1000005,
    VL_SNMP_API_RESULT_NULL_PARAM               = 1000006,
    VL_SNMP_API_RESULT_INVALID_PARAM            = 1000007,
    VL_SNMP_API_RESULT_OUT_OF_RANGE             = 1000008,
    VL_SNMP_API_RESULT_OPEN_FAILED              = 1000009,
    VL_SNMP_API_RESULT_READ_FAILED              = 1000010,
    VL_SNMP_API_RESULT_WRITE_FAILED             = 1000011,
    VL_SNMP_API_RESULT_MALLOC_FAILED            = 1000012,
    VL_SNMP_API_RESULT_TIMEOUT                  = 1000013,
    VL_SNMP_API_RESULT_INFINITE_LOOP            = 1000014,
    VL_SNMP_API_RESULT_INVALID_OID              = 1000015,
    VL_SNMP_API_RESULT_GET_ON_NODE              = 1000016,
    VL_SNMP_API_RESULT_PROTOCOL_ERROR           = 1000017,
    VL_SNMP_API_RESULT_BUFFER_OVERFLOW          = 1000018,
    VL_SNMP_API_RESULT_OID_MISMATCH             = 1000019,

    VL_SNMP_API_RESULT_INVALID                  = 0x7FFFFFFF
}VL_SNMP_API_RESULT;



/*-----------------------------------------------------------------*/
#endif // __VL_SNMP_TYPES_H__
/*-----------------------------------------------------------------*/
