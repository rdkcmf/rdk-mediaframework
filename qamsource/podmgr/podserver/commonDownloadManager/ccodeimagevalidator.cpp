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


#include "ccodeimagevalidator.h"
#include "pkcs7utils.h"
#include "cardUtils.h"
//#include "cMisc.h"
#include <CommonDownloadNVDataAccess.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "CommonDownloadSnmpInf.h"
#include <rdk_debug.h>
 #include <stdlib.h>
//#include <vlMemCpy.h>
extern void PrintfCVCInfo(cvcinfo_t cvcInfo);
CCodeImageValidator::CCodeImageValidator(){
}
CCodeImageValidator::~CCodeImageValidator(){
}
static void PrintTime(unsigned char *pBuf)
{
    int ii;
    for(ii = 0; ii < 12; ii++)
    {
     RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL",":%02X",pBuf[ii]);
    }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","\n");
}
static int vlg_MfrTimeValid = 0;
static int vlg_CoSinTimeValid = 0;
static cvcAccessStart_t MfrCVCAccessStartTime;
static CodeAccessStart_t MfrCodeAccessStartTime;
static cvcAccessStart_t CoSinCVCAccessStartTime;
static CodeAccessStart_t CoSinCodeAccessStartTime;
extern int vlg_CDLFirmwareDLFailedStatus;

int CCodeImageValidator::processCodeImage(void *code, int len, CommonDLObjType_e  objType, bool bCheckTime)
{
     unsigned char       *storedKey;
      unsigned long       storedKeyLen;
      unsigned char       signingTimes[4][12];
      CodeAccessStart_t   mfrCodeAccessStart;
      CodeAccessStart_t   coCodeAccessStart;
      cvcAccessStart_t    mfrCVCAccessStart;
      cvcAccessStart_t    coCVCAccessStart;
      cvcinfo_t           cvcInfos[4];
      unsigned char       *mfrName;
      unsigned char       *coName;
     int                 mfrIndex = -1;
     int                 coIndex = -1;
      int                 i,ii,jj;
     int                 cvcUsage = 1;
    int             timeCheckRet = 0;
    int            numCVC = 0;
    CVCType_e        MfrcodeType,CoSigncodeType;
    CVCType_e        MfrcodeUpgType,CoSigncodeUpgType;

    memset(&mfrCodeAccessStart,0,sizeof(mfrCodeAccessStart));
    memset(&coCodeAccessStart,0,sizeof(coCodeAccessStart));
    memset(&mfrCVCAccessStart,0,sizeof(mfrCVCAccessStart));
    memset(&coCVCAccessStart,0,sizeof(coCVCAccessStart));

      bzero(cvcInfos, sizeof(cvcInfos));
     RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::processCodeImage >>>>>>>>>>>>>>>... Entered \n");

    switch(objType)
    {
    case COMM_DL_FIRMWARE_IMAGE:
        MfrcodeType = COM_DOWNL_FW_MAN_CODE_ACCESS_START;
        CoSigncodeType = COM_DOWNL_FW_COSIG_CODE_ACCESS_START;
        MfrcodeUpgType = COM_DOWNL_UPGRD_FW_MAN_CODE_ACCESS_START;
        CoSigncodeUpgType = COM_DOWNL_UPGRD_FW_COSIG_CODE_ACCESS_START;
        break;
    case COMM_DL_APP_IMAGE:
        MfrcodeType = COM_DOWNL_APP_MAN_CODE_ACCESS_START;
        CoSigncodeType = COM_DOWNL_APP_COSIG_CODE_ACCESS_START;
        MfrcodeUpgType = COM_DOWNL_UPGRD_APP_MAN_CODE_ACCESS_START;
        CoSigncodeUpgType = COM_DOWNL_UPGRD_APP_COSIG_CODE_ACCESS_START;
        break;
    case COMM_DL_DATA_IMAGE:
        MfrcodeType = COM_DOWNL_DATA_MAN_CODE_ACCESS_START;
        CoSigncodeType = COM_DOWNL_DATA_COSIG_CODE_ACCESS_START;
        MfrcodeUpgType = COM_DOWNL_UPGRD_DATA_MAN_CODE_ACCESS_START;
        CoSigncodeUpgType = COM_DOWNL_UPGRD_DATA_COSIG_CODE_ACCESS_START;
        break;
    case COMM_DL_MONOLITHIC_IMAGE:
        MfrcodeType = COM_DOWNL_MAN_CODE_ACCESS_START;
        CoSigncodeType = COM_DOWNL_COSIG_CODE_ACCESS_START;
        MfrcodeUpgType = COM_DOWNL_UPGRD_MAN_CODE_ACCESS_START;
        CoSigncodeUpgType = COM_DOWNL_UPGRD_COSIG_CODE_ACCESS_START;
        break;
    default:
         RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CCodeImageValidator::%s: Error object type:%d\n", __FUNCTION__,objType);
        return -1;
    }
    
    if(bCheckTime)
    {
        if(CommonDownloadGetCodeAccessStartTime(&mfrCodeAccessStart, MfrcodeType))
        {
             RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CCodeImageValidator::%s:  Failed to get current Mfr codeAccessStart time from NV!\n", __FUNCTION__);
                return -1;
        }
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Manufacturer CodeAccessStartTime \n");
        PrintTime(mfrCodeAccessStart.Time);

        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::processCodeImage ## 1\n");
        if(CommonDownloadGetCodeAccessStartTime(&coCodeAccessStart, CoSigncodeType))
        {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CCodeImageValidator::%s:  Failed to get current CoSign codeAccessStart time from NV!\n", __FUNCTION__);
                return -1;
        }
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Co Sig CodeAccessStartTime \n");
        PrintTime(coCodeAccessStart.Time);

        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::processCodeImage ## 2\n");
        if(CommonDownloadGetCvcAccessStartTime(&mfrCVCAccessStart, COM_DOWNL_MAN_CVC))
        {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CCodeImageValidator::%s:  Failed to get current Mfr cvcAccessStart time from NV!\n", __FUNCTION__);
                return -1;
        }
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Manufac CvcccessStartTime \n");
        PrintTime(mfrCVCAccessStart.Time);

        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::processCodeImage ## 3\n");
        if(CommonDownloadGetCvcAccessStartTime(&coCVCAccessStart, COM_DOWNL_COSIG_CVC))
        {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CCodeImageValidator::%s:  Failed to get current CoSign cvcAccessStart time from NV!\n", __FUNCTION__);
                return -1;
        }
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Co Sign CvcccessStartTime \n");
        PrintTime(coCVCAccessStart.Time);
    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CDL image time not checked.\n");
    }
    
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::processCodeImage ## 4\n");
    mfrName = CommonDownloadGetManufacturerName();
#ifdef PKCS_UTILS
    cvcUsage = getCVCUsage();
#endif
    if(mfrName == NULL)
    {
         RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CCodeImageValidator::%s: Failed to get MfrName from NV!\n", __FUNCTION__);
            return -1;
    }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::processCodeImage ## 5 mfrName:%s\n",mfrName);
    coName = CommonDownloadGetCosignerName();
    if(coName == NULL)
    {
         RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CCodeImageValidator::%s: Failed to get MfrName from NV!\n", __FUNCTION__);
            return -1;
    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::processCodeImage ## 5.1 coName:%s\n",coName);
    }

    
#ifdef PKCS_UTILS
    if(getCVCInfos(code, len, (char)1,&cvcInfos[0]) < 0)
    {
         RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CCodeImageValidator::%s: Failed to get any CVCs from Code image!\n", __FUNCTION__);
         return -1;
    }
#endif    
#if 1
    for(ii  = 0; ii < 2; ii++)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CVC INFO>>>>[%d]: \n",ii);
        PrintfCVCInfo(cvcInfos[ii]);
    
    }        
