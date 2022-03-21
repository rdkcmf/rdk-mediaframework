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


#include "cdownloadcvtvalidationmgr.h"
#include "pkcs7utils.h"
#include "cvtdownloadtypes.h"
#include <CommonDownloadNVDataAccess.h>
#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "vlpluginapp_halcdlapi.h"
#include "coreUtilityApi.h"

#if USE_SYSRES_MLT
#include "rpl_malloc.h"
#endif

CDownloadCVTValidationMgr::CDownloadCVTValidationMgr() 
{

}

CDownloadCVTValidationMgr::~CDownloadCVTValidationMgr() {
  
}
static void PrintBuff(unsigned char *pData, int Size)
{
    int ii;
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","\nVIVIDCvc[size:%d]={ \n",Size);
    for(ii = 0; ii < Size; ii++)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","0x%02X, ",pData[ii]);
        if( ((ii+1) % 16) == 0)
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","\n");
        }
    }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","\n};\n");
}
 static int vlGetNumOfCvcs(unsigned char *pData, int len, int *pCvcNum)
{

    unsigned char *pTemp,*pCvc,CvtType;
    unsigned long Length,CvcLen;
    int ii=0;

    *pCvcNum = 0;
    Length = len;
    pTemp = pData;

    
    while(1)
    {
        if(len < 5)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","vlGetNumOfCvcs: len:%d < 5 Error !! \n",len);
            return -1;
        }
        
        CvtType = pData[0];
        CvcLen  = (unsigned long)( ((unsigned long)pData[3] << 8) | pData[4] );
        CvcLen += 4;
        if(len < CvcLen)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","vlGetNumOfCvcs: Error in CVCs Length:%d len:%d CvcLen:%d\n",Length,len,CvcLen);
            return -1;
        }
        ii++;
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","vlGetNumOfCvcs:CvtType[%d]:0x%02X CvcLen[%d]:%d\n",ii,CvtType,ii,CvcLen);
        len = len - (int)CvcLen - 1;
        if(len <= 0)
            break;
        
         pData = pData + CvcLen + 1;
    }
    *pCvcNum = ii;
    return 0;
}
int CDownloadCVTValidationMgr::processCVTTrigger(TableCVT *cvt)
{
    unsigned char     *mfrName=NULL;
      unsigned char     *coName = NULL;
      cvcinfo_t         cvcInfos[4],cvcInfosCheck[4];
      cvc_data_t        cvc_data;
      cvcAccessStart_t  currentCvcAccessStart;
      int               mfrIndex = -1;
      int               coIndex = -1;
      int               i;
      unsigned char     *key;
      unsigned long     keySize;
      unsigned char     *cvcRoot,*cvcCA,*ManCvc;
      unsigned long     cvcRootLen,cvcCALen,ManCvcLen;
      unsigned char     *cvcRootKey,*cvcCAKey;
      int     cvcRootKeyLen,cvcCAKeyLen;
      unsigned char *current;
      int currentLen;
      int num_cvcs = 0;
      int cvcMask       = 0;
      int  numberOfCvcs = 0;
      cvcinfo_t    cvcRootInfo;
      cvcinfo_t    cvcCAInfo;

    mfrName = CommonDownloadGetManufacturerName();
    if((mfrName == NULL) )
       {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownloadCVTValidationMgr:Error NULL pointer retunred from mfrName == %X || coName == %X \n",mfrName,coName);
        return -1;
       }
    
    if(mfrName)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","mfrName:%s \n",mfrName);
    }
      coName = CommonDownloadGetCosignerName();
    if(coName == NULL)
    {
        return -1;
    }
      if(coName)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","coName:%s \n",coName);
    }
       
       bzero(cvcInfos, sizeof(cvcinfo_t)*4);
    
      // get CVC/s from CVT.
      switch(cvt->getCVT_Type())
      {
        case CVT_T1V1:
        {
        int currentType,iRet;
        cvt_t1v1_download_data_t *cvtData = (cvt_t1v1_download_data_t*)cvt->getCVT_Data();
        cvc_data = cvtData->cvc_data;
              current = cvc_data.data;
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," >>>>>>>>> CDownloadCVTValidationMgr: CVT 1 cvc_data.len:%d \n",cvc_data.len);
        vlMpeosDumpBuffer(RDK_LOG_INFO, "LOG.RDK.CDL", cvc_data.data,cvc_data.len);
        iRet = vlGetNumOfCvcs(cvc_data.data, int(cvc_data.len),&numberOfCvcs);
        if(iRet != 0)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownloadCVTValidationMgr: CVT1 Error returned from vlGetNumOfCvcs \n");
            free(cvtData);
            return -1;
        }
        
        if((cvc_data.len == 0) || (numberOfCvcs == 0) )
        {     
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownloadCVTValidationMgr::%s: NO Cvcs found !! ERROR !!\n", __FUNCTION__);
            free(cvtData);
            return -1;
            
        }
    
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," CDownloadCVTValidationMgr: Number of CVCs:%d  \n",numberOfCvcs);
        while(num_cvcs < numberOfCvcs)
        {
            currentType = current[0];
            currentLen = (current[3] << 8) | current[4];
    //////////////////////////////////
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Cvc From CVT1 Len:%d\n",currentLen + 4);
            //PrintBuff((current + 1), (currentLen + 4));
    /////////////////////////////////////
#ifdef PKCS_UTILS    
            if(getCVCInfoFromNonSignedData(current + 1, currentLen + 4, &cvcInfos[num_cvcs]) < 0)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownoadCVTValidationMgr::%s: Failed to find a CVT1 CVC\n", __FUNCTION__);
                free(cvtData);
                return -1;
            }
