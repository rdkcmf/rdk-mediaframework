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



#ifndef __VL_SNMP_CLIENT_H__
#define __VL_SNMP_CLIENT_H__

#include "snmp_types.h"

#ifdef __cplusplus
extern "C" {
#endif
    
typedef enum _VL_SNMP_CLIENT_VALUE_TYPE
{
    VL_SNMP_CLIENT_VALUE_TYPE_NONE          = 0x0,
    VL_SNMP_CLIENT_VALUE_TYPE_INTEGER       = 0x2,
    VL_SNMP_CLIENT_VALUE_TYPE_BYTE          = 0x4,
    
    VL_SNMP_CLIENT_VALUE_TYPE_INVALID = 0x7FFFFFFF
}VL_SNMP_CLIENT_VALUE_TYPE;

typedef enum _VL_SNMP_CLIENT_REQUEST_TYPE
{
    VL_SNMP_CLIENT_REQUEST_TYPE_ASYNCHRONOUS = 0x0,
    VL_SNMP_CLIENT_REQUEST_TYPE_SYNCHRONOUS  = 0x1,
    VL_SNMP_CLIENT_REQUEST_TYPE_TRAP         = 0x0,
    VL_SNMP_CLIENT_REQUEST_TYPE_INFORM       = 0x1,
    
    VL_SNMP_CLIENT_REQUEST_TYPE_INVALID = 0x7FFFFFFF
}VL_SNMP_CLIENT_REQUEST_TYPE;

typedef struct _VL_SNMP_CLIENT_VALUE
{
    const char * pszOID;
    VL_SNMP_CLIENT_VALUE_TYPE eValueType;
    
    int nValues;
    int nValueCapacity;
    // Each member of the following union...
    // occupies no more bytes than sizeof(void*).
    union
    {
        void * pVoid;
        const char * pszValue;
        unsigned char * paBytValue;
        long * palValue;
        unsigned long * paulValue;
    }Value;
    
    int nResultCapacity;
    char * pszResult;
    int eSnmpObjectError;
    
    int nOidNameLength;
    int nOidNameCapacity;
    unsigned long * palOidNameOid;
    
}VL_SNMP_CLIENT_VALUE;

#define VL_SNMP_CLIENT_VALUE_ENTRY(eValueType, nValueCapacity, paValues, nResultCapacity, pszResult) \
    {NULL, (eValueType), 0, (nValueCapacity), (paValues), (nResultCapacity), (pszResult), 0, 0, 0, NULL}

#define VL_SNMP_CLIENT_INTEGER_ENTRY(pLong) \
    VL_SNMP_CLIENT_VALUE_ENTRY(VL_SNMP_CLIENT_VALUE_TYPE_INTEGER, 1, (pLong), 0, NULL)

#define VL_SNMP_CLIENT_BYTE_ARRAY_ENTRY(nValueCapacity, paValues) \
    VL_SNMP_CLIENT_VALUE_ENTRY(VL_SNMP_CLIENT_VALUE_TYPE_BYTE, (nValueCapacity), (paValues), 0, NULL)

#define VL_SNMP_CLIENT_STRING_ENTRY(nValueCapacity, paValues) \
    VL_SNMP_CLIENT_BYTE_ARRAY_ENTRY((nValueCapacity), (paValues))

#define VL_SNMP_CLIENT_DUMP_ENTRY(nResultCapacity, pszResult) \
    VL_SNMP_CLIENT_VALUE_ENTRY(VL_SNMP_CLIENT_VALUE_TYPE_NONE, 0, NULL, (nResultCapacity), (pszResult))

#define VL_SNMP_CLIENT_NULL_ENTRY() \
    VL_SNMP_CLIENT_VALUE_ENTRY(VL_SNMP_CLIENT_VALUE_TYPE_NONE, 0, NULL, 0, NULL)

#define VL_SNMP_CLIENT_MULTI_VAR_ENTRY(pszOID, eValueType, nValueCapacity, paValues) \
    {pszOID, (eValueType), 0, (nValueCapacity), (paValues), 0, NULL, 0, 0, 0, NULL}

#define VL_SNMP_CLIENT_MULTI_SET_ASN_ENTRY(pszOID, eValueType, paValues, nSize) \
    {pszOID, (VL_SNMP_CLIENT_VALUE_TYPE)(eValueType), (nSize), 0, (paValues), 0, NULL, 0, 0, 0, NULL}

#define VL_SNMP_CLIENT_MULTI_SET_STR_ENTRY(pszOID, eValueType, pszValue) \
    {pszOID, (VL_SNMP_CLIENT_VALUE_TYPE)(eValueType), 0, 0, (pszValue), 0, NULL, 0, 0, 0, NULL}
    
#define VL_SNMP_CLIENT_OID_NAME_INT_VAL_ENTRY(pLong, nOidNameCapacity, palOidNameOid) \
    {NULL, VL_SNMP_CLIENT_VALUE_TYPE_INTEGER, 0, 1, (pLong), 0, NULL, 0, 0, nOidNameCapacity, palOidNameOid}

VL_SNMP_API_RESULT vlSnmpGetPublicLong(char * pszIpAddress, char * pszOID, unsigned long * pLong);
VL_SNMP_API_RESULT vlSnmpGetLocalLong(char * pszOID, unsigned long * pLong);
VL_SNMP_API_RESULT vlSnmpGetEcmLong(char * pszOID, unsigned long * pLong);

VL_SNMP_API_RESULT vlSnmpGetPublicByteArray(char * pszIpAddress, char * pszOID, int nCapacity, unsigned char * paBytArray);
VL_SNMP_API_RESULT vlSnmpGetLocalByteArray(char * pszOID, int nCapacity, unsigned char * paBytArray);
VL_SNMP_API_RESULT vlSnmpGetEcmByteArray(char * pszOID, int nCapacity, unsigned char * paBytArray);

VL_SNMP_API_RESULT vlSnmpGetPublicString(char * pszIpAddress, char * pszOID, int nCapacity, char * pszString);
VL_SNMP_API_RESULT vlSnmpGetLocalString(char * pszOID, int nCapacity, char * pszString);
VL_SNMP_API_RESULT vlSnmpGetEcmString(char * pszOID, int nCapacity, char * pszString);

VL_SNMP_API_RESULT vlSnmpGetPublicStringDump(char * pszIpAddress, char * pszOID, int nResultCapacity, char * pszResult);
VL_SNMP_API_RESULT vlSnmpGetLocalStringDump(char * pszOID, int nResultCapacity, char * pszResult);
VL_SNMP_API_RESULT vlSnmpGetEcmStringDump(char * pszOID, int nResultCapacity, char * pszResult);
        
VL_SNMP_API_RESULT vlSnmpGetPublicObjects(char * pszIpAddress, char * pszOID, int * pnResults, VL_SNMP_CLIENT_VALUE * paResults);
VL_SNMP_API_RESULT vlSnmpGetLocalObjects(char * pszOID, int * pnResults, VL_SNMP_CLIENT_VALUE * paResults);
VL_SNMP_API_RESULT vlSnmpGetEcmObjects(char * pszOID, int * pnResults, VL_SNMP_CLIENT_VALUE * paResults);

VL_SNMP_API_RESULT vlSnmpGetObjects(char * pszCommunity, char * pszIpAddress, char * pszOID, int * pnResults, VL_SNMP_CLIENT_VALUE * paResults);
VL_SNMP_API_RESULT vlSnmpMultiGetObjects(char * pszCommunity, char * pszIpAddress, int * pnResults, VL_SNMP_CLIENT_VALUE * paResults);

VL_SNMP_API_RESULT vlSnmpSetPublicLong(char * pszIpAddress, char * pszOID, unsigned long nLong, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType);
VL_SNMP_API_RESULT vlSnmpSetLocalLong(char * pszOID, unsigned long nLong, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType);
VL_SNMP_API_RESULT vlSnmpSetEcmLong(char * pszOID, unsigned long nLong, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType);

VL_SNMP_API_RESULT vlSnmpSetPublicByteArray(char * pszIpAddress, char * pszOID, int nBytes, unsigned char * paBytArray, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType);
VL_SNMP_API_RESULT vlSnmpSetLocalByteArray(char * pszOID, int nBytes, unsigned char * paBytArray, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType);
VL_SNMP_API_RESULT vlSnmpSetEcmByteArray(char * pszOID, int nBytes, unsigned char * paBytArray, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType);

VL_SNMP_API_RESULT vlSnmpSetPublicString(char * pszIpAddress, char * pszOID, char * pszString, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType);
VL_SNMP_API_RESULT vlSnmpSetLocalString(char * pszOID, char * pszString, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType);
VL_SNMP_API_RESULT vlSnmpSetEcmString(char * pszOID, char * pszString, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType);

VL_SNMP_API_RESULT vlSnmpSetPublicStrVar(char * pszIpAddress, char * pszOID, char chValueType, char * pszValue, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType);
VL_SNMP_API_RESULT vlSnmpSetLocalStrVar(char * pszOID, char chValueType, char * pszValue, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType);
VL_SNMP_API_RESULT vlSnmpSetEcmStrVar(char * pszOID, char chValueType, char * pszValue, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType);

VL_SNMP_API_RESULT vlSnmpSetPublicAsnVar(char * pszIpAddress, char * pszOID, char asnValueType, void * pValue, int nBytes, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType);
VL_SNMP_API_RESULT vlSnmpSetLocalAsnVar(char * pszOID, char asnValueType, void * pValue, int nBytes, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType);
VL_SNMP_API_RESULT vlSnmpSetEcmAsnVar(char * pszOID, char asnValueType, void * pValue, int nBytes, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType);

VL_SNMP_API_RESULT vlSnmpSetPublicObjects(char * pszIpAddress, int * pnResults, VL_SNMP_CLIENT_VALUE * paResults, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType);
VL_SNMP_API_RESULT vlSnmpSetLocalObjects(int * pnResults, VL_SNMP_CLIENT_VALUE * paResults, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType);
VL_SNMP_API_RESULT vlSnmpSetEcmObjects(int * pnResults, VL_SNMP_CLIENT_VALUE * paResults, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType);

VL_SNMP_API_RESULT vlSnmpMultiSetObjects(char * pszCommunity, char * pszIpAddress, int * pnResults, VL_SNMP_CLIENT_VALUE * paResults, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType);

VL_SNMP_API_RESULT vlSnmpSendNotificationComplex(
        const char * pszCommunity, const char * pszIpAddress, const char * pszTrapOid,
        int nRemotePort, int nTimeout, int nRetries,
        int * pnResults, VL_SNMP_CLIENT_VALUE * paResults,
        VL_SNMP_CLIENT_REQUEST_TYPE eRequestType);

VL_SNMP_API_RESULT vlSnmpSendNotificationSimple(
        const char * pszCommunity, const char * pszIpAddress, const char * pszTrapOid,
        int * pnResults, VL_SNMP_CLIENT_VALUE * paResults,
        VL_SNMP_CLIENT_REQUEST_TYPE eRequestType);

int vlSnmpConvertOidToString(const int * paOid, int nOid, char * pszString, int nStrCapacity);
unsigned long * vlSnmpConvertStringToOid(const char * pszString, unsigned long * paOid, int * pnOidCapacity);

const char * vlSnmpGetErrorString(VL_SNMP_API_RESULT eResult);
        
#ifdef __cplusplus
}
#endif

#endif // __VL_SNMP_CLIENT_H__