#endif
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::processCodeImage ## 6\n");
  // check PKCS#7 signing time
    // >= codeAccessStart held in the host
    // >= Mfr CVC validity start time in CVC delivered in codeFile SignedData
    // <= Mfr CVC validity end time in CVC delivered in codeFile SignedData
#ifdef PKCS_UTILS    
    if(getSigningTimes(code, len,(char)1, &signingTimes[0]) < 0)
    {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CCodeImageValidator::%s: Failed to get signingTime!\n", __FUNCTION__);
            return -1;
    }
#endif    
#if 1
    for(ii  = 0; ii < 2; ii++)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","\nSigning Time[%d]: ",ii);
        for(jj=0; jj < 12; jj++)
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL",":%02X ",signingTimes[ii][jj]);
        }
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","\n ");
    }        
#endif
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::processCodeImage ## 7\n");
    
#if 1 
    if(signingTimes[0][0] < 0xFF)
    {
        if(bCheckTime)
        {
    #ifdef PKCS_UTILS
            timeCheckRet = checkTime(mfrCodeAccessStart.Time, signingTimes[0]);
            if(timeCheckRet < 0)
            {
                //returns 0 if Time 1 = Time 2
                // returns 1 if Time1 < Time2
                // retunrs -1 if Time1 > Time2
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CCodeImageValidator::%s[%d]: Image fails signingTime test!\n", __FUNCTION__, __LINE__);
                return -1;
            }
    #endif
        }
        else
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CDL image time not checked.\n");
            timeCheckRet = 1;
        }
        
        if(timeCheckRet == 1)
        {
            memcpy(MfrCodeAccessStartTime.Time,&signingTimes[0][0],12);
            if(CommonDownloadSetCodeAccessStartTime(&MfrCodeAccessStartTime, MfrcodeUpgType))
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CCodeImageValidator::%s:  Failed to get current Mfr codeAccessStart time from NV!\n", __FUNCTION__);
                return -1;
            }
        }
    }
    
    // Check same if there is a Co-Signer
    if(signingTimes[1][0] < 0xFF)
    {
        if(bCheckTime)
        {
    #ifdef PKCS_UTILS      
            if(checkTime(coCodeAccessStart.Time, signingTimes[1]) < 0)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CCodeImageValidator::%s[%d]: Image fails signingTime test!\n", __FUNCTION__, __LINE__);
                return -1;
            }
    #endif
        }
        else
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CDL image time not checked.\n");
        }
    }
#endif
  // verify Mfr CVC in codeFile SignedData
    // check OrgName is identical to Mfr name in Host memory
    // check CVC validity start time is >= cvcAccessTime held in host
    // check extendedKeyUsage in CVC includes OID ip-kp-codeSigning
