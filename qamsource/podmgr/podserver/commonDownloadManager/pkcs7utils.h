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


#ifndef PKCS7UTILS_H
#define PKCS7UTILS_H

#include <pkcs7SinedDataInfo.h>

#ifdef __cplusplus
extern "C" {
#endif
#define MFR_CVC   0x1
#define CO_CVC    0x2
#if 0
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
  
#endif

int getSignedContent(void *signedData, int len, char bFile, char **contentData, int *dataSize);
int getSignerInfo(void *signedData, int len, char bFile, pkcs7SignedDataInfo_t *signerInfo);
int getSigningTimes(void *signedData, int len, char bFile, unsigned char signingTime[2][12]);
int getCVCInfoFromNonSignedData(void *data, int len, cvcinfo_t *cvcInfo);
int getCVCInfos(void *signedData, int len, char bFile, cvcinfo_t cvcInfos[]);
int getPublicKeyFromCVC(void *data, int len, unsigned char **key, int *keyLen);
int validateCertSignature(void *code, int len, void *storedKey, int storedKeyLen);
int validateCertSigFromCodeImage(void *code, int len, char bFile, void *storedKey, int storedKeyLen);
//int verifySignedContent(void *code, int len, char bFile);
int pkcs7getRootCACert(void *code, int len, void **cert, int *certLen);
int pkcs7getBaseCACert(void *code, int len, void **cert, int *certLen);
int pkcs7getMfrCACert(void *code, int len, void **cert, int *certLen);
//int checkTime(unsigned char one[12], unsigned char two[12]);

void setCVCUsage(int cvcMask);
int  getCVCUsage(void);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* PKCS7UTILS_H */
