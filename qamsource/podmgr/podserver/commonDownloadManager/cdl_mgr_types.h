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


#ifndef _CDL_Mgr_Types_H_
#define _CDL_Mgr_Types_H_


#define  COM_DOWNL_TLV_TYPE_SUB_TLVS            28
#define  COM_DOWNL_TLV_TYPE_CL_DEVICE_CA_CERT        17
#define  COM_DOWNL_TLV_TYPE_CL_CVC_ROOT_CA_CERT        51
#define  COM_DOWNL_TLV_TYPE_CL_CVC_CA_CERT        52
#define COM_DOWNL_COMMAND_DEFERRED  0x01
#define COM_DOWNL_COMMAND_NOW        0x00


typedef enum
{
    COMM_DL_MONOLITHIC_IMAGE,
    COMM_DL_FIRMWARE_IMAGE,
    COMM_DL_APP_IMAGE,
    COMM_DL_DATA_IMAGE,
    
}CommonDLObjType_e;
typedef enum
{
    COMM_DL_TFTP_SUCESS,
    COMM_DL_TFTP_FAILED,
}CommonDownloadTftpDLStatus_e;
typedef enum
{
    COMM_DL_SUCESS = 0,
    COMM_DL_NV_RAM_DATA_ACCESS_ERROR,
    COMM_DL_NV_RAM_NO_DATA_ERROR,
    COMM_DL_CODE_FILE_NAME_LEN_NO_MATCH,
    COMM_DL_CODE_FILE_NAME_MATCH,
    COMM_DL_CVT_DATA_ERROR,
    COMM_DL_DEF_DL_CANCEL,
    COMM_DL_CVT_OBJECT_DATA_ERROR,
    COMM_DL_CVT_NVRAM_DATA_ERROR
}CommonDownloadReturnError;

//Move enum CDLBootDiagErrorCode to a common header file so that DIAG plugin does not have to include 
// this header file
typedef enum
{    
    // 02 – 25        CVC and image authentication failures
    // these error codes correspond to existing error codes defined for MIB object 
    // ocStbHostFirmwareDownloadFailed Status MIB in [MIB-HOST] and codes defined in Table 11.1-1.
    COMM_DL_BTDIAG_FILE_NOT_FOUND                        = 0x1A,  //26,
    COMM_DL_BTDIAG_SERVER_NOT_AVAILABLE                    = 0x1B,  //27,
    COMM_DL_BTDIAG_DOWNLOAD_MESSAGE_CORRUPT                = 0x5A,  //90,
    COMM_DL_BTDIAG_MAX_RETRY_EXHAUSTED                    = 0x5B,  //91,
    COMM_DL_BTDIAG_GENERAL_ERROR                        = 0x62,  //98,

    COMM_DL_BTDIAG_CVT_Invalid                            = 0xF0, 
    COMM_DL_BTDIAG_CVT_Damaged                            = 0xF1,    
    COMM_DL_BTDIAG_CVT_Mistmatch                        = 0xF2,
    COMM_DL_BTDIAG_CVT_Mistmatch_Hardware                = 0xF3,
    COMM_DL_BTDIAG_CVT_Mistmatch_HostMACAddress            = 0xF4,
    COMM_DL_BTDIAG_CVT_Mistmatch_HostID                    = 0xF5,
    COMM_DL_BTDIAG_CVT_Mismatch_GroupID                    = 0xF6,
    COMM_DL_BTDIAG_CVT_PKCS7_ValidationFailure            = 0xF7
}CDLBootDiagErrorCode;

typedef enum
{
    COM_DOWNL_IDLE, // common download manage is idle waiting for code image updates if available
    COM_DOWNL_CVT_PROC_AND_VALIDATION,// CVT1 or CVT2  process and Validation is in progress
    COM_DOWNL_CVT_PROC_VALIDATION_START,// CVT1 or CVT2 validation in process
    COM_DOWNL_CVT_PROC_VALIDATED,// CVT1 or CVT2 validation in process
    COM_DOWNL_CODE_IMAGE_DOWNLOAD_START,// CVT is validated and code image download is start initiated
    COM_DOWNL_CODE_IMAGE_DOWNLOAD_STARTED,// CVT is validated and code image download started
    COM_DOWNL_CODE_IMAGE_DOWNLOAD_COMPLETE,// CVT is validated and code image download is in progreess

    COM_DOWNL_CODE_IMAGE_PKCS7_VALIDATION,//code imaged downloaded and validation is in process
    COM_DOWNL_CODE_IMAGE_UPDATE,// code image validated and new image update in progress
    COM_DOWNL_NEW_IMAGE_CHECK, //system running with new image and checking the new image while running.
    COM_DOWNL_WAITING_ON_DEFF_DL, // common download manager is waiting on Deffered download time to expire...
    COM_DOWNL_IMAGE_UPGRADE,//
    COM_DOWNL_START_REBOOT,//

}CommonDownloadState_e;

typedef enum
{
    DOWNLOAD_NOT_REQUIRED,
    ONE_WAY_BROADCAST,//CVT1
    ALWAYS_ON_DEMAND,//CVT1
    DOWNL_UN_SUPPORT,//CVT1
    IN_BAND_FAT_DSMCC_DATA_CARSL,//CVT2
    DSG_APPL_TUNNEL_DSMCC_DATA_CARSL,//CVT2
    DOCSIS_TFTP//CVT2
}CommonDownloadDLType_t;
typedef enum
{
    DOWNLOAD_ENGINE_NO_IMAGE_FOUND,
    DOWNLOAD_ENGINE_IMAGE_DAMAGED,
    DOWNLOAD_ENGINE_DOWNLOAD_RETRY_TIMEOUT,
    DOWNLOAD_ENGINE_DOWNLOAD_TUNE_FAILED,
    COM_DOWNL_DSMCC_MAX_EXCEED,//max num of dsmcc blocks exceeded which is 64k
    DOWNLOAD_ENGINE_DOWNLOAD_TUNE_DONE_WAITING_FOR_IMAGE,
    DOWNLOAD_ENGINE_DOWNLOAD_START_TUNE,

}CommonDownEngineErrorCodes_e;

typedef enum
{
// defined as per Common download spec 2.0
    DOWNLOAD_STARTED = 0, // Initiated the  download process
    DOWNLOAD_COMPLETE = 1,// Download complete
    NO_DOWNLOAD_IMAGE =2,// Host could not find the download image.
    DOWNLOAD_MAX_RETRY =3,// Download max retries attempted during code file download
    IMAGE_DAMAGED = 4,// download file damaged or corrupted
    CERT_FAILURE = 5,// code CVC authentication failed
    REBOOT_MAX_RETRY = 6,// max retry attempted during the device rebooting with new code image
    CDL_RESERVED,
}CommonDownloadHostCmd_e;

typedef enum
{
     CVT_VALD_SUCCESS,//Complete CVT is validation is success..and CVC is valid
    CVT_UPDATE_CVC, // Update the cvc in the host because the cvc is  latest than the host cvc
    CVT_ERR_NO_CVC,//CVC is not prasent in the CVT
    //Add the status codes for different error cases.....
    CVT_ERR_CVC_EXPIR,// CVC validity time expired

}CVTValidationStatus_e;

#endif // _CDL_Mgr_Types_H_

