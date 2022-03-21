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


//#include "pfcresource.h"
#include "cardUtils.h"
#include "cmhash.h"
#include "rmf_osal_event.h"
#include "sys_init.h"
//#include "pfceventmpeg.h"
#include "core_events.h"
#include "cm_api.h"
#include "capmtparse.h"
#include "rmfqamsrc.h"
#include <rdk_debug.h>
#include "rmf_osal_mem.h"
#include "vlEnv.h"

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

#define __MTAG__ VL_CARD_MANAGER

#if 0
Program Number: 1
PcrPid        :0x43
VideoPid      :0x43
AudioPid      :0x44   

02 b0 3a 00 01 dd 00 00 e0 43 f0 00 80 e0 43 f0
12 83 01 00 86 0d e2 65 6e 67 7e 3f ff 65 6e 67
c1 3f ff 81 e0 44 f0 06 0a 04 65 6e 67 00 81 e0
45 f0 06 0a 04 73 70 61 00 00 00 00 00

Program Number: 2
PcrPid        :0x40
VideoPid      :0x40
AudioPid      :0x41   

02 b0 3a 00 02 cf 00 00 e0 40 f0 00 80 e0 40 f0
12 83 01 00 86 0d e2 65 6e 67 7e 3f ff 65 6e 67
c1 3f ff 81 e0 41 f0 06 0a 04 65 6e 67 00 81 e0
42 f0 06 0a 04 73 70 61 00 00 00 00 00

Program Number: 3
PcrPid        :0x48
VideoPid      :0x48
AudioPid      :0x49   

02 b0 3a 00 03 c9 00 00 e0 48 f0 00 80 e0 48 f0
12 83 01 00 86 0d e2 65 6e 67 7e 3f ff 65 6e 67
c1 3f ff 81 e0 49 f0 06 0a 04 65 6e 67 00 81 e0
4a f0 06 0a 04 73 70 61 00 00 00 00 00

Program Number: 4
PcrPid        :0x4b
VideoPid      :0x4b
AudioPid      :0x4c   

02 b0 3a 00 04 f3 00 00 e0 4b f0 00 80 e0 4b f0
12 83 01 00 86 0d e2 65 6e 67 7e 3f ff 65 6e 67
c1 3f ff 81 e0 4c f0 06 0a 04 65 6e 67 00 81 e0
4d f0 06 0a 04 73 70 61 00 00 00 00 00

Program Number: 5
PcrPid        :0x4e
VideoPid      :0x4e
AudioPid      :0x4f   


02 b0 3a 00 05 f7 00 00 e0 4e f0 00 80 e0 4e f0
12 83 01 00 86 0d e2 65 6e 67 7e 3f ff 65 6e 67
c1 3f ff 81 e0 4f f0 06 0a 04 65 6e 67 00 81 e0
50 f0 06 0a 04 73 70 61 00 00 00 00 00

Program Number: 6
PcrPid        :0x51
VideoPid      :0x51
AudioPid      :0x52   

02 b0 55 00 06 cf 00 00 e0 51 f0 00 80 e0 51 f0
1b 09 07 0e 00 e0 96 01 01 01 83 01 00 86 0d e2
65 6e 67 7e 3f ff 65 6e 67 c1 3f ff 81 e0 52 f0
0f 09 07 0e 00 e0 96 01 01 02 0a 04 65 6e 67 00
81 e0 53 f0 0f 09 07 0e 00 e0 96 01 01 03 0a 04
73 70 61 00 00 00 00 00

Program Number: 7
PcrPid        :0x54
VideoPid      :0x54
AudioPid      :0x55   

02 b0 55 00 07 d5 00 00 e0 54 f0 00 80 e0 54 f0
1b 09 07 0e 00 e0 a6 01 01 01 83 01 00 86 0d e2
65 6e 67 7e 3f ff 65 6e 67 c1 3f ff 81 e0 55 f0
0f 09 07 0e 00 e0 a6 01 01 02 0a 04 65 6e 67 00
81 e0 58 f0 0f 09 07 0e 00 e0 a6 01 01 03 0a 04
73 70 61 00 00 00 00 00

Program Number: 8
PcrPid        :0x59
VideoPid      :0x59
AudioPid      :0x5a   

02 b0 44 00 08 d7 00 00 e0 59 f0 00 80 e0 59 f0
12 83 01 7f 86 0d e2 65 6e 67 7e 3f ff 65 6e 67
c1 3f ff 81 e0 5a f0 0b 0a 04 65 6e 67 00 81 03
08 c8 1b 81 e0 5b f0 0b 0a 04 65 6e 67 00 81 03
08 c8 1b 00 00 00 00