#if 1
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL",">>>>>>>>>> @@  cvcUsage:0x%X \n",cvcUsage);
    if(cvcUsage & MFR_CVC)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," MFR_CVC \n");
      numCVC++;
    }
    if(cvcUsage & CO_CVC)
    {
      RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," CO_CVC \n");
      numCVC++;
    }
    
    if( ( numCVC >= 1) && (numCVC <=  2) )
    {
        int ii;
        int MfrIndex = -1;
        int CoSignIndex = -1;
        //REQ:4028
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator : ............ \n");
        for(ii = 0; ii < 2; ii++)
        {
            if(cvcInfos[ii].subjectOrgName != NULL)
            {
              if(strcmp((const char*)mfrName, (const char*)cvcInfos[ii].subjectOrgName) == 0)
              {
                
                MfrIndex = ii;
                if(MfrIndex == 1)
                  CoSignIndex = 0;
                else
                  CoSignIndex = 1;
                
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::%s[%d]:Code image contains mfr cvc at index[%d] ..Check OK \n", __FUNCTION__, __LINE__,ii);
                break;
              }
            }
        }
        if(MfrIndex == -1 )
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::%s[%d]:Error !!! The Image contains No Mfr CVC... Image CVC verification Failed!!\n", __FUNCTION__, __LINE__);
            vlg_CDLFirmwareDLFailedStatus = CDL_ERROR_14;
            return vlg_CDLFirmwareDLFailedStatus;
        }
        if( (numCVC == 1) && (cvcUsage & MFR_CVC) )
        {
            if(cvcInfos[CoSignIndex].subjectOrgName != NULL)
            {
            if(strcmp((const char*)coName, (const char*)cvcInfos[CoSignIndex].subjectOrgName) == 0)
            {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::%s[%d]:CVT has Mfr CVC only.. The Image contains Mfr and cosinger cvc  .. Image CVC verification Failed! @index0\n", __FUNCTION__, __LINE__);
              
            }
            else
            {
              RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::%s[%d]:CVT has Mfr CVC only.. The Image contains Mfr and one more cvc:%s  .. Image CVC verification Failed! @index0\n", __FUNCTION__, __LINE__,cvcInfos[CoSignIndex].subjectOrgName);
            }
            vlg_CDLFirmwareDLFailedStatus = CDL_ERROR_14;
            return vlg_CDLFirmwareDLFailedStatus;
            }
        }//if( (numCVC == 1) && (cvcUsage & MFR_CVC) )
        if( (numCVC == 1) && (cvcUsage & CO_CVC) )
        {
            if(cvcInfos[CoSignIndex].subjectOrgName != NULL)
            {
              if(strcmp((const char*)coName, (const char*)cvcInfos[0].subjectOrgName) == 0)
              {
                  RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::%s[%d]:CVT has Co-signer CVC only.. The Image contains both mfr and cosinger cvc \n", __FUNCTION__, __LINE__);
                
              }
              else
              {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::%s[%d]:CVT has Co-signer CVC only.. The Image contains Mfr and one more cvc:%s  .. Image CVC verification Failed! @index0\n", __FUNCTION__, __LINE__,cvcInfos[CoSignIndex].subjectOrgName);
                vlg_CDLFirmwareDLFailedStatus = CDL_ERROR_16;
                return vlg_CDLFirmwareDLFailedStatus;
              }
              
            }
            else
            {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::%s[%d]:CVT has Co-signer CVC only.. The Image contains Mfr cvc only and no co-signer cvc .. Image CVC verification Failed!! \n", __FUNCTION__, __LINE__);
            vlg_CDLFirmwareDLFailedStatus = CDL_ERROR_16;
            return vlg_CDLFirmwareDLFailedStatus;
            }
        }//if( (numCVC == 1) && (cvcUsage & CO_CVC) )
    }//if( ( numCVC >= 1) && (numCVC <=  2) )
    else
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::%s[%d]:Stored Cvcs information in CVTS [%d] is more than two.. or less than one !! \n", __FUNCTION__, __LINE__,numCVC);
        vlg_CDLFirmwareDLFailedStatus = CDL_ERROR_14;
        return vlg_CDLFirmwareDLFailedStatus;
    }

      for(i = 0; i < numCVC; i++)
      {
        if(cvcInfos[i].subjectOrgName == NULL )
        {
          break;
        }
            if(strcmp((const char*)mfrName, (const char*)cvcInfos[i].subjectOrgName) == 0)
        //if(memcmp((const char*)mfrName, (const char*)cvcInfos[i].subjectOrgName,15) == 0)
            {
                  /*if(!(cvcUsage & MFR_CVC))
                  {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::%s: Found Mfr CVC in Code image when it should not be there!\n", __FUNCTION__);
                    return -1;
                  }*/
                if(signingTimes[i][0] < 0xFF)
                {
                    if(bCheckTime)
                    {
                        timeCheckRet = checkTime(mfrCodeAccessStart.Time, signingTimes[0]);
                        if(timeCheckRet < 0)
                        {
                            //returns 0 if Time 1 = Time 2
                            // returns 1 if Time1 < Time2
                            // retunrs -1 if Time1 > Time2
                            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::%s[%d]:Error Image Mfr signingTime test( HostCodeAccessStartTime > ImageSigning Time )\n", __FUNCTION__, __LINE__);
                            vlg_CDLFirmwareDLFailedStatus = CDL_ERROR_4;
                            return vlg_CDLFirmwareDLFailedStatus;
                        }
                    }
                    else
                    {
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CDL image time not checked.\n");
                        timeCheckRet = 1;
                    }
                    
                    if(timeCheckRet == 1)
                    {
                        memcpy(MfrCodeAccessStartTime.Time,&signingTimes[0][0],12);
                        if(CommonDownloadSetCodeAccessStartTime(&MfrCodeAccessStartTime, MfrcodeUpgType))
                        {
                            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::%s:  Failed to get current Mfr codeAccessStart time from NV!\n", __FUNCTION__);
                            return -1;
                        }
                    }
                }
    
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::processCodeImage ## 8\n");
                  if(cvcInfos[i].codeSigningExists != 1)
                  {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::%s: Mfr CVC fails extendedKeyUsage test!\n", __FUNCTION__);
                    vlg_CDLFirmwareDLFailedStatus = CDL_ERROR_8;
                    return vlg_CDLFirmwareDLFailedStatus;
                  }
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::processCodeImage ## 9\n");
                if(bCheckTime)
                {
                    if(checkTime(cvcInfos[i].notBefore, signingTimes[0]) < 0)
                    /*|| (checkTime(signingTimes[0], cvcInfos[i].notAfter) < 0))*/
                    {
                    //returns 0 if Time 1 = Time 2
                    // returns 1 if Time1 < Time2
                    // retunrs -1 if Time1 > Time2
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::%s: Image fails Mfr CVC times test (ImagecvcAccessStartTime > ImageSigningTime) \n", __FUNCTION__);
                        vlg_CDLFirmwareDLFailedStatus = CDL_ERROR_7;
                        return vlg_CDLFirmwareDLFailedStatus;
                    }
                    if( checkTime(signingTimes[0], cvcInfos[i].notAfter) < 0 )
                    {
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::%s: Image fails Mfr CVC times test (ImageSigningTime > ImagecvcAccessEndTime) \n", __FUNCTION__);
                        vlg_CDLFirmwareDLFailedStatus = CDL_ERROR_5;
                        return vlg_CDLFirmwareDLFailedStatus;
                        
                    }
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::processCodeImage ## 10\n");
                if((checkTime(mfrCVCAccessStart.Time,cvcInfos[i].notBefore) < 0)       )
                    {
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::%s: Image fails Mfr CVC times test( HostCvcAcceessStartTime > ImageCvcStartTime )\n", __FUNCTION__);
                        vlg_CDLFirmwareDLFailedStatus = CDL_ERROR_6;
                    
                        return vlg_CDLFirmwareDLFailedStatus;
                    }
                }
                else
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CDL image time not checked.\n");
                }
                  mfrIndex = i;
            }
            
            else if(strcmp((const char*)coName, (const char*)cvcInfos[i].subjectOrgName) == 0)
            {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::processCodeImage ## 11\n");
                  /*if(!(cvcUsage & CO_CVC))
                  {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::%s: Found Mfr CVC in Code image when it should not be there!\n", __FUNCTION__);
                    return -1;
                  }*/
                if(signingTimes[i][0] < 0xFF)
                  {
                    if(bCheckTime)
                    {
                        timeCheckRet = checkTime(coCodeAccessStart.Time, signingTimes[1]);
                        if(timeCheckRet < 0)
                        {
                        //returns 0 if Time 1 = Time 2
                        // returns 1 if Time1 < Time2
                        // retunrs -1 if Time1 > Time2
                            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::%s[%d]: Image fails Co-signer signingTime test ( HostCvcAccessStartTime > ImageSigningTime )\n", __FUNCTION__, __LINE__);
                            vlg_CDLFirmwareDLFailedStatus = CDL_ERROR_9;
                            return vlg_CDLFirmwareDLFailedStatus;
                        }
                    }
                    else
                    {
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CDL image time not checked.\n");
                        timeCheckRet = 1;
                    }
                    if(timeCheckRet == 1)
                    {
                        memcpy(CoSinCodeAccessStartTime.Time,&signingTimes[1][0],12);
                        if(CommonDownloadSetCodeAccessStartTime(&CoSinCodeAccessStartTime, CoSigncodeUpgType))
                        {
                            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::%s:  Failed to get current Co-signer codeAccessStart time from NV!\n", __FUNCTION__);
                            return -1;
                        }
                    }
                }
                if(bCheckTime)
                {
                    if(checkTime(signingTimes[1], cvcInfos[i].notAfter) < 0)
                    {
                        //returns 0 if Time 1 = Time 2
                        // returns 1 if Time1 < Time2
                        // retunrs -1 if Time1 > Time2
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::%s: Image fails CoSign CVC times test (ImageSigningTime > ImageCvcEndTime)\n", __FUNCTION__);
                        vlg_CDLFirmwareDLFailedStatus = CDL_ERROR_10;
                        return vlg_CDLFirmwareDLFailedStatus;
                    }
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::processCodeImage ## 12\n");
                    if(checkTime(cvcInfos[i].notBefore, signingTimes[1]) <= 0)
                    {
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::%s: Image fails CoSign CVC times test(ImageCvcStartTime > ImageSigningTime )\n", __FUNCTION__);
                        vlg_CDLFirmwareDLFailedStatus = CDL_ERROR_12;
                        return vlg_CDLFirmwareDLFailedStatus;
                    }
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::processCodeImage ## 13\n");
                if((checkTime(coCVCAccessStart.Time,cvcInfos[i].notBefore) < 0)       )
                    {
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::%s: Image fails Co-sign CVC times test (HostCvcAccessTime  > ImageCvcStartTime\n", __FUNCTION__);
                        vlg_CDLFirmwareDLFailedStatus = CDL_ERROR_11;
                        return vlg_CDLFirmwareDLFailedStatus;
                    }
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::processCodeImage ## 14\n");
                }
                else
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CDL image time not checked.\n");
                }
            /*
                  if(cvcInfos[i].codeSigningExists != 1)
                  {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::%s: Co-sign CVC fails extendedKeyUsage test!\n", __FUNCTION__);
                    return -1;
                  }*/
                  coIndex = i;
        }
      }//for(i = 0; i < 1; i++)
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::processCodeImage ## 15\n");
      if((mfrIndex == -1)/* && (cvcUsage & MFR_CVC)*/)
      {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::%s: Failed to find a CVC for Mfr %s!\n", __FUNCTION__, mfrName);
        vlg_CDLFirmwareDLFailedStatus = CDL_ERROR_2;
        return vlg_CDLFirmwareDLFailedStatus;
      }
    
    if(cvcUsage & CO_CVC)
    {
        if(coIndex == -1)
        {
          RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::%s: Failed to find a CVC for CoSign %s!\n", __FUNCTION__, coName);
          vlg_CDLFirmwareDLFailedStatus = CDL_ERROR_3;
              return vlg_CDLFirmwareDLFailedStatus;
        }
      }
