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



#ifndef _CDLNVDataACC_H_
#define _CDLNVDataACC_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "cvtdownloadtypes.h"
//#define USE_FILEDATA
typedef enum
{
  COM_DOWNL_MAN_CVC = 0,
  COM_DOWNL_COSIG_CVC = 1,

  COM_DOWNL_MAN_CODE_ACCESS_START = 2,
  COM_DOWNL_COSIG_CODE_ACCESS_START,

  COM_DOWNL_FW_MAN_CODE_ACCESS_START = 4,
  COM_DOWNL_FW_COSIG_CODE_ACCESS_START,

  COM_DOWNL_APP_MAN_CODE_ACCESS_START = 6,
  COM_DOWNL_APP_COSIG_CODE_ACCESS_START,

  COM_DOWNL_DATA_MAN_CODE_ACCESS_START =  8,
  COM_DOWNL_DATA_COSIG_CODE_ACCESS_START,

  COM_DOWNL_UPGRD_MAN_CODE_ACCESS_START = 10,
  COM_DOWNL_UPGRD_COSIG_CODE_ACCESS_START,

  COM_DOWNL_UPGRD_FW_MAN_CODE_ACCESS_START = 12,
  COM_DOWNL_UPGRD_FW_COSIG_CODE_ACCESS_START,

  COM_DOWNL_UPGRD_APP_MAN_CODE_ACCESS_START = 14,
  COM_DOWNL_UPGRD_APP_COSIG_CODE_ACCESS_START,

  COM_DOWNL_UPGRD_DATA_MAN_CODE_ACCESS_START = 16,
  COM_DOWNL_UPGRD_DATA_COSIG_CODE_ACCESS_START,

  
}CVCType_e;
#define COM_DOWNL_TIME_SIZE    12
typedef struct
{
unsigned char Time[COM_DOWNL_TIME_SIZE];
//can add any other info required.. This structure can be changed if rquired.

}CodeAccessStart_t;
typedef struct
{
unsigned char Time[COM_DOWNL_TIME_SIZE];
//can add any other info required.. This structure can be changed if rquired.

}cvcAccessStart_t;
typedef struct
{
    char tftp_server_address[16];
    char code_file_name[128];
    unsigned long code_file_name_len;
}CommDLTftpParams_t;
// returns the ManufacturerName string pointer or NULL if not availalbe
unsigned char * CommonDownloadGetManufacturerName(void);

// returns the CosignerName string pointer or NULL if not availalbe
unsigned char * CommonDownloadGetCosignerName(void);

//returns 0 on Success.   This is for both Man and cosigner access time values
int  CommonDownloadGetCodeAccessStartTime(CodeAccessStart_t *pCodeAccessTime,CVCType_e cvcType);

//return 0 on success.. This is for both Man and cosigner access time values
int  CommonDownloadGetCvcAccessStartTime(cvcAccessStart_t *pCodeAccessTime,CVCType_e cvcType);

//returns 0 on Success.   This is for both Man and cosigner access time values
int  CommonDownloadSetCodeAccessStartTime(CodeAccessStart_t *pCodeAccessTime,CVCType_e);

//return 0 on success.. This is for both Man and cosigner access time values
int  CommonDownloadSetCvcAccessStartTime(cvcAccessStart_t *pCodeAccessTime,CVCType_e);



//returns 0 on success..... returns stored CVCCA pubic key  in NV ram.
int  CommonDownloadGetCvcCAPublicKey(unsigned char **pData, unsigned long *Size);


//returns 0 on success..... returns sets the public key   in NV ram.
int  CommonDownloadSetCvcCAPublicKey(unsigned char *pData, unsigned long Size);





//--------------------------------------------------------------------------//
//returns 0 on success..... Returns CLCVCRootCA stored in NV ram and size.
int  CommonDownloadGetCLCvcRootCA(unsigned char **pData, unsigned long *Size);

//returns 0 on success..... Returns CLCVCCA stored in NV ram and size.
int  CommonDownloadGetCLCvcCA(unsigned char **pData, unsigned long *Size);

//returns 0 on success..... Returns Manufacturer CVC stored in NV ram and size.
int  CommonDownloadGetManCvc(unsigned char **pData, unsigned long *Size);

//returns 0 on success..... Returns AppCVCCA stored in NV ram and size.
int  CommonDownloadGetAppCVCCA(unsigned char **pData, unsigned long *pSize);

//returns 0 on success..... Returns AppManCVC stored in NV ram and size.
int  CommonDownloadGetAppManCVC(unsigned char **pData, unsigned long *pSize);


//returns 0 on success..... stores CLCVCRootCA  in NV ram.
int  CommonDownloadSetCLCvcRootCA(unsigned char *pData, unsigned long Size);

//returns 0 on success..... stores CLCVCRootCA  in NV ram.
int  CommonDownloadSetCLCvcCA(unsigned char *pData, unsigned long Size);

//returns 0 on success..... stores CLCVCRootCA  in NV ram.
int  CommonDownloadSetManCvc(unsigned char *pData, unsigned long Size);



//----------------------------------------------------------------------------------
int  CommonDownloadGetVendorId(unsigned char **pData, unsigned long *Size);
int  CommonDownloadGetHWVerId(unsigned char **pData, unsigned long *Size);
int  CommonDownloadGetCodeFileName(unsigned char **pData, unsigned long *Size);
int  CommonDownloadGetCodeFileNameCvt2v2(unsigned char **pData, unsigned long *pSize,cvt2v2_object_type_e objType );
int  CommonDownloadSetCodeFileNameCvt2v2(unsigned char *pData, unsigned long Size,cvt2v2_object_type_e objType );
int  CommonDownloadGetCDLMgrStatus(unsigned char **pData, unsigned long *Size);
int  CommonDownloadGetCDLTftpParams(unsigned char **pData, unsigned long *Size);

int  CommonDownloadSetCDLMgrStatus(unsigned char *pData, unsigned long Size);
int  CommonDownloadSetCDLTftpParams(unsigned char *pData, unsigned long Size);
int  CommonDownloadSetCodeFileName(unsigned char *pData, unsigned long Size);
int CommonDownloadSetCosignerName(unsigned char *pData,unsigned long size);
int CommonDownloadNvRamDataInit();
int CommonDownLSetTimeControls();
int CommonDownLUpdateCodeAccessTime();
int CommonDownLClearCodeAccessUpgrTime();
int CommonDownloadInitFilePath();
char * CommonDownloadGetFilePath();
#ifdef __cplusplus
}// extern "C" {
#endif

#endif
