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


#ifndef ComDownLDefDLMONITOR_H_
#define ComDownLDefDLMONITOR_H_


//extern "C" int  ComDownLDefDownloadNotify();
typedef enum  CDLSnmpRet_s
{
    CLD_SNMP_SUCCESS = 0,
    CLD_SNMP_FAILURE = 1,
}CDLSnmpRet_e;
typedef enum CDLFirmwareImageStatus
{
    IMAGE_AUTHORISED = 1,//'imageAuthorized' means the image is valid.
    IMAGE_CORRUPTED = 2,//imageCorrupted' means the image is invalid.
    IMAGE_CERT_FAILURE = 3,//'imageCertFailure' means CVC authentication has failed.
    IMAGE_MAX_DOWNLOAD_RETRY = 4,
        /*imageMaxDownloadRetry' means the maximum number of code
            file download retries has been reached. When the value of
            this object is imageMaxDownloadRetry(4), the value of
            ocStbHostSoftwareCodeDownloadStatus must be
            downloadingFailed(3).*/
    IMAGE_MAX_REBOOT_RETRY = 5,
    /*means the maximum number of reboots
            has occurred after code file download*/
}CDLFirmwareImageStatus_e;
typedef enum CDLFirmwareCodeDLStatus
{
    DOWNLOADING_STARTED = 1,// downloading means started
    DOWNLOADING_COMPLETE = 2,//'downloadingComplete' means the image download was successful.
    DOWNLOADING_FAILED= 3,//'downloadingFailed' means the image download failed."
    
}CDLFirmwareCodeDLStatus_e;
typedef enum CDLFirmwareDLFailedStatus
{
    CDL_ERROR_1 = 1,
    CDL_ERROR_2,
    CDL_ERROR_3,
    CDL_ERROR_4,
    CDL_ERROR_5,
    CDL_ERROR_6,
    CDL_ERROR_7,
    CDL_ERROR_8,
    CDL_ERROR_9,
    CDL_ERROR_10,
    CDL_ERROR_11,
    CDL_ERROR_12,
    CDL_ERROR_13,
    CDL_ERROR_14,
    CDL_ERROR_15,
    CDL_ERROR_16,
    CDL_ERROR_17,
    CDL_ERROR_18,
    CDL_ERROR_19,
    CDL_ERROR_20,
    CDL_ERROR_21,
    CDL_ERROR_22,
    CDL_ERROR_23,
    CDL_ERROR_24,
    CDL_ERROR_25,
    CDL_ERROR_99=99,
/*      failed status codes.
            cdlError1, No Failure
            cdlError2, Improper code file controls - CVC subject
                organizationName for manufacturer does not match the
                Host device manufacturer name
            cdlError3, Improper code file controls - CVC subject
                organizationName for code cosigning agent does not
                match the Host device current code cosigning agent.
            cdlError4, Improper code file controls - The
                manufacturer's PKCS #7 signingTime value is equal-to
                or less-than the codeAccessStart value currently held
                in the Host device.
            cdlError5, Improper code file controls - The
                manufacturer's PKCS #7 validity start time value is
                less-than the cvcAccessStart value currently held in
                the Host device.
            cdlError6, Improper code file controls - The manufacturer's
                CVC validity start time is less-than the cvcAccessStart
                value currently held in the Host device.
            cdlError7, Improper code file controls - The manufacturer's
                PKCS #7 signingTime value is less-than the CVC validity
                start time.
            cdlError8, Improper code file controls - Missing or
                improper extendedKeyUsage extension in the
                manufacturer CVC.
            cdlError9, Improper code file controls - The cosigner's
                PKCS #7 signingTime value is equal-to or less-than the
                codeAccessStart value currently held in the
                Host device.
            cdlError10, Improper code file controls - The cosigner's
                PKCS #7 validity start time value is less-than the
                cvcAccessStart value currently held in the Host device.
            cdlError11, Improper code file controls - The cosigner's
    CVC validity start time is less-than the cvcAccessStart
    value currently held in the Host device.
cdlError12, Improper code file controls - The cosigner's
    PKCS #7 signingTime value is less-than the CVC validity
    start time.
cdlError13, Improper code file controls - Missing or
    improper extended key-usage extension in the cosigner's
    CVC.
cdlError14, Code file manufacturer CVC validation failure.
cdlError15, Code file manufacturer CVS validation failure.
cdlError16, Code file cosigner CVC validation failure.
cdlError17, Code file cosigner CVS validation failure.
cdlError18, Improper eCM configuration file CVC format
    (e.g., missing or improper key usage attribute).
cdlError19, eCM configuration file CVC validation failure.
cdlError20, Improper SNMP CVC format.
cdlError21, CVC subject organizationName for manufacturer
    does not match the Host devices manufacturer name.
cdlError22, CVC subject organizationName for code cosigning
    agent does not match the Host devices current code
    cosigning agent.
cdlError23, The CVC validity start time is less-than or
    equal-to the corresponding subject's cvcAccessStart
    value currently held in the Host device.
cdlError24, Missing or improper key usage attribute.
cdlError25, SNMP CVC validation failure."
cdlError99, Other errors. Errors that do not fall into the
    categories delineated above."
*/

}CDLFirmwareDLFailedStatus_e;
#define CDL_MAX_NAME_STR_SIZE    256
#define CDL_MAX_TIME_STR_SIZE    128
typedef struct CDLMgrSnmpHostFirmwareStatus_s
{
    int HostFirmwareImageStatus;//details the image status(CDLFirmwareImageStatus) recently downloaded
    int HostFirmwareCodeDownloadStatus;//details the download status(CDLFirmwareCodeDLStatus_e) of the target image.
    char HostFirmwareCodeObjectName[CDL_MAX_NAME_STR_SIZE];//"The file name of the software image to be loaded into this device
    int HostFirmwareCodeObjectNameSize;// size of the file to be downloaded, size is 0 if no file to be downloaded.
    int HostFirmwareDownloadFailedStatus;//details the firmware download failed status(CDLFirmwareDLFailedStatus_e) codes
}CDLMgrSnmpHostFirmwareStatus_t;