Program Number: 10
PcrPid        :0x5c
VideoPid      :0x5c
AudioPid      :0x5d   

02 b0 3a 00 0a df 00 00 e0 5c f0 00 80 e0 5c f0
12 83 01 00 86 0d e2 65 6e 67 7e 3f ff 65 6e 67
c1 3f ff 81 e0 5d f0 06 0a 04 65 6e 67 00 81 e0
5e f0 06 0a 04 73 70 61 00 00 00 00 00

#endif 


static unsigned short VidPid1 = 0x43;
static unsigned short AudPid1 = 0x44;
static unsigned char Pmt1[] = {0x02,0xb0 ,0x3a ,0x00 ,0x01 ,0xdd ,0x00 ,0x00 ,0xe0 ,0x43 ,0xf0 ,0x00 ,0x80 ,0xe0 ,0x43 ,0xf0,
            0x12,0x83 ,0x01 ,0x00 ,0x86 ,0x0d ,0xe2 ,0x65 ,0x6e ,0x67 ,0x7e ,0x3f ,0xff ,0x65 ,0x6e ,0x67,
            0xc1,0x3f ,0xff ,0x81 ,0xe0 ,0x44 ,0xf0 ,0x06 ,0x0a ,0x04 ,0x65 ,0x6e ,0x67 ,0x00 ,0x81 ,0xe0,
            0x45,0xf0 ,0x06 ,0x0a ,0x04 ,0x73 ,0x70 ,0x61 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 };
            
static unsigned short VidPid2 = 0x40;
static unsigned short AudPid2 = 0x41;
static unsigned char Pmt2[] =    {0x02 ,0xb0 ,0x3a ,0x00 ,0x02 ,0xcf ,0x00 ,0x00 ,0xe0 ,0x40 ,0xf0 ,0x00 ,0x80 ,0xe0 ,0x40 ,0xf0
,0x12 ,0x83 ,0x01 ,0x00 ,0x86 ,0x0d ,0xe2 ,0x65 ,0x6e ,0x67 ,0x7e ,0x3f ,0xff ,0x65 ,0x6e ,0x67
,0xc1 ,0x3f ,0xff ,0x81 ,0xe0 ,0x41 ,0xf0 ,0x06 ,0x0a ,0x04 ,0x65 ,0x6e ,0x67 ,0x00 ,0x81 ,0xe0
,0x42 ,0xf0 ,0x06 ,0x0a ,0x04 ,0x73 ,0x70 ,0x61 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00};

static unsigned short VidPid3 = 0x48;
static unsigned short AudPid3 = 0x49;
static unsigned char Pmt3[] =    {0x02 ,0xb0 ,0x3a ,0x00 ,0x03 ,0xc9 ,0x00 ,0x00 ,0xe0 ,0x48 ,0xf0 ,0x00 ,0x80 ,0xe0 ,0x48 ,0xf0
,0x12 ,0x83 ,0x01 ,0x00 ,0x86 ,0x0d ,0xe2 ,0x65 ,0x6e ,0x67 ,0x7e ,0x3f ,0xff ,0x65 ,0x6e ,0x67
,0xc1 ,0x3f ,0xff ,0x81 ,0xe0 ,0x49 ,0xf0 ,0x06 ,0x0a ,0x04 ,0x65 ,0x6e ,0x67 ,0x00 ,0x81 ,0xe0
,0x4a ,0xf0 ,0x06 ,0x0a ,0x04 ,0x73 ,0x70 ,0x61 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00};

static unsigned short VidPid4 = 0x4b;
static unsigned short AudPid4 = 0x4c;
static unsigned char Pmt4[] =    {0x02 ,0xb0 ,0x3a ,0x00 ,0x04 ,0xf3 ,0x00 ,0x00 ,0xe0 ,0x4b ,0xf0 ,0x00 ,0x80 ,0xe0 ,0x4b ,0xf0
,0x12 ,0x83 ,0x01 ,0x00 ,0x86 ,0x0d ,0xe2 ,0x65 ,0x6e ,0x67 ,0x7e ,0x3f ,0xff ,0x65 ,0x6e ,0x67
,0xc1 ,0x3f ,0xff ,0x81 ,0xe0 ,0x4c ,0xf0 ,0x06 ,0x0a ,0x04 ,0x65 ,0x6e ,0x67 ,0x00 ,0x81 ,0xe0
,0x4d ,0xf0 ,0x06 ,0x0a ,0x04 ,0x73 ,0x70 ,0x61 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00};