#endif            
            switch(currentType)
            {
                case 0: // Mfr CVC
                {
                    if(strcmp((const char*)mfrName, (const char*)cvcInfos[num_cvcs].subjectOrgName) != 0)
                    {
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownloadCVTValidationMgr::%s: CVT1 CVC name [%s] does not match Mfr Name [%s]\n",
                        __FUNCTION__, cvcInfos[num_cvcs].subjectOrgName, mfrName);
                        free(cvtData);
                        return -1;
                    }
        
                    mfrIndex = num_cvcs;
                    cvcMask != MFR_CVC;
                    break;
                }
                case 1: // CoSigner CVC
                {
                    if(strcmp((const char*)coName, (const char*)cvcInfos[num_cvcs].subjectOrgName) != 0)
                    {
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownloadCVTValidationMgr::%s:CVT1 CVC name [%s] does not match Mfr Name [%s]\n",
                        __FUNCTION__, cvcInfos[num_cvcs].subjectOrgName, coName);
                        free(cvtData);
                        return -1;
                    }
        
                    coIndex = num_cvcs;
                    cvcMask != CO_CVC;
                    break;
                }
                default:
                {
                    // deside what VVL wants to do in the case.
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownloadCVTValidationMgr::%s: Unknown CVT1 CVC type of %d\n", __FUNCTION__, currentType);    
                    free(cvtData);
                    return -1;
                }
            }//switch
            current = current + currentLen + 4 + 1;
            num_cvcs++;
        }//while
        free(cvtData);
              break;
        }
        case CVT_T2Vx:
        {
        if( cvt->getCVT_Version() == 1)
        {
            int currentType;
            cvt_t2v1_download_data_t *cvtData = (cvt_t2v1_download_data_t*)cvt->getCVT_Data();
            cvc_data = cvtData->cvc_data;
            current = cvc_data.data;
            numberOfCvcs = cvtData->num_cvcs;
            if(cvc_data.len == 0)
            {     
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownloadCVTValidationMgr::%s: NO Cvcs found !! ERROR !!\n", __FUNCTION__);
                free(cvtData);
                return -1;
            
            }
    
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," CDownloadCVTValidationMgr: Number of CVCs:%d  \n",numberOfCvcs);
            while(num_cvcs < cvtData->num_cvcs)
            {
                currentType = current[0];
                currentLen = (current[3] << 8) | current[4];
    //////////////////////////////////
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Cvc From CVT Len:%d\n",currentLen + 4);
                //PrintBuff((current + 1), (currentLen + 4));
    /////////////////////////////////////
#ifdef PKCS_UTILS    
                if(getCVCInfoFromNonSignedData(current + 1, currentLen + 4, &cvcInfos[num_cvcs]) < 0)
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownoadCVTValidationMgr::%s: Failed to find a CVC\n", __FUNCTION__);
                        free(cvtData);
                    return -1;
                }
#endif                
                switch(currentType)
                {
                    case 0: // Mfr CVC
                    {
                        if(strcmp((const char*)mfrName, (const char*)cvcInfos[num_cvcs].subjectOrgName) != 0)
                        {
                            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CDownloadCVTValidationMgr::%s: CVC name [%s] does not match Mfr Name [%s]\n",
                            __FUNCTION__, cvcInfos[num_cvcs].subjectOrgName, mfrName);
                                    free(cvtData);
                            return -1;
                        }
        
                        mfrIndex = num_cvcs;
                        cvcMask != MFR_CVC;
                        break;
                    }
                    case 1: // CoSigner CVC
                    {
                        if(strcmp((const char*)coName, (const char*)cvcInfos[num_cvcs].subjectOrgName) != 0)
                        {
                            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownloadCVTValidationMgr::%s: CVC name [%s] does not match Mfr Name [%s]\n",
                            __FUNCTION__, cvcInfos[num_cvcs].subjectOrgName, coName);
                                    free(cvtData);
                            return -1;
                        }
        
                        coIndex = num_cvcs;
                        cvcMask != CO_CVC;
                                
                        break;
                    }
                    default:
                    {
                    // deside what VVL wants to do in the case.
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownloadCVTValidationMgr::%s: Unknown CVC type of %d\n", __FUNCTION__, currentType);
                        free(cvtData);    
                        return -1;
                    }
                }//switch
        
                current = current + currentLen + 4 + 1;
                num_cvcs++;
            }//while
            free(cvtData);
        }
        else if( cvt->getCVT_Version() == 2)
        {
            int currentType;
            cvt_t2v2_download_data_t *cvtData = (cvt_t2v2_download_data_t*)cvt->getCVT_Data();
            cvc_data = cvtData->cvc_data;
            current = cvc_data.data;
            numberOfCvcs = cvtData->num_cvcs;
            if(cvc_data.len == 0)
            {     
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownloadCVTValidationMgr::%s: NO Cvcs found !! ERROR !!\n", __FUNCTION__);
                if(cvtData->cvc_data.data != NULL)
                {
                    free(cvtData->cvc_data.data);

                    cvtData->cvc_data.data = NULL;
                }
                free(cvtData);
                return -1;
            
            }
    
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," CDownloadCVTValidationMgr: Number of CVCs:%d  \n",numberOfCvcs);
            while(num_cvcs < cvtData->num_cvcs)
            {
                currentType = current[0];
                currentLen = (current[3] << 8) | current[4];
    //////////////////////////////////
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Cvc From CVT Len:%d\n",currentLen + 4);
                //PrintBuff((current + 1), (currentLen + 4));
    /////////////////////////////////////
#ifdef PKCS_UTILS    
                if(getCVCInfoFromNonSignedData(current + 1, currentLen + 4, &cvcInfos[num_cvcs]) < 0)
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownoadCVTValidationMgr::%s: Failed to find a CVC\n", __FUNCTION__);
                        free(cvtData);
                    return -1;
                }