/*
      if((coIndex == -1) && (cvcUsage & CO_CVC))
      {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::%s: Failed to find a CVC for CoSign %s!\n", __FUNCTION__, mfrName);
            return -1;
      }*/
#endif

#if 1
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","\n\n\n  >>>>>>>>>>>>> calling CommonDownloadGetCvcCAPublicKey() >>>>>>>>>>>>>>>>>>>>>>>>> \n\n\n");
/*
      if(CommonDownloadGetCvcCAPublicKey(&storedKey, &storedKeyLen) != 0)
      {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::%s: Failed to get the stored public key!\n", __FUNCTION__);
            return -1;
      }*/
{
    unsigned char     *cvcCA;
      unsigned long     cvcCALen;
      unsigned char     *cvcCAKey;
      int     cvcCAKeyLen;
    if(CommonDownloadGetCLCvcCA(&cvcCA, &cvcCALen))
      {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CDownloadCVTValidationMgr::%s: Failed to get CL Root CVC from NVM!\n", __FUNCTION__);
            return -1;
      }
#ifdef PKCS_UTILS
    if(getPublicKeyFromCVC(cvcCA, cvcCALen, &cvcCAKey, &cvcCAKeyLen))
      {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CDownloadCVTValidationMgr::%s: Failed to get PubKey from CL Root CVC in NVM!\n", __FUNCTION__);
            free( cvcCA );		
            return -1;
      }
#endif
      RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","\n\n\n  >>>>>>>>>>>>> calling validateCertSigFromCodeImage() >>>>>>>>>>>>>>>>>>>>>>>>> \n\n\n");
      // Validate CVC signature via the CL CVC CA Public Key held in device
#ifdef PKCS_UTILS
      if(validateCertSigFromCodeImage(code, len, (char)1,cvcCAKey, cvcCAKeyLen) != 0)
      {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::%s: Image fails Certificate validation test!\n", __FUNCTION__);
         vlg_CDLFirmwareDLFailedStatus = CDL_ERROR_14;
            free( cvcCAKey ); 		 
            free( cvcCA );	
            return vlg_CDLFirmwareDLFailedStatus;
      }
#endif
            free( cvcCA );
}