static unsigned short VidPid5 = 0x4e;
static unsigned short AudPid5 = 0x4f;
static unsigned char Pmt5[] =    {0x02 ,0xb0 ,0x3a ,0x00 ,0x05 ,0xf7 ,0x00 ,0x00 ,0xe0 ,0x4e ,0xf0 ,0x00 ,0x80 ,0xe0 ,0x4e ,0xf0
,0x12 ,0x83 ,0x01 ,0x00 ,0x86 ,0x0d ,0xe2 ,0x65 ,0x6e ,0x67 ,0x7e ,0x3f ,0xff ,0x65 ,0x6e ,0x67
,0xc1 ,0x3f ,0xff ,0x81 ,0xe0 ,0x4f ,0xf0 ,0x06 ,0x0a ,0x04 ,0x65 ,0x6e ,0x67 ,0x00 ,0x81 ,0xe0
,0x50 ,0xf0 ,0x06 ,0x0a ,0x04 ,0x73 ,0x70 ,0x61 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00};

static unsigned short VidPid6 = 0x51;
static unsigned short AudPid6 = 0x52;
static unsigned char Pmt6[] =    {0x02 ,0xb0 ,0x55 ,0x00 ,0x06 ,0xcf ,0x00 ,0x00 ,0xe0 ,0x51 ,0xf0 ,0x00 ,0x80 ,0xe0 ,0x51 ,0xf0
,0x1b ,0x09 ,0x07 ,0x0e ,0x00 ,0xe0 ,0x96 ,0x01 ,0x01 ,0x01 ,0x83 ,0x01 ,0x00 ,0x86 ,0x0d ,0xe2
,0x65 ,0x6e ,0x67 ,0x7e ,0x3f ,0xff ,0x65 ,0x6e ,0x67 ,0xc1 ,0x3f ,0xff ,0x81 ,0xe0 ,0x52 ,0xf0
,0x0f ,0x09 ,0x07 ,0x0e ,0x00 ,0xe0 ,0x96 ,0x01 ,0x01 ,0x02 ,0x0a ,0x04 ,0x65 ,0x6e ,0x67 ,0x00
,0x81 ,0xe0 ,0x53 ,0xf0 ,0x0f ,0x09 ,0x07 ,0x0e ,0x00 ,0xe0 ,0x96 ,0x01 ,0x01 ,0x03 ,0x0a ,0x04
,0x73 ,0x70 ,0x61 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00};
    
extern unsigned char GetPodInsertedStatus();
extern void vlCardCaPmtMutexBegin();
extern void vlCardCaPmtMutexEnd();

static void freeCAPmtTbl(void* ptr)
{
    
    struct vlcaDetailInfo *pCaDetails = (struct vlcaDetailInfo*)ptr;
    if(NULL == pCaDetails)
        return;

#if 0// commenting the code because we are going to delete the object
    struct vl_pmt_tbl *tbl = pCaDetails->pPmtTable;

    if(tbl)
    {
        if(tbl->desc)
        {
            //VLFREE_INFO2(tbl->desc);
            delete tbl->desc;
        }

        if(tbl->streams)
        {
            for (int i = 0; i < tbl->num_streams; i++)
            {
                if (tbl->streams[i].desc)
                {
                    //VLFREE_INFO2(tbl->streams[i].desc);
                    delete tbl->streams[i].desc;
            }
            }
            //VLFREE_INFO2(tbl->streams);
            delete [] tbl->streams;
        }
        //VLFREE_INFO2(tbl);
        delete tbl;
    }
#endif
    
    if(pCaDetails->pPmtObj)
    {
        //VLFREE_INFO2(pCaDetails->pPmtObj);
        delete pCaDetails->pPmtObj;
    }
    
    //VLFREE_INFO2(pCaDetails);
    delete pCaDetails;
    return;
}
//extern rmf_Error vlMpeosImpl_TunerGetLTSID(int tunerId, unsigned char *pltsid);
#if 0
int vlCardSendCAPmt(int TunerHandle, unsigned char *pPMT, unsigned long uPMTSize,int* pCardResourceHandle )
{
   // unsigned long Ltsid = 16;
    unsigned char Ltsid = 0;
    vlTablePMT *PmtObj;

    struct vl_pmt_tbl* pPmtTable = NULL;
    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    cCardManagerIF *pCardMgrIf;
    int cardResourceHandle = -1;
    int ccMgrRc;
    bool bSendCADetailsToCard = false;
  
    
    if((pPMT == NULL) || (uPMTSize == 0) || (pCardResourceHandle == NULL) )
    {
      return -1;
    }
    PmtObj = new vlTablePMT();

    if(PmtObj == NULL)
    {
      return -1;
    }
    if( 0 != PmtObj->ParseData(pPMT, uPMTSize))
    {
        //VLFREE_INFO2(PmtObj);
      delete PmtObj;
      return -1;
    }
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," >>>>>>>>>>>>>>>>>> TunerHandle:%d >>>>>>>>>>>>>>>>>> \n",TunerHandle);
    if(RMF_SUCCESS !=  rmf_qamsrc_getLtsid(TunerHandle,&Ltsid))	
    {
        //VLFREE_INFO2(PmtObj);
      delete PmtObj;
      return -1;
    }

    pPmtTable = PmtObj->GetPmtTable();