#endif                
                switch(currentType)
                {
                    case 0: // Mfr CVC
                    {
                        if(strcmp((const char*)mfrName, (const char*)cvcInfos[num_cvcs].subjectOrgName) != 0)
                        {
                            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownloadCVTValidationMgr::%s: CVC name [%s] does not match Mfr Name [%s]\n",
                            __FUNCTION__, cvcInfos[num_cvcs].subjectOrgName, mfrName);
                                    free(cvtData);
                            return -1;
                        }
        
                        mfrIndex = num_cvcs;
                        cvcMask != MFR_CVC;
                        break;
                    }
                    case 1: // CoSigner CVC
                    {
                        if(strcmp((const char*)coName, (const char*)cvcInfos[num_cvcs].subjectOrgName) != 0)
                        {
                            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownloadCVTValidationMgr::%s: CVC name [%s] does not match Mfr Name [%s]\n",
                            __FUNCTION__, cvcInfos[num_cvcs].subjectOrgName, coName);
                                    free(cvtData);
                            return -1;
                        }
        
                        coIndex = num_cvcs;
                        cvcMask != CO_CVC;
                                
                        break;
                    }
                    default:
                    {
                    // deside what VVL wants to do in the case.
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownloadCVTValidationMgr::%s: Unknown CVC type of %d\n", __FUNCTION__, currentType);    
                        free(cvtData);
                        return -1;
                    }
                }//switch
        
                current = current + currentLen + 4 + 1;
                num_cvcs++;
            }//while
            free(cvtData);
        }
              break;
        }//case CVT_T2Vx:
         default:
         {
           RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownloadCVTValidationMgr::%s: Unknown CVT type of %d\n", __FUNCTION__, cvt->getCVT_Type());
        return -1;
         }
    
      }//switch(cvt->getCVT_Type())
  // verify Mfr CVC in codeFile SignedData
    // check OrgName is identical to Mfr name in Host memory
    // check CVC validity start time is >= cvcAccessTime held in host
    // check extendedKeyUsage in CVC includes OID ip-kp-codeSigning
        if(mfrIndex == -1 && coIndex == -1)
           {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownloadCVTValidationMgr::%s: Failed to find a CVC for Mfr %s!\n", __FUNCTION__, mfrName);
                return -1;
           }
    
    
    if(mfrIndex != -1 )
    {
          if(cvcInfos[mfrIndex].codeSigningExists != 1)
          {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownloadCVTValidationMgr::%s: Mfr CVC fails extendedKeyUsage test!\n", __FUNCTION__);
                return -1;
          }
    }


