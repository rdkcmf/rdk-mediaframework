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





#ifndef __CARDMGR_H__
#define __CARDMGR_H__

#include "cmThreadBase.h"
//
//#include "pfcresource.h"
#include "cmevents.h"
#include "podmain.h"
#include "RsMgr.h"
#include "appinfo.h"
#include "ca.h"
#include "cardres.h"
#include "cardMibAcc.h"
#include "dsg.h"
#include "ipdirect_unicast.h"
#include "homing.h"
#include "host.h"
#include "mmi.h"
#include "sas.h"
#include "systime.h"
#include "xchan.h"
#include "syscontrol.h"
#include "cp_prot.h"
#include "genfeature.h"
#include "gen_diag.h"
#include "mmi_handler.h"
#include "hostAddPrt.h"
#include "headendComm.h"
#include "cablecarddriver.h"

#include <rdk_debug.h>
//#include "gateway.h"
#ifdef GCC4_XXX
#include <list>
#else
#include <list.h>
#endif
//#include "hal_api.h"


typedef int HostId_t;
typedef int HardwareId_t;
typedef int UIMode;
typedef int CaSystemId_t;
typedef int LCSM_DEVICE_HANDLE;

typedef void (* CallBackFn)( unsigned queue,LCSM_EVENT_INFO *eventInfo );  // for send_event ( called by POD stack)

#define TUNER_MASK(enumtype)   (1 << enumtype)

#ifdef __cplusplus

extern "C" {

#endif

//extern void POD_STACK_NOTIFY_CALLBACK(void * pFn);
extern void rmProc( void * par );
extern void rsmgr_init(void);
//extern int POD_STACK_INIT(LCSM_DEVICE_HANDLE *h);

#if 0
void  pdtTunerNotifyFn( DEVICE_HANDLE_t         hDevicehandle,
                       DEVICE_LOCK_t        eStatus,
                       UINT32                ulFrequency,
                       TUNER_MODE_t            enTunerMode,
                       UINT32                ulSymbolRate,
                       void                 *pulData);
#endif
#ifdef __cplusplus
}
#endif

#define MAX_TUNERS    2

typedef struct MMIDispCapabities_s
{
    unsigned short num_display_rows;
    unsigned short num_display_colums;
    unsigned char  num_ver_scrolling;
    unsigned char  num_hor_scrolling;
    unsigned char  display_type_support;
    unsigned char  data_entry_support;
    unsigned char  html_support;
    unsigned char  link_support;
    unsigned char  form_support;
    unsigned char  table_support;
    unsigned char  list_support;
    unsigned char  image_support;
}MMIDispCapabities_t;

class CardManager;
class cPODDriverWrapper;
class cOOB;
class ctuner_driver;
class cPOD_driver;

class CardManager{//: public pfcResource {

friend class cCardManagerIF;
friend class cAppInfo;
friend class cExtChannelThread;
friend class cExtChannelFlow;
friend class cMMI;
friend class cSAS;
friend class cHostControl;
friend class TSInterface;
friend class cMMI_Handler;
friend class cPOD_driver;
friend class ctuner_driver;
friend class cPodPresenceEventListener;
friend class cOOB;

public:

    CardManager();
    ~CardManager();
    int is_default();
    int initialize(void );
    int init_complete();

//    static void SendEventCallBack( unsigned queue,LCSM_EVENT_INFO *eventInfo);  // use pfc dispatch event mechanism
//    void RegisterCallBack(void);

//    CallBackFn      pFn;
    void StartPODThreads(CardManager *cm);
    void StopPODThreads();
    bool IsOCAP()
    {
        return ocapMode;
    }
    bool IsCaAPDURoute()
    {
        return CaAPDURoute;
    }

    unsigned char  GetCableCardState()
    {
        return this->cablecardFlag;
    }

    int         sockFd;

    int GetSocketHandle()
    {
        return sockFd;

    }
    cExtChannelFlow *pIP_flow;
    cOOB   *pOOB_packet_listener_thread;


     ctuner_driver    *ptuner_driver;
    cPOD_driver    *pPOD_driver;
    unsigned long   ulPhysicalTunerID[MAX_TUNERS];
    unsigned char        cablecardFlag;
    int CmcablecardFlag;
private:
    cRsMgr        *pRsMgr;
    cAppInfo    *pAppInfo;
     cMMI        *pMMI;
    cHoming        *pHoming;
    cExtChannelThread    *pXChan;
    cSysTime    *pSysTime;
    cHostControl    *pHostControl;
    cCA        *pCA;
    cCP_Protection    *pCP_Protection;
    cCardRes     *pCardRes;
    cCardMibAcc     *pCardMibAcc;
    cHostAddPrt     *pHostAddPrt;
    cHeadEndComm    *pHeadEndComm;

    cDsgThread     *pDsg;
#ifdef USE_IPDIRECT_UNICAST    
    cIpDirectUnicastThread     *pIpDirectUnicast;
#endif // USE_IPDIRECT_UNICAST    
    cSAS        *pSAS;
    cDiagnostic     *pDiagnostic;
    cGenFeatureControl *pGenFeatureControl;
    cSysControl     *pSysControl;
     cMMI_Handler    *pMMI_handler;
    cPodPresenceEventListener    *pPodMainThread;
    //cGateway    *pGateway;
    cCardManagerIF  *pCMIF;
    bool        ocapMode;
    bool        CaAPDURoute;

};


/** Abstract a given buffer as a stream of bits.
 *
 */
class cSimpleBitstream
{
public:

/**
 * Initialize the bit-getter to point to the specified buffer.
 */
  cSimpleBitstream(unsigned char *buf, //!< Buffer to use.
                  unsigned long len   //!< Length of \e buf.
                  );

/**
 * Not implemented.
 */
  ~cSimpleBitstream();

/** Extract the next 'n' bits and return them as an unsigned long.
 *
 * Reimplement this function with a faster algorithm.
 */
      unsigned long get_bits(int num);

/**
 * Set #curr_bit_index to #curr_bit_index + \e num.
 */
  void skip_bits(int num //!< Number of bits to skip.
                 );

/**
 * \return #curr_bit_index.
 */
  unsigned long get_pos() { return curr_bit_index; }

private:

/**
 * Bit stream bytes. Set by constructor.
 */
  unsigned char *bs;

/**
 * Bit stream length. Set by constructor.
 */
  unsigned long bs_len;

/**
 * Current bit index.
 */
  unsigned long curr_bit_index;
};


void set_cablecard_device(cableCardDriver* pDevice);
cableCardDriver* get_cablecard_device();



#endif