#endif    


    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::processCodeImage ## 16\n");
      // Perform SHA-1 Hash over SignedContent and compare with MessageDigest in signerInfo
      if(verifySignedContent(code, len,(char)1) != 0)
      {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::%s: Image fails MessageDigest test!\n", __FUNCTION__);
            return -1;
      }

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::processCodeImage ## 17\n");



    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::processCodeImage ## 18\n");
      return 0;
}

int CommonDownLSetTimeControls()
{
// Update time varying controls

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownLSetTimeControls: Updating Time Controls \n");
    if(vlg_MfrTimeValid)
    {
#if 0
          if( 0 != CommonDownloadSetCodeAccessStartTime((CodeAccessStart_t *)(&MfrCodeAccessStartTime), COM_DOWNL_MAN_CVC))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownLSetTimeControls: Error!!!! Upating CommonDownloadSetCodeAccessStartTime,COM_DOWNL_MAN_CVC\n");
            return -1;
        }
        else
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownLSetTimeControls: Success Upating CommonDownloadSetCodeAccessStartTime,COM_DOWNL_MAN_CVC\n");
        }
#endif
          if(0 != CommonDownloadSetCvcAccessStartTime((cvcAccessStart_t *)(&MfrCVCAccessStartTime), COM_DOWNL_MAN_CVC))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownLSetTimeControls: Error!!!! Upating CommonDownloadSetCvcAccessStartTime,COM_DOWNL_MAN_CVC\n");
            return -1;
        }
        else
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownLSetTimeControls: Success Upating CommonDownloadSetCvcAccessStartTime,COM_DOWNL_MAN_CVC\n");
        }
        vlg_MfrTimeValid = 0;
    }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CCodeImageValidator::processCodeImage ## 10\n");

    if(vlg_CoSinTimeValid)
    {
#if 0    
            if( 0 != CommonDownloadSetCodeAccessStartTime((CodeAccessStart_t *)(&CoSinCodeAccessStartTime), COM_DOWNL_COSIG_CVC))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownLSetTimeControls: Error!!!! Upating CommonDownloadSetCodeAccessStartTime,COM_DOWNL_COSIG_CVC\n");
            return -1;
        }
        else
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownLSetTimeControls: Success Upating CommonDownloadSetCodeAccessStartTime,COM_DOWNL_MAN_CVC\n");
        }
#endif
            if( 0 != CommonDownloadSetCvcAccessStartTime((cvcAccessStart_t *)(&CoSinCVCAccessStartTime), COM_DOWNL_COSIG_CVC))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownLSetTimeControls: Error!!!! Upating CommonDownloadSetCvcAccessStartTime,COM_DOWNL_COSIG_CVC\n");
            return -1;
        }
        else
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownLSetTimeControls: Success Upating CommonDownloadSetCvcAccessStartTime,COM_DOWNL_COSIG_CVC\n");
        }
        vlg_CoSinTimeValid = 0;
      }
    return 0; 

}
int CommonDownLUpdateCodeAccessTime()
{
    int timeCheckRet = 0,ii,jj;
    CodeAccessStart_t  Time,UpTime;
    
    ii = COM_DOWNL_MAN_CODE_ACCESS_START;
    jj= COM_DOWNL_UPGRD_MAN_CODE_ACCESS_START;
    while(1)
    {
        if((ii > COM_DOWNL_DATA_COSIG_CODE_ACCESS_START) || (jj > COM_DOWNL_UPGRD_DATA_COSIG_CODE_ACCESS_START))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: Exit loop ii:%d jj:%d \n",ii,jj);
            break;
        }
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: READ REG ii:0x%08X  \n",ii);
    
        if( 0 != CommonDownloadGetCodeAccessStartTime((CodeAccessStart_t *)(&Time), (CVCType_e)ii))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: Error!!!! Upating CommonDownloadGetCodeAccessStartTime,ii:%d\n",ii);
            return -1;
        }
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: READ REG jj:0x%08X  \n",jj);
        if( 0 != CommonDownloadGetCodeAccessStartTime((CodeAccessStart_t *)(&UpTime), (CVCType_e)jj))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: Error!!!! Upating CommonDownloadGetCodeAccessStartTime,jj:%d\n",jj);
            return -1;
        }