#if 1
      RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CDownloadCVTValidationMgr: Going to Verify CLCvcCA\n");
      if(CommonDownloadGetCLCvcRootCA(&cvcRoot, &cvcRootLen))
      {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownloadCVTValidationMgr::%s: Failed to get CL Root CVC from NVM!\n", __FUNCTION__);
            return -1;
      }
      RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CL CVC ROOT cvcRootLen:%d\n",cvcRootLen);
#ifdef PKCS_UTILS
    //PrintBuff(cvcRoot, cvcRootLen);
      if(getPublicKeyFromCVC(cvcRoot, cvcRootLen, &cvcRootKey, &cvcRootKeyLen))
      {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownloadCVTValidationMgr::%s: Failed to get PubKey from CL Root CVC in NVM!\n", __FUNCTION__);
            free( cvcRoot );		
            return -1;
      }
#endif

#endif
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Reading CL CVC CA \n");
      if(CommonDownloadGetCLCvcCA(&cvcCA, &cvcCALen))
      {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownloadCVTValidationMgr::%s: Failed to get CL Root CVC from NVM!\n", __FUNCTION__);
            return -1;
      }
RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CL CVC CA Len:%d\n",cvcCALen);
//PrintBuff(cvcCA, cvcCALen);

#ifdef PKCS_UTILS
      if(validateCertSignature(cvcCA, cvcCALen, cvcRootKey, cvcRootKeyLen))
      {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownloadCVTValidationMgr::%s: CVC data failed to validate to CL CVC Root PublicKey!\n", __FUNCTION__);
            free( cvcRoot );
            free( cvcCA );		
            return -1;
      }
       RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," ##################################################\n");
       RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," >>>> SUCESS verifying CLCVCCA signature <<<<<<<< \n");
       RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," ##################################################\n");

       if(getCVCInfoFromNonSignedData(cvcRoot, cvcRootLen, &cvcRootInfo) < 0)
       {
             RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownloadCVTValidationMgr::%s: Failed to get CVC Root Information!\n", __FUNCTION__);
            free( cvcRoot );
             free( cvcCA );		
             return -1;
       }
