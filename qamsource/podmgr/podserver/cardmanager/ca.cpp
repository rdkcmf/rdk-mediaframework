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



#include <unistd.h>
#ifdef GCC4_XXX
#include <list>
#else
#include <list.h>
#endif

#include "cardUtils.h"
#include "cmhash.h"
#include "rmf_osal_event.h"
#include "core_events.h"

#include "cardmanager.h"
#include "cmevents.h"
#include "ca.h"

#include "poddriver.h"
#include "cm_api.h"
#include "capmtparse.h"
#include "rmf_osal_mem.h"
#include "cardManagerIf.h"
#include <coreUtilityApi.h>

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

#define __MTAG__ VL_CARD_MANAGER



#define CA_DESC_SIZE 256// pmt is not more than 188 bytes

extern unsigned char GetPodInsertedStatus();
extern "C" int vlCpCalReqGetStatus();
extern "C" int vlCardResGetMaxProgramsSupportedByCard();
extern "C" int vlGetCaAllocStateForPrgIndex(unsigned char PrgIndex, unsigned char Ltsid,unsigned short PrgNum, unsigned char *pAllocState);
extern "C" int vlSetCaEncryptedForPrgIndex(unsigned char PrgIndex, unsigned char Ltsid,unsigned short PrgNum);

cCA::cCA(CardManager *cm,char *name):CMThread(name)
{
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","cCA::cCA()\n");
    event_queue = 0;
    this->cm = cm;
    this->usVideoPid =  0;
    this->usAudioPid = 0;
    this->hIn = 0;
}


cCA::~cCA(){}



void cCA::initialize(void){

    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_eventqueue_handle_t eq;

    rmf_osal_eventqueue_create ( (const uint8_t* )"cCA" ,
		&eq);


//PodMgrEventListener    *listener;

        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","cCA::initialize()\n");
        rmf_osal_eventmanager_register_handler(
   		em, eq, RMF_OSAL_EVENT_CATEGORY_CA );

		        //Prasad (06/09/2008)- Commented the following statement as the Table_Manager event category is not used

//New code
        rmf_osal_eventmanager_register_handler(
   		em, eq, RMF_OSAL_EVENT_CATEGORY_MPEG );

        this->event_queue = eq;
        //listener = new PodMgrEventListener(cm, eq,"ResourceManagerThread);


        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","cCA::initialize() eq=%p\n",(void *)eq);
        //listener->start();
}

 static struct vl_pmt_tbl *vlg_pPmt_tbl_forCa = NULL;

#define DUMPFILE        "/mnt/nfs/log/pmt1.dump"
#if 0
void cCA::dump(void)
{
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Entering... cCA::dump:%s\n", __FUNCTION__);

    int i;
    FILE *dumpf=NULL;
    if(!vlg_pPmt_tbl_forCa)
        return;

    // Open a file for dumping debug info
    if(!dumpf && !(dumpf = fopen(DUMPFILE, "a"))) {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","TSDemux::initialize(): failed to open dump file... using stderr\n");
        dumpf = stderr;
    }
    fprintf(dumpf, "PMT:\n");
    fprintf(dumpf, "  section length:            %d\n", vlg_pPmt_tbl_forCa->section_len);
    fprintf(dumpf, "  version:                   %d\n", vlg_pPmt_tbl_forCa->version);
    fprintf(dumpf, "  section number:            %d\n", vlg_pPmt_tbl_forCa->section_num);
    fprintf(dumpf, "  last section number:       %d\n", vlg_pPmt_tbl_forCa->last_section_num);
    fprintf(dumpf, "  current/next indicator:    %d\n", vlg_pPmt_tbl_forCa->current_next);
    fprintf(dumpf, "  pcr pid:                   0x%x\n", vlg_pPmt_tbl_forCa->pcr_pid);
    fprintf(dumpf, "  prog info length:          %d\n", vlg_pPmt_tbl_forCa->prog_info_len);
    fprintf(dumpf, "  num streams:               %d\n", vlg_pPmt_tbl_forCa->num_streams);
    for(i=0; i<vlg_pPmt_tbl_forCa->num_streams; i++) {
        struct pmt_streams *prog = &vlg_pPmt_tbl_forCa->streams[i];
        fprintf(dumpf, "  program %d:\n", i);
        fprintf(dumpf, "    stream type:             0x%x\n", prog->stream_type);
        fprintf(dumpf, "    elem pid:                0x%x\n", prog->elem_pid);
        fprintf(dumpf, "    es info len:             %d\n", prog->es_info_len);
        if(prog->desc)
            prog->desc->dump(dumpf);
    }
    if(vlg_pPmt_tbl_forCa->desc) {
        fprintf(dumpf, "additional descriptor:\n");
        vlg_pPmt_tbl_forCa->desc->dump(dumpf);
    }
    fflush(dumpf);
}
#endif
void vlCAPmtFreeMem(void *pBuf)
{
    if(pBuf)
                rmf_osal_memFreeP(RMF_OSAL_MEM_POD,pBuf);
}
extern unsigned short vlg_CaSystemId;

