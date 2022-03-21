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



#ifndef _CDLPKCS7DataC_H_
#define _CDLPKCS7DataC_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    unsigned char *pCountryName;
    unsigned char *pOrgName;
    unsigned char *pComName;
}IssuerName_t;

typedef struct
{
    unsigned char Time[12];

}ValidityStartTime_t;

typedef struct
{
    unsigned char Time[12];

}SigningTime_t;


typedef struct cvcinfo {
  unsigned char *subjectCountryName;
  unsigned char *issuerCountryName;
  unsigned char *subjectOrgName;
  unsigned char *issuerOrgName;
  unsigned char *subjectCommonName;
  unsigned char *issuerCommonName;
  unsigned char notBefore[12];
  unsigned char notAfter[12];
  unsigned long validityPeriod;
  int           codeSigningExists;
  unsigned char *key;
  int           keyLen;
} cvcinfo_t;
  

typedef struct
{
     unsigned char bValid; /* Boolean flag is set to '1' if the cvc info is valid otherwise it is set to '0' */
     ValidityStartTime_t ValStartTime; /* I think this size is same as cvcAccessStart, which is 12 bytes */
     unsigned long ValidityPeriod;
     IssuerName_t Subject;
}cvcInfo_t;
typedef struct
{
     unsigned char bValid; /* Boolean flag is set to '1' if the singer info is valid otherwise it is set to '0' */
     unsigned long Version;
     IssuerName_t IssuerName;
     unsigned long CertSerialNumber;
     SigningTime_t signingTime;
}signerInfo_t;



typedef struct
{
    cvcInfo_t   MfgCvcInfo;
    cvcInfo_t   CosignerCvcInfo; /* This is optional in the pkcs7 Image file structure, if  Cosigner CVC is not available then the bValid  flag in cvcInfo_t  is set to '0' */
   signerInfo_t MfgSignerInfo;
   signerInfo_t CoSignerInfo;/* This is optional  pkcs7 Image file structure, if  CosignerInfo is not available then the bValid  flag in signerInfo_t  is set to '0' */
}
pkcs7SignedDataInfo_t;


#ifdef __cplusplus
}// extern "C" {
#endif

#endif