typedef struct CDLMgrSnmpCertificateStatus_s
{
    unsigned char bCLRootCACertAvailable;// 0 if certificate is not stored
    unsigned char bCvcCACertAvailable;// 0 if certificate is not stored
    unsigned char bDeviceCVCCertAvailable;// 0 if certificate is not stored

     char pCLRootCASubjectName[CDL_MAX_NAME_STR_SIZE];// string contains the subject common name
     char pCLRootCAStartDate[CDL_MAX_TIME_STR_SIZE];// string contains the cert start date 
     char pCLRootCAEndDate[CDL_MAX_TIME_STR_SIZE];// string contaings the cert end date

     char pCLCvcCASubjectName[CDL_MAX_NAME_STR_SIZE];// string contains the subject common name
     char pCLCvcCAStartDate[CDL_MAX_TIME_STR_SIZE];// string contains the cert start date 
     char pCLCvcCAEndDate[CDL_MAX_TIME_STR_SIZE];// string contaings the cert end date

     char pDeviceCvcSubjectName[CDL_MAX_NAME_STR_SIZE];// string contains the subject common name
     char pDeviceCvcStartDate[CDL_MAX_TIME_STR_SIZE];// string contains the cert start date 
     char pDeviceCvcEndDate[CDL_MAX_TIME_STR_SIZE];// string contaings the cert end date

    unsigned char bVerifiedWithChain;// 1 if cert chain is verified successfully
}CDLMgrSnmpCertificateStatus_t;
// returns CDLSnmpRet_e
int CommonDownloadMgrSnmpHostFirmwareStatus(CDLMgrSnmpHostFirmwareStatus_t *pStatus);
int CommonDownloadMgrSnmpCertificateStatus_check(CDLMgrSnmpCertificateStatus_t *pStatus);

//returns 0 on success(means all the certificates are valid) , returns -1 on failure 
//the data CDLMgrSnmpCertificateStatus_t is valid on both cases
int CommonDownloadMgrSnmpCertificateStatus(CDLMgrSnmpCertificateStatus_t *pStatus);

void  PrintSnmpCertificateStatus(CDLMgrSnmpCertificateStatus_t *pStatus);
#endif //_CDownloadEngine_H_