static bool vlCADescValidityCheck(unsigned char *pCaDes, int desclen)
{
    unsigned char *pData;
    unsigned char Length;
    unsigned short CA_system_id,CA_pid;
    bool ret = false;
    
    if((pCaDes == NULL) || desclen < 2)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlCADescValidityCheck: Error Null pointer or desclen:%d < 2\n",desclen);
        return ret;
    }
    if( pCaDes[0] != 0x09 )
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlCADescValidityCheck: Error Incorrect CA desc tag:0x%02\n",pCaDes[0]);
        return ret;
    }
    Length = pCaDes[1];
    if(Length >= 4)
    {
        CA_system_id = ((pCaDes[2]) << 8) | (pCaDes[3]);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlCADescValidityCheck:CA_system_id:0x%04X ... 0x%02X 0x%02X\n",CA_system_id,pCaDes[2],pCaDes[3]);
        if(vlg_CaSystemId == CA_system_id)
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlCADescValidityCheck: valid CA desc  \n");
            ret = true;
        }
        else
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlCADescValidityCheck: invalid CA_system_id:0x%04X  vlg_CaSystemId:0x%04X\n",CA_system_id,vlg_CaSystemId);
        }
        CA_pid = ((pCaDes[4]) << 8) | (pCaDes[5]);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlCADescValidityCheck:CA_pid:0x%04X ... 0x%02X 0x%02X\n",CA_pid,pCaDes[4],pCaDes[5]);
    }
    return ret;
    
}

void vlSendCaPmt(struct vl_pmt_tbl *tbl,unsigned long ltsid, int caResourceHandle,void* stream_ptr)
{
    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_event_handle_t event_handle;
    rmf_osal_event_params_t event_params = {0};
	
    LCSM_EVENT_INFO *pEventInfo;
//    unsigned long len = 0;
    unsigned char i;        //    unsigned char *pChar,i;
    unsigned char *pCaDes,indx=0;
    int desclen = 0;
    int PrgCaDesclen=0;//,ElCaDesclen=0;
    unsigned char *ca_map = NULL,*cp_map = NULL;
    //unsigned char ca_pmt_cmd_id = ca_pmt_cmd_query;//ca_pmt_cmd_query;//ca_pmt_cmd_ok_mmi;//ca_pmt_cmd_ok_descrambling;
    unsigned char ca_pmt_cmd_id = ca_pmt_cmd_ok_descrambling;
    unsigned char  ProgramCAEncrypted = 0;
    unsigned short ProgramNumber;

    unsigned short srcId;
    unsigned short ECMPid;
    unsigned char State = 0xFF;
    vector<vlcondAccessesDescsInfo*> condAccessesDescsList;
    vlcondAccessesDescsInfo *pCondAccessesDescsInfo=NULL;
    bool IsCADescValid = false;

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," >>>>>>>>>> Stream Pointer:0x%X \n",stream_ptr);
    if(GetPodInsertedStatus() == 1)
    {
        if(caResourceHandle >= vlCardResGetMaxProgramsSupportedByCard())
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlSendCaPmt: Error !!!!!!! caResourceHandle:%d is invalid ... Returning from here...\n",caResourceHandle);
            return;
        }
        #if 1
        if(0 != vlGetCaAllocStateForPrgIndex((unsigned char)caResourceHandle, (unsigned char)ltsid,(unsigned short)vlg_pPmt_tbl_forCa->program_num , &State))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlSendCaPmt: Error !!!!!!! vlGetCaAllocStateForPrgIndex() returned Error..\n");
            //return;
        }
        else
        {
            if(State != 1)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlSendCaPmt: Error !!!!!!! vlGetCaAllocStateForPrgIndex() the state is Resource not allocated \n");
                //return;
            }
        }
        #endif
    }

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlSendCaPmt: caResourceHandle:%d is Valid ... going to send Ca pmt \n",caResourceHandle);
	rmf_osal_memAllocP(RMF_OSAL_MEM_POD, CA_DESC_SIZE,(void **)&cp_map);
    if(cp_map == NULL)
    {
       RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlSendCaPmt: memory allocation failure %d\n",__LINE__);
    	return;
    }
	rmf_osal_memAllocP(RMF_OSAL_MEM_POD, CA_DESC_SIZE,(void **)&ca_map);
