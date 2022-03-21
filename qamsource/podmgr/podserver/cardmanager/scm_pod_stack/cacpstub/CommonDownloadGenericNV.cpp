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

#include <CommonDownloadNVDataAccess.h>
#include <stddef.h>
#include <cdl_mgr_types.h>

#define UNUSED_VAR(v) ((void)v)

int CommonDownloadGetCLCvcCA(unsigned char **pData, unsigned long *pSize)
{
	UNUSED_VAR(pData);
	UNUSED_VAR(pSize);
	return 0;
}

int CommonDownloadGetCLCvcRootCA(unsigned char **pData, unsigned long *pSize)
{
	UNUSED_VAR(pData);
	UNUSED_VAR(pSize);
	return 0;
}

int CommonDownloadGetCodeAccessStartTime(CodeAccessStart_t *pCodeAccessTime,CVCType_e cvcType)
{
	UNUSED_VAR(pCodeAccessTime);
	UNUSED_VAR(cvcType);
	return 0;
}

unsigned char *CommonDownloadGetCosignerName(void)
{
	return NULL;
}

int CommonDownloadGetCvcAccessStartTime(cvcAccessStart_t *pCodeAccessTime,CVCType_e cvcType)
{
	UNUSED_VAR(pCodeAccessTime);
	UNUSED_VAR(cvcType);
	return 0;
}

int CommonDownloadGetCvcCAPublicKey(unsigned char **pData, unsigned long *pSize)
{
	UNUSED_VAR(pData);
	UNUSED_VAR(pSize);
	return 0;
}

int CommonDownloadGetManCvc(unsigned char **pData, unsigned long *pSize)
{
	UNUSED_VAR(pData);
	UNUSED_VAR(pSize);
	return 0;
}

unsigned char *CommonDownloadGetManufacturerName(void)
{
	return NULL;
}

int CommonDownloadGetVendorId(unsigned char **pData, unsigned long *pSize)
{
	UNUSED_VAR(pData);
	UNUSED_VAR(pSize);
	return 0;
}

int CommonDownLClearCodeAccessUpgrTime()
{
	return 0;
}

int CommonDownloadGetCDLTftpParams(unsigned char **pData, unsigned long *Size)
{
	UNUSED_VAR(pData);
	UNUSED_VAR(Size);
    return COMM_DL_SUCESS;
}

int CommonDownloadSetCDLTftpParams(unsigned char *pData, unsigned long Size)
{
	UNUSED_VAR(pData);
	UNUSED_VAR(Size);
    return COMM_DL_SUCESS;
}

int CommonDownloadNvRamDataInit()
{
    return 0;
}

int CommonDownloadGetCodeFileName(unsigned char **pData, unsigned long *pSize)
{
	UNUSED_VAR(pData);
	UNUSED_VAR(pSize);
    return 0;	
}

int CommonDownloadGetCodeFileNameCvt2v2(unsigned char **pData, unsigned long *pSize,cvt2v2_object_type_e objType )
{
	UNUSED_VAR(pData);
	UNUSED_VAR(pSize);
	UNUSED_VAR(objType);
    return 0;	
}

int CommonDownloadSetCLCvcCA(unsigned char *pData, unsigned long Size)
{	
	UNUSED_VAR(pData);
	UNUSED_VAR(Size);
    return COMM_DL_SUCESS;
}

int CommonDownloadSetCLCvcRootCA(unsigned char *pData, unsigned long Size)
{	
	UNUSED_VAR(pData);
	UNUSED_VAR(Size);
    return COMM_DL_SUCESS;
}

int CommonDownloadSetCodeAccessStartTime(CodeAccessStart_t *pCodeAccessTime,CVCType_e cvcType)
{	
	UNUSED_VAR(pCodeAccessTime);
	UNUSED_VAR(cvcType);
    return COMM_DL_SUCESS;
}

int CommonDownloadSetCvcAccessStartTime(cvcAccessStart_t *pCodeAccessTime,CVCType_e cvcType)
{	
	UNUSED_VAR(pCodeAccessTime);
	UNUSED_VAR(cvcType);
    return COMM_DL_SUCESS;
}

int CommonDownloadSetManCvc(unsigned char *pData, unsigned long Size)
{	
	UNUSED_VAR(pData);
	UNUSED_VAR(Size);
    return COMM_DL_SUCESS;
}

char * CommonDownloadGetFilePath()
{
	return NULL;
}
	
int CommonDownloadInitFilePath()
{
	return 0;
}