#ifdef PKCS_UTILS        
        timeCheckRet = checkTime(Time.Time, UpTime.Time);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," Check Time >>>>>>>>>>>>>>>>timeCheckRet:%d \n",timeCheckRet);
#endif        
        if(timeCheckRet == 1)
        {
            //returns 0 if Time 1 = Time 2
            // returns 1 if Time1 < Time2
            // retunrs -1 if Time1 > Time2
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: WRITE REG ii:0x%08X  \n",ii);
            if( 0 != CommonDownloadSetCodeAccessStartTime((CodeAccessStart_t *)(&UpTime),(CVCType_e) ii))
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: Error!!!! Upating CommonDownloadSetCodeAccessStartTime,ii:%d\n",ii);
                return -1;
            }
        }
        ii++;
        jj++;
    }//while(1)

    return 0;
}
#if 0
int CommonDownLUpdateCodeAccessTime()
{
    int timeCheckRet;
    CodeAccessStart_t  Time,UpTime;

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: READ REG:0x%08X COM_DOWNL_MAN_CODE_ACCESS_START \n",COM_DOWNL_MAN_CODE_ACCESS_START);

    if( 0 != CommonDownloadGetCodeAccessStartTime((CodeAccessStart_t *)(&Time), COM_DOWNL_MAN_CODE_ACCESS_START))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: Error!!!! Upating CommonDownloadGetCodeAccessStartTime,COM_DOWNL_MAN_CODE_ACCESS_START\n");
        return -1;
    }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: READ REG:0x%08X COM_DOWNL_UPGRD_MAN_CODE_ACCESS_START \n",COM_DOWNL_UPGRD_MAN_CODE_ACCESS_START);
    if( 0 != CommonDownloadGetCodeAccessStartTime((CodeAccessStart_t *)(&UpTime), COM_DOWNL_UPGRD_MAN_CODE_ACCESS_START))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: Error!!!! Upating CommonDownloadGetCodeAccessStartTime,COM_DOWNL_UPGRD_MAN_CODE_ACCESS_START\n");
        return -1;
    }
    
    timeCheckRet = checkTime(Time.Time, UpTime.Time);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," Check Time >>>>>>>>>>>>>>>>timeCheckRet:%d \n",timeCheckRet);
      if(timeCheckRet == 1)
      {
        //returns 0 if Time 1 = Time 2
        // returns 1 if Time1 < Time2
        // retunrs -1 if Time1 > Time2
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: WRITE REG:0x%08X COM_DOWNL_MAN_CODE_ACCESS_START \n",COM_DOWNL_MAN_CODE_ACCESS_START);
        if( 0 != CommonDownloadSetCodeAccessStartTime((CodeAccessStart_t *)(&UpTime), COM_DOWNL_MAN_CODE_ACCESS_START))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: Error!!!! Upating CommonDownloadSetCodeAccessStartTime,COM_DOWNL_MAN_CODE_ACCESS_START\n");
            return -1;
        }
      }