//program_index
    if(ca_map == NULL)
    {
       RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlSendCaPmt: memory allocation failure %d\n",__LINE__);    
	rmf_osal_memFreeP(RMF_OSAL_MEM_POD,cp_map);
    	return;
    }
//program_index

    	ca_map[indx++] = caResourceHandle;//0;//vlg_Program_Indx;

#if 0
    if(vlg_Program_Indx == vlg_MaxProg_Support)
    {
        vlg_Program_Indx  = 0;
    }
    else
    {
        vlg_Program_Indx++;
    }
#endif
//transcation_id, will be assigned later
    ca_map[indx++] = 0;// vlg_Tran_Id;
#if 0
    if(vlg_Tran_Id == 255)
    {
        vlg_Tran_Id  = 0;
    }
    else
    {
        vlg_Tran_Id++;
    }
#endif
    ProgramNumber = vlg_pPmt_tbl_forCa->program_num & 0xFFFF;
//ltsid=0xaa is meant for only ATI for the first tuner... Get the correct ltsid from the  tuner
    ca_map[indx++] = ltsid;//0x01 for broadcom first Inband tuner;//0xaa for ATI first Inband Tuner;
//program_number ... 16 bits
   //high
    
    ca_map[indx++] = (vlg_pPmt_tbl_forCa->program_num&0xFF00)>>8;
   //low
    ca_map[indx++] = vlg_pPmt_tbl_forCa->program_num&0xFF;

  srcId = 0;
//source_id ... 16 bits
   //high//

    ca_map[indx++] = (srcId & 0xFF00) >> 8;//0;
   //low
    ca_map[indx++] = srcId & 0xFF;//0;