#endif

#ifdef PKCS_UTILS
       if(getCVCInfoFromNonSignedData(cvcCA, cvcCALen, &cvcCAInfo) < 0)
       {
             RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownloadCVTValidationMgr::%s: Failed to get CVC CA Information!\n", __FUNCTION__);
             free( cvcRoot );
             free( cvcCA );	
             return -1;
       }

       if(strcmp((const char*)cvcRootInfo.subjectOrgName, (const char*)cvcCAInfo.issuerOrgName) != 0)
       {     
             RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownladCVTValidationMgr::%s:  CVC CA failed to chain to the CVC Root via OrgNames!\n", __FUNCTION__);
             free( cvcRoot );
             free( cvcCA );	
             return -1;
       }

       if(strcmp((const char*)cvcRootInfo.subjectCommonName, (const char*)cvcCAInfo.issuerCommonName) != 0)
       {
             RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownladCVTValidationMgr::%s:  CVC CA failed to chain to the CVC Root via CommonNames!\n", __FUNCTION__);
             free( cvcRoot );
             free( cvcCA );	
             return -1;
       }
#endif
       RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","#########################################################################\n");
       RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CDownladCVTValidationMgr:CVC ROOT and CVC CA Link Verification is SUCCESS \n");
       RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","##########################################################################\n");
    // Validate CVC signature via the CL CVC CA Public Key held in device
        // If fail, send invalid cert event
#if 0
       if(CommonDownloadGetCvcCAPublicKey(&key, &keySize) < 0)
       {
             RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownloadCVTValidationMgr::%s: Failed to get CVC CA PubKey from NVM!\n", __FUNCTION__);
             return -1;
       }
#endif
#ifdef PKCS_UTILS
    if(getPublicKeyFromCVC(cvcCA, cvcCALen, &cvcCAKey, &cvcCAKeyLen))
      {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownloadCVTValidationMgr::%s: Failed to get PubKey from CL Root CVC in NVM!\n", __FUNCTION__);
             free( cvcRoot );
             free( cvcCA );	
            return -1;
      }
#endif    
      current = cvc_data.data;
      num_cvcs = 0;
    bzero(cvcInfosCheck, sizeof(cvcinfo_t)*4);
      while(num_cvcs < numberOfCvcs)
      {
              currentLen = (current[3] << 8) | current[4];
#ifdef PKCS_UTILS        
         if(getCVCInfoFromNonSignedData(current + 1, currentLen + 4, &cvcInfosCheck[num_cvcs]) < 0)
            {
                  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownloadCVTValidationMgr::%s: Failed to find a CVC\n", __FUNCTION__);
                  free( cvcRoot );
                  free( cvcCA );	
                  return -1;
            }
        

               if( validateCertSignature(current + 1, currentLen + 4, cvcCAKey, cvcCAKeyLen) )
               {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownloadCVTValidationMgr::%s: Failed to validate CertSignature with stored key!\n", __FUNCTION__);
                free( cvcRoot );
                free( cvcCA );	
                return -1;
            }
#endif        
        VL_CDL_CV_CERTIFICATE eCmCvc;
        int iRet;

        eCmCvc.eCvcType = VL_CDL_CVC_TYPE_MANUFACTURER;
        eCmCvc.cvCertificate.nBytes = currentLen + 4;
        eCmCvc.cvCertificate.pData = current + 1;
        vlMpeosDumpBuffer(RDK_LOG_INFO, "LOG.RDK.CDL", eCmCvc.cvCertificate.pData,eCmCvc.cvCertificate.nBytes);
        iRet = CHALCdl_notify_cdl_mgr_event(VL_CDL_MANAGER_EVENT_SET_CV_CERTIFICATE, (unsigned long )&eCmCvc);
        if(VL_CDL_RESULT_SUCCESS != iRet )
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","#######################  VL_CDL_MANAGER_EVENT_SET_CV_CERTIFICATE Failed with Error:0x%X \n",iRet);
            
        }

        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","#######################################################################\n");
           RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," CDownladCVTValidationMgr:Received CVC Signature Verification is SUCCESS \n");
           RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","########################################################################\n");