//     pPmtTable->program_num;
    pCardMgrIf = cCardManagerIF::getInstance(/*pfc_kernel_global*/);
    if(pCardMgrIf != NULL)
    {
    ccMgrRc = pCardMgrIf->vlAllocateCaFreeResource(&cardResourceHandle, (unsigned int) Ltsid, pPmtTable->program_num);
    if (ccMgrRc == cCardManagerIF::CM_IF_CA_RES_BUSY_WITH_LTSID_PRG)
    {
                  //No need to send the CA details to card
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," CA resource is already allocated CM_IF_CA_RES_BUSY_WITH_LTSID_PRG \n");
        *pCardResourceHandle = cardResourceHandle;
    }
    else if (ccMgrRc == cCardManagerIF::CM_IF_CA_RES_ALLOC_SUCCESS)
    {
                  //Need to send the CA details to card
          bSendCADetailsToCard = true;
          *pCardResourceHandle = cardResourceHandle;
    }
    else if(ccMgrRc == cCardManagerIF::CM_IF_CA_RES_NO_CABLE_CARD)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD"," no CA resource not allocated bcoz no Cable Card inserted \n");
        bSendCADetailsToCard = true;
    }
    else
    {
        //VLFREE_INFO2(PmtObj);
        delete PmtObj;
        return -1;
      }
    }

    if(bSendCADetailsToCard)
    {

      rmf_osal_event_handle_t event_handle;
      rmf_osal_event_params_t event_params = {0};	
      struct vlcaDetailInfo *pCaDetails = new struct vlcaDetailInfo;


      pCaDetails->ulSourceLTSID = Ltsid;
      pCaDetails->cardResourceHandle = *pCardResourceHandle;
      pCaDetails->pPmtTable = pPmtTable;
      pCaDetails->stream_ptr = NULL;
      pCaDetails->pPmtObj = PmtObj;

      event_params.priority = 0; //Default priority;
      event_params.data = (void *)pCaDetails;
      event_params.data_extension = 0;
      event_params.free_payload_fn = freeCAPmtTbl;
      rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_MPEG, pfcEventMPEG::mpeg_PMT_WITH_CA, 
  		&event_params, &event_handle);
  
      rmf_osal_eventmanager_dispatch_event(em, event_handle);
    }
    else
    {
        //VLFREE_INFO2(PmtObj);
        delete PmtObj;
        
    }
   if(cardResourceHandle == -1)
   {
     return -1;
   }
    
    return 0;
}
#endif
int vlCaGetPmt(int  TunerId, unsigned short VideoPid, unsigned short AudioPid, unsigned char **pMt, unsigned long *pPmtSize)
{
 
  
  if( (VidPid1 == VideoPid) && ( AudPid1 == AudioPid) )
  {
    *pMt = Pmt1;
    *pPmtSize = sizeof(Pmt1);
    
  }
  else if( (VidPid2 == VideoPid) && ( AudPid2 == AudioPid) )
  {
    *pMt = Pmt2;
    *pPmtSize = sizeof(Pmt2);
  }
  else if( (VidPid3 == VideoPid) && ( AudPid3 == AudioPid) )
  {
    *pMt = Pmt3;
    *pPmtSize = sizeof(Pmt3);
  }
  else if( (VidPid4 == VideoPid) && ( AudPid4 == AudioPid) )
  {
    *pMt = Pmt4;
    *pPmtSize = sizeof(Pmt4);
  }
  else if( (VidPid5 == VideoPid) && ( AudPid5 == AudioPid) )
  {
    *pMt = Pmt5;
    *pPmtSize = sizeof(Pmt5);
  }
  else if( (VidPid6 == VideoPid) && ( AudPid6 == AudioPid) )
  {
    *pMt = Pmt6;
    *pPmtSize = sizeof(Pmt6);
  }
  else
  {
      return -1;
  }
  return 0;
}
extern "C"  int vlSectionGetPmtForTunerId(int index, int TunerId, unsigned short VideoPid, unsigned short AudioPid,unsigned char **pMt, unsigned long  *PmtLen);