//ca_pmt_cmd_id
    ca_map[indx++] = ca_pmt_cmd_id;// OK_descrambling


     if(vlg_pPmt_tbl_forCa->desc)
     {
        //condAccessesDescsList.clear();
        condAccessesDescsList = vlg_pPmt_tbl_forCa->desc->getcondAccessesDescsList();
        if(condAccessesDescsList.size() > 0)
        {
          //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlSendCaPmt: Debug ## 4\n");
            for(vector<vlcondAccessesDescsInfo*>::iterator Res_iter = condAccessesDescsList.begin(); Res_iter != condAccessesDescsList.end(); Res_iter++)
            {
              //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlSendCaPmt: Debug ## 5\n");
                pCondAccessesDescsInfo = *Res_iter;
                if(pCondAccessesDescsInfo)
                {
                    
                    //desclen = vlg_pPmt_tbl_forCa->desc->get_ca_desc_len();
                    desclen = pCondAccessesDescsInfo->condAccessesRawBuffLen;
                    pCaDes = pCondAccessesDescsInfo->pCondAccessesRawBuff;
                    IsCADescValid = vlCADescValidityCheck(pCaDes,desclen);
                    if(IsCADescValid == true)
                        break;
                }//if

            }//for
        }//if
        
        //program_info_length.. 16bits( 4bits reserved + 12bit info_length...
             //high
        if(IsCADescValid == true)
        {
            ca_map[indx++] = (desclen & 0x0F00) >> 8;//
            //low
            ca_map[indx++] = desclen & 0x00FF;//
        }
        else
        {
            desclen = 0;
            ca_map[indx++] = (desclen & 0x0F00) >> 8;//
            //low
            ca_map[indx++] = desclen & 0x00FF;//
        }
        if(desclen > 0)
        {
            //pCaDes = vlg_pPmt_tbl_forCa->desc->get_ca_desc();
            rmf_osal_memcpy(&ca_map[indx],pCaDes,desclen, CA_DESC_SIZE-indx, desclen);

    //checking for ca descriptor tag and the length >= 4 ( tag(1)+len(1)+CA_Systemid(2) + CA pid(2) )
    //to get the CA pid
            if(desclen >= 6)
            {
                if( (ca_map[indx + 0] == 9) && (ca_map[indx+1] >= 4) )
                {
                    ECMPid = ( ((unsigned short)ca_map[indx+4] << 8) | ca_map[indx+5]);
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlSendCaPmt:ECMPid:0x%02X \n",ECMPid );
                    if(ECMPid != 0)
                    ProgramCAEncrypted = 1;
                }
                else
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlSendCaPmt: Error in Ca Desc tag %d or len:%d \n",ca_map[0],ca_map[1]);
                }
            }
            else
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlSendCaPmt: Error in Ca Desc Length %d \n",desclen);
            }

            //ProgramCAEncrypted = 1;
            indx += desclen;
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ########## Main CA descriptor found \n");
            
                    
            

        }
    }
    else
    {
        //program_info_length.. 16bits( 4bits reserved + 12bit info_length...
             //high
        ca_map[indx++] = 0;//
        //low
        ca_map[indx++] = 0;//
    }
    PrgCaDesclen = desclen;

    for(i=0; i < vlg_pPmt_tbl_forCa->num_streams; i++)
    {
        struct vlpmt_streams *prog = &vlg_pPmt_tbl_forCa->streams[i];

        //Stream Type
        ca_map[indx++] = prog->stream_type;
        //Elementary PID ... 16 bits ( 3 reserved + 13 pid)
            //high
        ca_map[indx++] = (prog->elem_pid & 0x1F00)>>8;
            //low
        ca_map[indx++] = prog->elem_pid&0xFF;
        IsCADescValid = false;
        if(prog->desc)
        {
            //desclen = prog->desc->get_ca_desc_len();
            condAccessesDescsList.clear();
            condAccessesDescsList = prog->desc->getcondAccessesDescsList();
            if(condAccessesDescsList.size() > 0)
            {
                for(vector<vlcondAccessesDescsInfo*>::iterator Res_iter = condAccessesDescsList.begin(); Res_iter != condAccessesDescsList.end(); Res_iter++)
                {
                    pCondAccessesDescsInfo = *Res_iter;
                    if(pCondAccessesDescsInfo)
                    {
                        
                        //desclen = vlg_pPmt_tbl_forCa->desc->get_ca_desc_len();
                        desclen = pCondAccessesDescsInfo->condAccessesRawBuffLen;
                        pCaDes = pCondAccessesDescsInfo->pCondAccessesRawBuff;
                        IsCADescValid = vlCADescValidityCheck(pCaDes,desclen);
                        if(IsCADescValid == true)
                            break;
                    }//if
    
                }//for
            }//if
        }
        else
            desclen = 0;

        if(IsCADescValid == false)
        {
        
            desclen = 0;
        }

        //ElCaDesclen = desclen;
        if( desclen)
        {
            //Adding one for pmt_cmd_id
            desclen += 1;
            // ES_info_length .. 16btis ( 4 res + 12 info_length )
                //high
            ca_map[indx++] = (desclen & 0xF00)>>8;
                //low
            ca_map[indx++] = desclen & 0xFF ;
            //ca_pmt_cmd_id at ES level
            ca_map[indx++] = ca_pmt_cmd_id;//ca_pmt_cmd_id;//OK_descrambling

            desclen -= 1;
//Right now assuming that there will be only one CA descriptor per elem stream.
//but in the spec for ca_pmt() for M-card and S-card ..it has a loop for CA_descriptors...
//for more than one CA_descriptor per elem stream...
//we need to do the right thing with respect to CA_descriptors
            //if(prog->es_info_len == desclen)
            {
                //Assuming only one CA descriptor...
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","ONE Elem CA descriptor ... pid:0x%X strm type:0x%X\n",prog->elem_pid,prog->stream_type);
                //pCaDes = prog->desc->get_ca_desc();
                rmf_osal_memcpy(&ca_map[indx],pCaDes,desclen, CA_DESC_SIZE-indx, desclen);

                if(desclen >= 6)
                {
                    ECMPid = 0;
                    if( (ca_map[indx + 0] == 9) && (ca_map[indx + 1] >= 4) )
                    {
                        ECMPid = ( ((unsigned short)ca_map[indx+ 4] << 8) | ca_map[indx + 5]);
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlSendCaPmt:Elm ECMPid:0x%02X \n",ECMPid );
                        if(ECMPid != 0)
                            ProgramCAEncrypted = 1;
                    }
                    else
                    {
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlSendCaPmt: Error in Elm Ca Desc tag %d or len:%d \n",ca_map[0],ca_map[1]);
                    }
                }
                else
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlSendCaPmt: Error in Elm Ca Desc Length %d \n",desclen);
                }

                indx += desclen;
            //    ProgramCAEncrypted  = 1;

            }
#if 0
            else if(desclen)
            {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," \n More than ONE ES CA descriptor ... \n");
                pCaDes = prog->desc->get_ca_desc();
                memcpy(&ca_map[indx],pCaDes,desclen);
                indx += desclen;

            }

            else
            {
                // NO CA descriptors at ES level..
            }
