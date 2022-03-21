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


 
//
#include <string.h>
#include "poddef.h"
#include <lcsm_log.h>

#include "tsdecode.h"  //Hannah - copied

#include "utils.h"
#include "transport.h"
#include "session.h"
#include "podhighapi.h"
#include "appmgr.h"
#include "applitest.h"
#include "cardMibAcc_app.h"

#include "global_event_map.h"
#include <string.h>
#include "rmf_osal_event.h"
#include "core_events.h"

#include "cardUtils.h"
#include <rdk_debug.h>
#include "rmf_osal_mem.h"

#if USE_SYSRES_MLT
#include "mlt_malloc.h"
#endif

#define __MTAG__ VL_CARD_MANAGER

#ifdef __cplusplus
extern "C" {
#endif


extern LCSM_DEVICE_HANDLE cardMibAccLCSMHandle;
extern LCSM_TIMER_HANDLE  cardMibAccTimer;



UCHAR  gcardMibAccCurrentVersionNumber;
UCHAR  gcardMibAccCurrentNextIndicator;

static UCHAR * //advanced pointer position
LengthIs(unsigned char *  ptr, USHORT * pLenField) //point to APDU Packet
{
    ptr += 3;
    // Figure out length field
#if (1)
    ptr += bGetTLVLength ( ptr, pLenField );
#else /* Old way to compute len: Save for now (just in case!) Delete later */
    if ((*ptr & 0x82) == 0x82) { // two bytes
        ptr++;
                //need to byte swap
                *pLenField = (  (((*(USHORT *)(ptr)) & 0x00ff) << 8) 
                              | (((*(USHORT *)(ptr)) & 0xff00) >> 8)
                             );
        ptr += 2;
    }
    else if ((*ptr & 0x81) == 0x81) { // one byte
        ptr++;
        *pLenField = (unsigned char)(*ptr);
        ptr++;
    }
    else { // lower 7 bits
        *pLenField = (unsigned char)(*ptr & 0x7F);
        ptr++;
    }
#endif    
    return (ptr);
}


/**************************************************
 *    APDU Functions to handle CA SUPPORT
 *************************************************/
int cardMibAccSndApdu(unsigned short sesnum, unsigned long tag, unsigned short dataLen, unsigned char *data)
{
    unsigned short alen;
    unsigned char papdu[MAX_APDU_HEADER_BYTES + dataLen];

        memset(papdu, 0,sizeof(papdu));
    if (buildApdu(papdu, &alen, tag, dataLen, data ) == NULL)
       {
       MDEBUG (DPM_ERROR, "ERROR:ca: Unable to build apdu.\n");
       return APDU_CARDMIBACC_FAILURE;
       }
    AM_APDU_FROM_HOST(sesnum, papdu, alen);
    return APDU_CARDMIBACC_SUCCESS;
}

int cardMibAccSnmpReq(unsigned char *pSnmpMess, unsigned long size)
{
    unsigned short alen;
    
    unsigned long tag = 0x9FA000;
    unsigned char papdu[MAX_APDU_HEADER_BYTES + size];
    memset(papdu, 0,sizeof(papdu));
    if (buildApdu(papdu, &alen, tag, size, pSnmpMess ) == NULL)
       {
       MDEBUG (DPM_ERROR, "ERROR:ca: Unable to build apdu.\n");
       return APDU_CARDMIBACC_FAILURE;
       }
    AM_APDU_FROM_HOST(lookupcardMibAccSessionValue(), papdu, alen);
    return APDU_CARDMIBACC_SUCCESS;
}
int cardMibAccRootOidReq(unsigned short sesnum)
{
    unsigned short alen;
    
    unsigned long tag = 0x9FA002;
    unsigned char papdu[MAX_APDU_HEADER_BYTES ];
    papdu[0] = 0x9F;
    papdu[1] = 0xA0;
    papdu[2] = 0x02;
    papdu[3] = 0x00;
    AM_APDU_FROM_HOST(sesnum, papdu, 4);
    return APDU_CARDMIBACC_SUCCESS;
}
void vlCardMibAccDataFreeMem(void *pData)
{
    if(pData)
		rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pData);
}
int cardMibAccSnmpReply(unsigned char *apdu, unsigned short apduLen)
{

   rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
   rmf_osal_event_handle_t event_handle;
   rmf_osal_event_params_t event_params = {0};
   unsigned char *pData,*pMess;
   unsigned short Len;
   
   if(apdu == NULL || apduLen == 0)
   {
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","cardMibAccSnmpReply: apdu == NULL || apduLen == 0 \n");
    return -1;
   }

   pMess = LengthIs(apdu,&Len);
    
  if(apduLen < (Len + 3))
  {
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","cardMibAccSnmpReply: apduLen:%d  < (Len:%d + 3) \n",apduLen,Len);
    return -1;
  }


   rmf_osal_memAllocP(RMF_OSAL_MEM_POD, Len,(void **)&pData);
   memcpy(pData,pMess,Len);

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\n##################################################################\n");
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",">>>>>>>> Posting the Snmp Msg Reply Event with Data Len:%d >>> \n",Len);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","##################################################################\n");

    event_params.priority = 0; //Default priority;
    event_params.data = (void *)pData;
    event_params.data_extension = Len;
    event_params.free_payload_fn = vlCardMibAccDataFreeMem;
    rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_CARD_MANAGER, CardMgr_Card_Mib_Access_Snmp_Reply, 
		&event_params, &event_handle);

    rmf_osal_eventmanager_dispatch_event(em, event_handle);

    return APDU_CARDMIBACC_SUCCESS;

}

#ifdef __cplusplus
}
#endif

