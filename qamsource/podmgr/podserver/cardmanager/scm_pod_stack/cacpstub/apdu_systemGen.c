/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2014 RDK Management
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

#include <stddef.h>

typedef unsigned char  UCHAR;
typedef unsigned short USHORT;
typedef unsigned int uint32;

unsigned char * SysCtlGetVendorId(void)
{
	return NULL;
}
unsigned char * SysCtlGetHWVerId(void)
{
	return NULL;
}
unsigned char * SysCtlGetCodeFileName(int *size)
{
	return NULL;
}
unsigned char * SysCtlGetHostId(void)
{
	return NULL;
}
unsigned char * SysCtlGetMACAddress()
{
	return NULL;
}
int SysCtlReadVendorData()
{
    return 0;
}

extern "C" int  APDU_HostInfoReq (USHORT sesnum, UCHAR * apkt, USHORT len)
{
    return 0;
}

extern "C" int  APDU_HostInfoResponse (USHORT sesnum, UCHAR * apkt, USHORT len)
{
    return 1;
}

UCHAR CVCCheck(UCHAR *pData, uint32 uLen,UCHAR CVTNum)
{
	return 0;
}
UCHAR CVT_T1V1(UCHAR *pCVT,USHORT sLength)
{
	return 0;
}
UCHAR CVT_T2V1(UCHAR *pCVT,USHORT sLength)
{
	return 0;
}
void vlSystemFreeMem(void *pData)
{
	return;
}

extern "C" int  APDU_CodeVerTable (USHORT sesnum, UCHAR * packet, USHORT len)
{
	return 1;
}

extern "C" int  APDU_CodeVerTableReply (USHORT sesnum, UCHAR * apkt, USHORT len)
{
    return 1;
}

extern "C" int  APDU_HostDownloadCtl (USHORT sesnum, int  cmd, USHORT len)
{  
    return 1;
}