#endif

        }
        else
        {
            // ES_info_length .. 16btis ( 4 res + 12 info_length )
                //high
            ca_map[indx++] = 0;
                //low
            ca_map[indx++] = 0;

        }

    }//for


    /*RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\n######## ca_pmt ############ \n");
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ca_pmt:%d bytes dump in raw format\n",indx);
    for(int ii =0; ii < indx; ii++)
    {
        if(ii == 0)
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","0x%02X  :",ii);
        if((ii % 16) == 0)
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\n0x%02X  :",ii);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%02X ",ca_map[ii]);


    }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\n");
    */

    //
    {
        CardManagerCAEncryptionData_t *pCAEncryptionData;
        rmf_osal_event_handle_t bEvent_handle;
        rmf_osal_event_params_t bEvent_params = {0};

		rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(CardManagerCAEncryptionData_t),(void **)&pCAEncryptionData);

        if(pCAEncryptionData == NULL)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD"," ERROR !! pCAEncryptionData in NULL returning \n");
		rmf_osal_memFreeP(RMF_OSAL_MEM_POD,cp_map);
		rmf_osal_memFreeP(RMF_OSAL_MEM_POD,ca_map);
            return;
        }
        pCAEncryptionData->CaEncrypFlag =  ProgramCAEncrypted;
        pCAEncryptionData->prgNum = ProgramNumber;
        pCAEncryptionData->LTSID = ltsid&0xFF;
        //Report about the program CA-Encryption
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",">>>>>>> Posting  CAEncryption Event Flag >>>>>>>>>>>>>>>> : Flag:0x%02X prgNum:0x%04X ltsid:%02X\n",pCAEncryptionData->CaEncrypFlag,pCAEncryptionData->prgNum,pCAEncryptionData->LTSID);

        bEvent_params.priority = 0; //Default priority;
        bEvent_params.data = (void *)pCAEncryptionData;
        bEvent_params.data_extension = sizeof(CardManagerCAEncryptionData_t);
        bEvent_params.free_payload_fn = vlCAPmtFreeMem;
        rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_CARD_MANAGER, CardMgr_CA_Encrypt_Flag, 
    		&bEvent_params, &bEvent_handle);
    
        rmf_osal_eventmanager_dispatch_event(em, bEvent_handle);

        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," vlCpCalReqGetStatus(): %d ProgramCAEncrypted:%d\n",vlCpCalReqGetStatus(),ProgramCAEncrypted);
        if((GetPodInsertedStatus() == 1) && (vlCpCalReqGetStatus() == 0 )&& (ProgramCAEncrypted ==1 ))
        {
            char pData[128];

            //Display message saying program is scrambled can not be viewed..
        //    sprintf(pData,"CP System Failed, Can not descramble the program ");
                snprintf(pData,sizeof(pData),"CP System Failed, Can not descramble the program ");
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," Sending display event to to display: %s \n",pData);
            cPOD_driver::CardManagerSendMsgEvent(pData,strlen(pData));
        }
        if(ProgramCAEncrypted && (GetPodInsertedStatus() == 1) )
        {
            if(0 != vlSetCaEncryptedForPrgIndex((unsigned char)caResourceHandle, (unsigned char) ltsid,(unsigned short) ProgramNumber))
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlSetCaEncryptedForPrgIndex function returned Error \n");
            }
        }
    }

    if( 0 == GetPodInsertedStatus() )
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," <<<<<<<<<<<<<,  CARD NOT INSERTED >>>>>>>>>>>>>>> \n");
                 rmf_osal_memFreeP(RMF_OSAL_MEM_POD,cp_map);
                rmf_osal_memFreeP(RMF_OSAL_MEM_POD,ca_map);
        return;
       
    }
    if(ProgramCAEncrypted == 0)
    {
      RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ProgramCAEncrypted:%d Program is not scrambled , No CA PMT is sent in this case. \n",ProgramCAEncrypted);
               rmf_osal_memFreeP(RMF_OSAL_MEM_POD,cp_map);
                rmf_osal_memFreeP(RMF_OSAL_MEM_POD,ca_map);
        return;
      
    }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Posting the CA descriptor to the CARD \n");

    rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(LCSM_EVENT_INFO),(void **)&pEventInfo);    
	
    pEventInfo->event = POD_CA_PMT_READY;
    pEventInfo->x     = 0;
    pEventInfo->y     = (long)indx;
    pEventInfo->z     = 0;
    pEventInfo->dataPtr = &ca_map[0];
    pEventInfo->dataLength = (long)indx;

    event_params.priority = 0; //Default priority;
    event_params.data = (void *)pEventInfo;
    event_params.data_extension = sizeof(LCSM_EVENT_INFO);
    event_params.free_payload_fn = podmgr_freefn;
    rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_CA, POD_CA_PMT_READY, 
		&event_params, &event_handle);

    rmf_osal_eventmanager_dispatch_event(em, event_handle);

    //if(PrgCaDesclen || ElCaDesclen)
    {

        memcpy(cp_map,ca_map,CA_DESC_SIZE);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Posting the CA descriptor to the cprot.... \n");

        rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(LCSM_EVENT_INFO),(void **)&pEventInfo);    

        pEventInfo->event = POD_CP_PMT_READY;
        pEventInfo->x     = (long int)stream_ptr;
        pEventInfo->y     = (long)indx;
        pEventInfo->z     = 0;
        pEventInfo->dataPtr = (unsigned char *)&cp_map[0];
        pEventInfo->dataLength = (long)indx;
    	    
        event_params.priority = 0; //Default priority;
        event_params.data = (void *)pEventInfo;
        event_params.data_extension = sizeof(LCSM_EVENT_INFO);
        event_params.free_payload_fn = podmgr_freefn;
        rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_CP, POD_CA_PMT_READY, 
    		&event_params, &event_handle);

        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," >>>>>>>>>> Stream Pointer:0x%X \n",stream_ptr);
        rmf_osal_eventmanager_dispatch_event(em, event_handle);

    }

}
void vlCAPostNoPrgSelectEvent(int PrgIndx)
{
    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_event_handle_t event_handle;
    rmf_osal_event_params_t event_params = {0};


    LCSM_EVENT_INFO *pEvent;
    rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(LCSM_EVENT_INFO),(void **)&pEvent); 

    pEvent->event = POD_CA_NO_SELECT;
    pEvent->x     = (long int)PrgIndx;
    pEvent->y     = 0;
    pEvent->z     = 0;
    pEvent->dataPtr = NULL;
    pEvent->dataLength = 0;
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," Posting the POD_CA_NO_SELECT event \n");

    event_params.priority = 0; //Default priority;
    event_params.data = (void *)pEvent;
    event_params.data_extension = sizeof(LCSM_EVENT_INFO);
    event_params.free_payload_fn = podmgr_freefn;
    rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_CA, POD_CA_NO_SELECT, 
		&event_params, &event_handle);

    rmf_osal_eventmanager_dispatch_event(em, event_handle);
}
void cCA::run(void )
{
    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_event_handle_t event_handle_rcv;
    rmf_osal_event_params_t event_params_rcv = {0}; 
    rmf_osal_event_category_t event_category;
    uint32_t event_type;
	
    //int car_OpenRes = 0;
//    TableManager *ptm ;
#ifdef USE_TABLE_MANAGER

    MasterTableManager *pmtm;

#endif
//    int    pcr_pid;

    LCSM_EVENT_INFO *pEventInfo=NULL;
    //pfcEventBase    *event=NULL;

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","CAEventListener::Run()\n");
    ca_init();
    while (1)
    {
        int result = 0;

        rmf_osal_eventqueue_get_next_event( event_queue,
		&event_handle_rcv, &event_category, &event_type, &event_params_rcv);
        		

        switch (event_category)
        {
        case RMF_OSAL_EVENT_CATEGORY_CA:
            pEventInfo = (LCSM_EVENT_INFO * ) event_params_rcv.data;
            
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","Got a CA EVENT of type  %x\n",event_category);
            switch (event_type)
            {
            case MR_CHANNEL_CHANGE_START:
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","MR_CHANNEL_CHANGE_START %x %x\n",usVideoPid, usAudioPid);
                if(this->cm->cablecardFlag == CARD_READY)
                {
#ifdef USE_TABLE_MANAGER
                    pmtm =  (MasterTableManager *)pfc_kernel_global->get_resource("TableManager");
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","CA: pmtm=%p\n", (void *)pmtm);
                    if(pmtm != NULL)
                    {
#endif
                        //ptm = pmtm->get_tm(this->hIn);
                        //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","ptm = %x\n", ptm);
                        //if(ptm)

                            //this->cm->ulPhysicalTunerID[0] = ptm->tune_completed_info.ulPhysicalTunerID;
                            if((this->cm->pPOD_driver) /*&& (this->cm->ulPhysicalTunerID[0])*/)
                            {
                                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Connecting POD to tuner handle %lx\n",this->cm->ulPhysicalTunerID[0]);
                                this->cm->pPOD_driver->ConnectSourceToPOD(0/*(unsigned long)this->cm->ulPhysicalTunerID[0]*/, (uint32_t)usVideoPid);
                            }

#ifdef USE_TABLE_MANAGER
                            pmtm->startCollectPMT(  this->hIn  , this->usVideoPid, this->event_queue , 1);

                    }
#endif
                }

                break;
            default:
              if(pEventInfo)
              {
	              if( (pEventInfo->event  == POD_APDU) && this->cm->IsCaAPDURoute() )
	              {
	                   EventCableCard_RawAPDU *pEventApdu;
	                   rmf_osal_event_handle_t event_handle;
	                   rmf_osal_event_params_t event_params = {0};

			     rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(EventCableCard_RawAPDU),(void **)&pEventApdu);
	                   int ii;
	                        //do a dispatch event here
	              
	                  pEventApdu->resposeData = new uint8_t[(uint16_t)pEventInfo->z];

	/*                        memcpy(pEventApdu->resposeData , (uint8_t *)event->pEventInfo.y, (uint16_t)event->pEventInfo.z);*/
	                  memcpy(pEventApdu->resposeData , (uint8_t *)pEventInfo->y, (uint16_t)pEventInfo->z);
	                  pEventApdu->dataLength  =  (uint16_t)pEventInfo->z;
	                  pEventApdu->sesnId = (short)pEventInfo->x;

	                  RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "Posting the Ca Response TAG:0x%02X%02X%02X <<<<<\n",pEventApdu->resposeData[0],pEventApdu->resposeData[1],pEventApdu->resposeData[2]);
	                  vlMpeosDumpBuffer(RDK_LOG_TRACE1, "LOG.RDK.POD", pEventApdu->resposeData, pEventApdu->dataLength);
	                  
	                  #if 0
	                  for(ii = 0; ii < pEventApdu->dataLength; ii++)
	                  {
	                printf("0x%02X ",pEventApdu->resposeData[ii]);
	                if( ((ii+1) % 16) == 0)
	                {
	                  printf("\n");
	                }
	                  }
	                  printf("\n");
	                  #endif

	                  event_params.priority = 0; //Default priority;
	                  event_params.data = (void *)pEventApdu;
	                  event_params.data_extension = sizeof(EventCableCard_RawAPDU);
	                  event_params.free_payload_fn = EventCableCard_RawAPDU_free_fn;
	                  rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_CM_RAW_APDU, card_Raw_APDU_response_avl, 
	              		&event_params, &event_handle);
	                  rmf_osal_eventmanager_dispatch_event(em, event_handle);
	              }
	              else
	              {
	                  caProc((void *)pEventInfo);
	                  #if 0
	                  if(ca_resopen() && (car_OpenRes == 0) )
	                  {
	                  car_OpenRes = 1;
	                  vlMpeosPostCASystemEvent(1);
	                  }
	                  #endif
	              }
            	}			  
            break;

            }



            break;
        case RMF_OSAL_EVENT_CATEGORY_MPEG:
            switch (event_type)
            {
            case  CM_Mpeg_PMT_WITH_CA:
            {
                vlcaDetailInfo* pEventData = NULL;
                unsigned long LTSID = 0;
                //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
                //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD",">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD",">>>>>>>>>>>>>>>>>>CM_Mpeg_PMT_updated\n");
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD",">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
                //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
                pEventData = (vlcaDetailInfo*)event_params_rcv.data;
                if( pEventData )	
                {
                     vlg_pPmt_tbl_forCa = (struct vl_pmt_tbl*)(pEventData->pPmtTable);
                     LTSID = pEventData->ulSourceLTSID;
                     RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>LTSID=%lu\n",__FUNCTION__, LTSID);
     #ifdef INTEL_CANMORE
                     //LTSID = 0;// This is for Intel,temporary, need to remove later
     #endif
                     vlSendCaPmt(vlg_pPmt_tbl_forCa,LTSID,pEventData->cardResourceHandle,pEventData->stream_ptr);
                     //dump();
                }
            }
            }
            break;
            case RMF_OSAL_EVENT_CATEGORY_TABLE_MANAGER:

                switch (event_type)
                {
                case    TableManager_PMT_available:
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ############################################\n");
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ############################################\n");
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ############################################\n");
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," got table_PMT_available event\n");
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ############################################\n");
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ############################################\n");
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ############################################\n");
                
                break;
                case TableManager_STT_available:
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ############################################\n");
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ############################################\n");
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ############################################\n");
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," GOT SST update Event\n");
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ############################################\n");
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ############################################\n");
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ############################################\n");
                break;
                default:
                break;
                }

                break;
            default:
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET"," CAEventListener::default\n");
            break;
        }

        rmf_osal_event_delete(event_handle_rcv);

    }


}




void cCA::NotifyChannelChange( IOResourceHandle    hIn,unsigned short usVideoPid,unsigned short usAudioPid)
{

    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_event_handle_t event_handle;

    this->usVideoPid = usVideoPid;
    this->usAudioPid = usAudioPid;
    this->hIn        = hIn;
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","cCA::NotifyChannelChange \n");
    rmf_osal_event_create(RMF_OSAL_EVENT_CATEGORY_CA, MR_CHANNEL_CHANGE_START, 
		NULL, &event_handle);

    rmf_osal_eventmanager_dispatch_event(em, event_handle);
}