#if 1
             if(strcmp((const char*)cvcCAInfo.subjectOrgName, (const char*)(cvcInfosCheck[num_cvcs].issuerOrgName)) != 0)
             {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownladCVTValidationMgr::%s:  CVC CA failed to chain to the CVC Root via OrgNames!\n", __FUNCTION__);
                    free( cvcRoot );
                    free( cvcCA );	
                    return -1;
              }

             if(strcmp((const char*)cvcCAInfo.subjectCommonName, (const char*)(cvcInfosCheck[num_cvcs].issuerCommonName)) != 0)
             {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownladCVTValidationMgr::%s:  CVC CA failed to chain to the CVC Root via CommonNames!\n", __FUNCTION__);
                    free( cvcRoot );
                    free( cvcCA );	
                    return -1;
                   }
#endif
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","######################################################\n");
           RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CDownladCVTValidationMgr:CVC  Link Verification is SUCCESS \n");
           RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","######################################################\n");
        current = current + currentLen + 4 + 1;
        num_cvcs++;
      }//while(num_cvcs < cvtData->num_cvcs)

     RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","############################################################\n");
       RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CDownladCVTValidationMgr:Complete CVC  Verification is SUCCESS \n");
       RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","############################################################\n");
    if(CommonDownloadGetCvcAccessStartTime(&currentCvcAccessStart, COM_DOWNL_MAN_CVC) != 0)
      {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownloadCVTValidationMgr::%s Failed to get the current CVCAccessStart time!\n", __FUNCTION__);
            free( cvcRoot );
            free( cvcCA );	
            return -1;
      }

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CDownloadCVTValidationMgr: Checking the time \n");
    if(mfrIndex != -1 )
      {
        cvcAccessStart_t cvcTime;
#ifdef PKCS_UTILS        
               if(checkTime( currentCvcAccessStart.Time,cvcInfos[mfrIndex].notBefore) < 0)
          {
        //returns 0 if Time 1 = Time 2
        // returns 1 if Time1 < Time2
        // retunrs -1 if Time1 > Time2
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownloadCVTValidationMgr::%s: Mfr CVC fails cvcAccessStart time test!\n", __FUNCTION__);
                //return -1;
          }
#endif        
        memcpy(cvcTime.Time,cvcInfos[mfrIndex].notBefore,12);
        // Update time varying controls
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CDownloadCVTValidationMgr: Calling CommonDownloadSetCvcAccessStartTime COM_DOWNL_MAN_CVC \n");
        CommonDownloadSetCvcAccessStartTime((cvcAccessStart_t *)(&cvcTime), COM_DOWNL_MAN_CVC);
      }

      if(coIndex != -1)
      {
        cvcAccessStart_t cvcTime;
#ifdef PKCS_UTILS        
            if(checkTime(currentCvcAccessStart.Time,cvcInfos[coIndex].notBefore ) < 0)
            {
            //returns 0 if Time 1 = Time 2
        // returns 1 if Time1 < Time2
        // retunrs -1 if Time1 > Time2
                  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownloadCVTValidationMgr::%s: Co CVC fails cvcAccessStart time test!\n", __FUNCTION__);
                  free( cvcRoot );
                  free( cvcCA );	
                  return -1;
            }
#endif        
            memcpy(cvcTime.Time,cvcInfos[coIndex].notBefore,12);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CDownloadCVTValidationMgr: Calling CommonDownloadSetCvcAccessStartTime COM_DOWNL_COSIG_CVC \n");
            CommonDownloadSetCvcAccessStartTime((cvcAccessStart_t *)(&cvcTime), COM_DOWNL_COSIG_CVC);
      }
#ifdef PKCS_UTILS    
      setCVCUsage(cvcMask);
#endif    
      free( cvcRoot );
      free( cvcCA );	
      return 0;
}