>>>>>>>>>>>>>>>>>>>>>

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: READ REG:0x%08X COM_DOWNL_UPGRD_MAN_CODE_ACCESS_START \n",COM_DOWNL_UPGRD_MAN_CODE_ACCESS_START);
    if( 0 != CommonDownloadGetCodeAccessStartTime((CodeAccessStart_t *)(&Time), COM_DOWNL_COSIG_CODE_ACCESS_START))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: Error!!!! Upating CommonDownloadGetCodeAccessStartTime,COM_DOWNL_COSIG_CODE_ACCESS_START\n");
        return -1;
    }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: READ REG:0x%08X COM_DOWNL_UPGRD_MAN_CODE_ACCESS_START \n",COM_DOWNL_UPGRD_MAN_CODE_ACCESS_START);
    if( 0 != CommonDownloadGetCodeAccessStartTime((CodeAccessStart_t *)(&UpTime), COM_DOWNL_UPGRD_COSIG_CODE_ACCESS_START))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: Error!!!! Upating CommonDownloadGetCodeAccessStartTime,COM_DOWNL_UPGRD_COSIG_CODE_ACCESS_START\n");
        return -1;
    }
    
    timeCheckRet = checkTime(Time.Time, UpTime.Time);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," Check Time >>>>>>>>>>>>>>>>timeCheckRet:%d \n",timeCheckRet);
      if(timeCheckRet == 1)
      {
        //returns 0 if Time 1 = Time 2
        // returns 1 if Time1 < Time2
        // retunrs -1 if Time1 > Time2
        if( 0 != CommonDownloadSetCodeAccessStartTime((CodeAccessStart_t *)(&UpTime), COM_DOWNL_COSIG_CODE_ACCESS_START))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: Error!!!! Upating CommonDownloadSetCodeAccessStartTime,COM_DOWNL_COSIG_CODE_ACCESS_START\n");
            return -1;
        }
      }



    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: READ REG:0x%08X COM_DOWNL_UPGRD_MAN_CODE_ACCESS_START \n",COM_DOWNL_UPGRD_MAN_CODE_ACCESS_START);
    if( 0 != CommonDownloadGetCodeAccessStartTime((CodeAccessStart_t *)(&Time), COM_DOWNL_FW_MAN_CODE_ACCESS_START))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: Error!!!! Upating CommonDownloadGetCodeAccessStartTime,COM_DOWNL_FW_MAN_CODE_ACCESS_START\n");
        return -1;
    }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: READ REG:0x%08X COM_DOWNL_UPGRD_MAN_CODE_ACCESS_START \n",COM_DOWNL_UPGRD_MAN_CODE_ACCESS_START);
    if( 0 != CommonDownloadGetCodeAccessStartTime((CodeAccessStart_t *)(&UpTime), COM_DOWNL_UPGRD_FW_MAN_CODE_ACCESS_START))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: Error!!!! Upating CommonDownloadGetCodeAccessStartTime,COM_DOWNL_UPGRD_FW_MAN_CODE_ACCESS_START\n");
        return -1;
    }
    
    timeCheckRet = checkTime(Time.Time, UpTime.Time);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," Check Time >>>>>>>>>>>>>>>>timeCheckRet:%d \n",timeCheckRet);
      if(timeCheckRet == 1)
      {
        //returns 0 if Time 1 = Time 2
        // returns 1 if Time1 < Time2
        // retunrs -1 if Time1 > Time2
        if( 0 != CommonDownloadSetCodeAccessStartTime((CodeAccessStart_t *)(&UpTime), COM_DOWNL_FW_MAN_CODE_ACCESS_START))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: Error!!!! Upating CommonDownloadSetCodeAccessStartTime,COM_DOWNL_FW_MAN_CODE_ACCESS_START\n");
            return -1;
        }
      }


    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: READ REG:0x%08X COM_DOWNL_UPGRD_MAN_CODE_ACCESS_START \n",COM_DOWNL_UPGRD_MAN_CODE_ACCESS_START);
    if( 0 != CommonDownloadGetCodeAccessStartTime((CodeAccessStart_t *)(&Time), COM_DOWNL_FW_COSIG_CODE_ACCESS_START))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: Error!!!! Upating CommonDownloadGetCodeAccessStartTime,COM_DOWNL_FW_COSIG_CODE_ACCESS_START\n");
        return -1;
    }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: READ REG:0x%08X COM_DOWNL_UPGRD_MAN_CODE_ACCESS_START \n",COM_DOWNL_UPGRD_MAN_CODE_ACCESS_START);
    if( 0 != CommonDownloadGetCodeAccessStartTime((CodeAccessStart_t *)(&UpTime), COM_DOWNL_UPGRD_FW_COSIG_CODE_ACCESS_START))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: Error!!!! Upating CommonDownloadGetCodeAccessStartTime,COM_DOWNL_UPGRD_FW_COSIG_CODE_ACCESS_START\n");
        return -1;
    }
    
    timeCheckRet = checkTime(Time.Time, UpTime.Time);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," Check Time >>>>>>>>>>>>>>>>timeCheckRet:%d \n",timeCheckRet);
      if(timeCheckRet == 1)
      {
        //returns 0 if Time 1 = Time 2
        // returns 1 if Time1 < Time2
        // retunrs -1 if Time1 > Time2
        if( 0 != CommonDownloadSetCodeAccessStartTime((CodeAccessStart_t *)(&UpTime), COM_DOWNL_FW_COSIG_CODE_ACCESS_START))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: Error!!!! Upating CommonDownloadSetCodeAccessStartTime,COM_DOWNL_FW_COSIG_CODE_ACCESS_START\n");
            return -1;
        }
      }


    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: READ REG:0x%08X COM_DOWNL_UPGRD_MAN_CODE_ACCESS_START \n",COM_DOWNL_UPGRD_MAN_CODE_ACCESS_START);
    if( 0 != CommonDownloadGetCodeAccessStartTime((CodeAccessStart_t *)(&Time), COM_DOWNL_APP_MAN_CODE_ACCESS_START))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: Error!!!! Upating CommonDownloadGetCodeAccessStartTime,COM_DOWNL_APP_MAN_CODE_ACCESS_START\n");
        return -1;
    }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: READ REG:0x%08X COM_DOWNL_UPGRD_MAN_CODE_ACCESS_START \n",COM_DOWNL_UPGRD_MAN_CODE_ACCESS_START);
    if( 0 != CommonDownloadGetCodeAccessStartTime((CodeAccessStart_t *)(&UpTime), COM_DOWNL_UPGRD_APP_MAN_CODE_ACCESS_START))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: Error!!!! Upating CommonDownloadGetCodeAccessStartTime,COM_DOWNL_UPGRD_APP_MAN_CODE_ACCESS_START\n");
        return -1;
    }
    
    timeCheckRet = checkTime(Time.Time, UpTime.Time);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," Check Time >>>>>>>>>>>>>>>>timeCheckRet:%d \n",timeCheckRet);
      if(timeCheckRet == 1)
      {
        //returns 0 if Time 1 = Time 2
        // returns 1 if Time1 < Time2
        // retunrs -1 if Time1 > Time2
        if( 0 != CommonDownloadSetCodeAccessStartTime((CodeAccessStart_t *)(&UpTime), COM_DOWNL_APP_MAN_CODE_ACCESS_START))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: Error!!!! Upating CommonDownloadSetCodeAccessStartTime,COM_DOWNL_APP_MAN_CODE_ACCESS_START\n");
            return -1;
        }
      }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: READ REG:0x%08X COM_DOWNL_UPGRD_MAN_CODE_ACCESS_START \n",COM_DOWNL_UPGRD_MAN_CODE_ACCESS_START);
    if( 0 != CommonDownloadGetCodeAccessStartTime((CodeAccessStart_t *)(&Time), COM_DOWNL_APP_COSIG_CODE_ACCESS_START))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: Error!!!! Upating CommonDownloadGetCodeAccessStartTime,COM_DOWNL_UPGRD_APP_COSIG_CODE_ACCESS_START\n");
        return -1;
    }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: READ REG:0x%08X COM_DOWNL_UPGRD_MAN_CODE_ACCESS_START \n",COM_DOWNL_UPGRD_MAN_CODE_ACCESS_START);
    if( 0 != CommonDownloadGetCodeAccessStartTime((CodeAccessStart_t *)(&UpTime), COM_DOWNL_UPGRD_APP_COSIG_CODE_ACCESS_START))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: Error!!!! Upating CommonDownloadGetCodeAccessStartTime,COM_DOWNL_UPGRD_APP_COSIG_CODE_ACCESS_START\n");
        return -1;
    }
    
    timeCheckRet = checkTime(Time.Time, UpTime.Time);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," Check Time >>>>>>>>>>>>>>>>timeCheckRet:%d \n",timeCheckRet);
      if(timeCheckRet == 1)
      {
        //returns 0 if Time 1 = Time 2
        // returns 1 if Time1 < Time2
        // retunrs -1 if Time1 > Time2
        if( 0 != CommonDownloadSetCodeAccessStartTime((CodeAccessStart_t *)(&UpTime), COM_DOWNL_APP_COSIG_CODE_ACCESS_START))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: Error!!!! Upating CommonDownloadSetCodeAccessStartTime,COM_DOWNL_APP_COSIG_CODE_ACCESS_START\n");
            return -1;
        }
      }



    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: READ REG:0x%08X COM_DOWNL_UPGRD_MAN_CODE_ACCESS_START \n",COM_DOWNL_UPGRD_MAN_CODE_ACCESS_START);
    if( 0 != CommonDownloadGetCodeAccessStartTime((CodeAccessStart_t *)(&Time), COM_DOWNL_DATA_MAN_CODE_ACCESS_START))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: Error!!!! Upating CommonDownloadGetCodeAccessStartTime,COM_DOWNL_DATA_MAN_CODE_ACCESS_START\n");
        return -1;
    }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: READ REG:0x%08X COM_DOWNL_UPGRD_MAN_CODE_ACCESS_START \n",COM_DOWNL_UPGRD_MAN_CODE_ACCESS_START);
    if( 0 != CommonDownloadGetCodeAccessStartTime((CodeAccessStart_t *)(&UpTime), COM_DOWNL_UPGRD_DATA_MAN_CODE_ACCESS_START))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: Error!!!! Upating CommonDownloadGetCodeAccessStartTime,COM_DOWNL_UPGRD_DATA_MAN_CODE_ACCESS_START\n");
        return -1;
    }
    
    timeCheckRet = checkTime(Time.Time, UpTime.Time);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," Check Time >>>>>>>>>>>>>>>>timeCheckRet:%d \n",timeCheckRet);
      if(timeCheckRet == 1)
      {
        //returns 0 if Time 1 = Time 2
        // returns 1 if Time1 < Time2
        // retunrs -1 if Time1 > Time2
        if( 0 != CommonDownloadSetCodeAccessStartTime((CodeAccessStart_t *)(&UpTime), COM_DOWNL_DATA_MAN_CODE_ACCESS_START))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: Error!!!! Upating CommonDownloadSetCodeAccessStartTime,COM_DOWNL_DATA_MAN_CODE_ACCESS_START\n");
            return -1;
        }
      }



    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: READ REG:0x%08X COM_DOWNL_UPGRD_MAN_CODE_ACCESS_START \n",COM_DOWNL_UPGRD_MAN_CODE_ACCESS_START);
    if( 0 != CommonDownloadGetCodeAccessStartTime((CodeAccessStart_t *)(&Time), COM_DOWNL_DATA_COSIG_CODE_ACCESS_START))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: Error!!!! Upating CommonDownloadGetCodeAccessStartTime,COM_DOWNL_DATA_COSIG_CODE_ACCESS_START\n");
        return -1;
    }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: READ REG:0x%08X COM_DOWNL_UPGRD_MAN_CODE_ACCESS_START \n",COM_DOWNL_UPGRD_MAN_CODE_ACCESS_START);
    if( 0 != CommonDownloadGetCodeAccessStartTime((CodeAccessStart_t *)(&UpTime), COM_DOWNL_UPGRD_DATA_COSIG_CODE_ACCESS_START))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: Error!!!! Upating CommonDownloadGetCodeAccessStartTime,COM_DOWNL_UPGRD_DATA_COSIG_CODE_ACCESS_START\n");
        return -1;
    }
    
    timeCheckRet = checkTime(Time.Time, UpTime.Time);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," Check Time >>>>>>>>>>>>>>>>timeCheckRet:%d \n",timeCheckRet);
      if(timeCheckRet == 1)
      {
        //returns 0 if Time 1 = Time 2
        // returns 1 if Time1 < Time2
        // retunrs -1 if Time1 > Time2
        if( 0 != CommonDownloadSetCodeAccessStartTime((CodeAccessStart_t *)(&UpTime), COM_DOWNL_DATA_COSIG_CODE_ACCESS_START))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownLUpdateCodeAccessTime: Error!!!! Upating CommonDownloadSetCodeAccessStartTime,COM_DOWNL_DATA_COSIG_CODE_ACCESS_START\n");
            return -1;
        }
    }

    return 0;
}
#endif

#ifdef PKCS_UTILS
int CCodeImageValidator::getRootCACert(void *code, int len, void **cert, int *certLen)
{
  return pkcs7getRootCACert(code, len, cert, certLen);
}

int CCodeImageValidator::getBaseCACert(void *code, int len, void **cert, int *certLen)
{
  return pkcs7getBaseCACert(code, len, cert, certLen);
}

int CCodeImageValidator::getMfrCACert(void *code, int len, void **cert, int *certLen)
{
  return pkcs7getMfrCACert(code, len, cert, certLen);
}
#endif