extern "C" int vlCaAllocateResource(int  TunerId, unsigned short VideoPid, unsigned short AudioPid, int *pCaResourceHdl)
{
    unsigned char *pPmt=NULL;
    unsigned long PmtSize= 0;
    int iRet = -1;
    
    if(vl_env_get_bool(false, "FEATURE.USE_RI_CA_IMPL"))
    {
    	return iRet;
    }

 #if 0
    if((pCaResourceHdl == NULL ) || (VideoPid == 0 ) || (AudioPid == 0) || (TunerId < 0) )
    {
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlCaAllocateResource: Error !! input params pCaResourceHnadle:0x%X  VideoPid:0x%X AudioPid:0x%X TunerId:%d\n",pCaResourceHdl,VideoPid,AudioPid,TunerId);
    return iRet;
    }

    vlCardCaPmtMutexBegin();
   //iRet = vlCaGetPmt(TunerId,VideoPid,AudioPid,&pPmt,&PmtSize);
  
   //BUG FIX: PARKER-3056. Instead of using the pointer which might get deleted by another section filter thread while we are processing the code is modified to get the copy of the contents 
   //so that we can free after our usage.
   //NOTE: memory for  pPmt is allocated inside the api and is expected to be freed by the caller

    rmf_osal_memAllocP(RMF_OSAL_MEM_POD, 1024, (void **)&pPmt);
    PmtSize  = 1024;
//   iRet = vlSectionGetPmtForTunerId(1,TunerId,VideoPid,AudioPid,&pPmt,&PmtSize);
   if(iRet != 0)
   {
     RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlCaAllocateResource: vlSectionGetPmtForTunerId Failed \n");
     vlCardCaPmtMutexEnd();
     rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pPmt);     	
     return iRet;
   }
   if((pPmt == NULL ) || (PmtSize == 0) )
   {
     RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlCaAllocateResource: Error Pmt is NULL or PmtSize is zero \n");
     vlCardCaPmtMutexEnd();
     return -1;
   }
   iRet = vlCardSendCAPmt(TunerId, pPmt, PmtSize ,pCaResourceHdl );
   if(iRet != 0)
   {
     RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlCaAllocateResource: vlCardSendCAPmt Failed \n");
     
   }
   //Free the memory that was allocated in the vlSectionGetPmtForTunerId api.
   rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pPmt);
   vlCardCaPmtMutexEnd();

#endif 	

   return iRet;
}

extern "C" int vlCaDeAllocateResource(int CAResourceHandle )
{
    int ccMgrRc = -1;
    
    if(vl_env_get_bool(false, "FEATURE.USE_RI_CA_IMPL"))
    {
        return ccMgrRc;
    }
   
#if 0    
    
    if(-1 == CAResourceHandle)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlCaDeAllocateResource: Error !! Return error CAResourceHandle is not valid \n");
        return -1;
    }
    
    if(GetPodInsertedStatus() != 1)
    {
      RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlCaDeAllocateResource: Error !! Return error No cable card inserted \n");
      return-1;
    }    
    vlCardCaPmtMutexBegin();
    cCardManagerIF *pCardMgrIf = cCardManagerIF::getInstance(/*pfc_kernel_global*/);
    if(pCardMgrIf != NULL)
    {
        ccMgrRc = pCardMgrIf->vlDeAllocCaResource(CAResourceHandle);
        if(ccMgrRc != cCardManagerIF::CM_IF_CA_RES_DEALLOC_SUCCESS)
        {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlCaDeAllocateResource: vlDeAllocCaResource returned failed \n");
                vlCardCaPmtMutexEnd();
                return -1;
        }
        
    }
    vlCardCaPmtMutexEnd();
#endif
    return ccMgrRc;
    
    
    
}
