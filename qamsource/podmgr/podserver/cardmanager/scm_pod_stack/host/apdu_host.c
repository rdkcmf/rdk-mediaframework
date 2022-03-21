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



#include <string.h>
#include "poddef.h"
#include <lcsm_log.h>

#include "global_event_map.h"

#include "utils.h"
//#include "module.h"
//#include "link.h"
#include "transport.h"
#include "session.h"
//#include "ci.h"
//#include "podlowapi.h"
#include "podhighapi.h"
#include "appmgr.h"
#include "applitest.h"
//#include "debug.h"
#include "hc_app.h"
#include "ob_qpsk.h"
#include "pod_support.h"
#include "mr.h"
#include "dsgResApi.h"
#include "sys_init.h"
#include <rdk_debug.h>
#include <string.h>
#include "rmf_osal_event.h"
#include "rmf_osal_sync.h"
#ifdef __cplusplus
extern "C" {
#endif

#ifdef USE_IPDIRECT_UNICAST
extern int HostCtrlInbandTunerAcquire( int frequency, UCHAR modulation );
extern int HostCtrlInbandTunerRelease( void );
#endif

extern int   vlIsDsgOrIpduMode();
LCSM_DEVICE_HANDLE hLCSMdev_pod; // in main.c
extern USHORT /*sesnum */lookupHCSessionValue(void);

uint32 gPODType = 0xFFFFFFFF;  //reserved
uint32 ndstytpe = FALSE;
uint8  gExpectingOOBevent = 0;
uint32  gExpectingIBevent = 0;
uint32  gExpectingIBeventNext = 0;

int vlg_bOutstandingOobTxTuneRequest = 0;
int vlg_bOutstandingOobRxTuneRequest = 0;

static LCSM_TIMER_HANDLE timerHandle;

void setFirmupgradeFlag( Bool on );
static Bool firmupgradeFlag = FALSE;
int vlg_InbanTuneSuccess=0;
int vlg_InbanTuneStart = 0;
UCHAR * //advanced pointer position
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
 *    APDU Functions to handle HOST CONTROL
 *************************************************/
int hcSndApdu(unsigned short sesnum, unsigned long tag, unsigned short dataLen, unsigned char *data)
{
    unsigned short alen;
    unsigned char papdu[MAX_APDU_HEADER_BYTES + dataLen];

    if (buildApdu(papdu, &alen, tag, dataLen, data ) == NULL)
    {
      MDEBUG (DPM_ERROR, "ERROR:hc: Unable to build apdu.\n");
      return APDU_HC_FAILURE;
    }
    AM_APDU_FROM_HOST(sesnum, papdu, alen);

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","hcSndApdu : APDU len = %d\n",alen);
    for(int i =0;i<alen;i++)
      RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","0x%02x ", papdu[i]);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","\n");

    return APDU_HC_SUCCESS;
}


#if 0
/*
 *    APDU in: 0x9F, 0x84, 0x04, "OOB_TX_tune_req"
 */
int  APDU_OOBTxTuneReq (USHORT sesnum, UCHAR * apkt, USHORT len)
{
//simple case beacause we're going to reject, we're not going to parse

if (sesnum == 0x00)
   sesnum = lookupHCSessionValue();
if (sesnum == 0)
   return APDU_HC_FAILURE;

return (RespOOBTxTuneCnf(sesnum) ) ;
}

/*
 *    APDU in: 0x9F, 0x84, 0x05, "OOB_TX_tune_cnf"
 */
#endif
int  APDU_OOBTxTuneCnf (USHORT sesnum, UCHAR * apkt, USHORT len)
{
//not implementing, this is an reponse called by us not by an ADPU from POD to Host
//we will send this ADPU back to POD via short method. (below)

   return APDU_HC_FAILURE;
}






/*
 *    APDU in: 0x9F, 0x84, 0x04, "OOB_TX_tune_req"
 */
int  APDU_OOBTxTuneReq (USHORT sesnum, UCHAR * apkt, USHORT len)
{
int      retvalue;
USHORT   lengthIs;
UCHAR*   pAdjusted;

UCHAR  rf_tx_freq_value[2];
UCHAR  rf_tx_data_rate;
UCHAR  rf_tx_power_level;
UCHAR  spectrum;


uint32 baud=0;
int status=0;

LCSM_EVENT_INFO eventInfo;

retvalue = APDU_HC_FAILURE;
lengthIs = 0;
pAdjusted = apkt;

   if ( apkt    == NULL )        return retvalue;
   MDEBUG (DPM_ALWAYS,"APDU_OOBTxTuneReq\n");
   MDEBUG (DPM_ALWAYS,"%x %x %x %x %x %x\n", *pAdjusted, *(pAdjusted +1), *(pAdjusted+2), *(pAdjusted+3), *(pAdjusted+4),*(pAdjusted+5));
   if (*(pAdjusted+2) != 0x04 )  return retvalue;
   pAdjusted = LengthIs(apkt, &lengthIs ); //also indexes the pointer 1 or 2 bytes, fills in the length amount

   if (lengthIs == 0)            return retvalue;
   if (sesnum == 0x00)
      sesnum = lookupHCSessionValue();
   if (sesnum == 0)
      return retvalue;
   spectrum = 0; //default to non-inverted
#ifdef USE_DSG
    if(!vlhal_oobtuner_Capable())
    {
        int nReason = OOB_QPSK_TUNE_NO_TRANSMITTER;
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","APDU_OOBTxTuneReq: No OOB Tuner Mode: Returning %d\n", nReason);
        RespOOBTxTuneCnf(nReason);
        return APDU_HC_SUCCESS;
    }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","APDU_OOBTxTuneReq: Calling vlGetDsgMode():%d\n",vlGetDsgMode());
    switch(vlGetDsgMode())
    {
        case VL_DSG_OPERATIONAL_MODE_BASIC_ONE_WAY:
        case VL_DSG_OPERATIONAL_MODE_ADV_ONE_WAY:
        {
            int nReason = OOB_QPSK_TUNE_OTHER_REASONS;
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","APDU_OOBTxTuneReq: Returning %d\n", nReason);
            RespOOBTxTuneCnf(nReason);
            return APDU_HC_SUCCESS;
        }
        break;

        case VL_DSG_OPERATIONAL_MODE_BASIC_TWO_WAY:
        case VL_DSG_OPERATIONAL_MODE_ADV_TWO_WAY:
        case VL_DSG_OPERATIONAL_MODE_IPDM:
        case VL_DSG_OPERATIONAL_MODE_IPDIRECT_UNICAST:
        {
            int nReason = OOB_QPSK_TUNE_TRANSMITTER_BUSY;
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","APDU_OOBTxTuneReq: Returning %d\n", nReason);
            RespOOBTxTuneCnf(nReason);
            return APDU_HC_SUCCESS;
        }
        break;
    }
#endif
//processing block
   rf_tx_freq_value[1] = *pAdjusted++; //swap endian
   rf_tx_freq_value[0] = *pAdjusted++; //swap endian
   rf_tx_power_level   = *pAdjusted++;
   rf_tx_data_rate  = *pAdjusted++;
   spectrum = (rf_tx_data_rate & 0x01);
   MDEBUG (DPM_ALWAYS, "rf_tx_power_level in HostControl =%x\n",rf_tx_power_level);
   MDEBUG (DPM_ALWAYS, "spectrum in HostControl =%x\n",spectrum);
   switch ( (rf_tx_data_rate & 0x00c0) >> 6)
       {
       case 0:
      baud = 256;
      break;
       case 1:
          baud = 2048;
          break;
       case 2:
          baud = 1544;
          break;
       case 3:
          baud = 3088;
          break;
       }




//spec states (rf_rx_freq_value * 0.05) + 50; //Mhz
//frontend wants units of Khz
//thus (rf_rx_freq_value * 50) + 50000; //Khz

   memset(&eventInfo, 0, sizeof(LCSM_EVENT_INFO));
   eventInfo.event      = FRONTEND_OOB_US_TUNE_REQUEST;
   eventInfo.x          = *((USHORT*)rf_tx_freq_value) * 1000; //hz
   eventInfo.y          = baud;


   eventInfo.z          = rf_tx_power_level;//spectrum;//turnerScriptToUse;
   eventInfo.dataLength = 0;
   eventInfo.dataPtr    = 0; // no copying of dataPtr
   eventInfo.SrcQueue   = POD_HOST_QUEUE;
   RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","APDU_OOBTxTuneReq:Frequency eventInfo.x:%d Hz \n",eventInfo.x);
    if(eventInfo.x < 5000000/*Hz*/ || eventInfo.x > 42000000/*Hz*/)
    {
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","APDU_OOBTxTuneReq:Invalid Frequency eventInfo.x:%d Hz \n",eventInfo.x);
    RespOOBTxTuneCnf(OOB_US_QPSK_TUNE_BAD_PARAM);
     }
     else
     {
         status = lcsm_send_event( hLCSMdev_pod, POD_HOST_QUEUE, &eventInfo);
     }

    vlg_bOutstandingOobTxTuneRequest = 1;
    return APDU_HC_SUCCESS;
//return (RespOOBTxTuneCnf(sesnum) ) ;
}



#if 0
//short method for APDU_OOBTxTuneCnf
//just response
//Do this from CardManager  after actually getting tune status from HAL_TUNER
int  RespOOBTxTuneCnf (USHORT sesnum)
{
//simple case beacause we're going to reject, we're not going to parse
LCSM_EVENT_INFO eventInfo;
unsigned char   myData[1];
unsigned char*  pMyData = NULL;
uint16 dataLen    = 1;
unsigned long tag = 0x9F8405;
int             status;

if (sesnum == 0x00)
   sesnum = lookupHCSessionValue();
if (sesnum == 0)
   return APDU_HC_FAILURE;
RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","sending RespOOBTxTuneCnf..\n");
myData[0] = OOB_DS_QPSK_TUNE_OK;//OOB_DS_QPSK_TUNE_NA;  //always reject with fail_na as we're a 1-way pod, DS is the same as TX
pMyData = (unsigned char* )&myData;

  status = hcSndApdu(sesnum, tag, dataLen, pMyData); //AM_APDU_FROM_HOST is contained inside

  memset(&eventInfo, 0, sizeof(LCSM_EVENT_INFO));
  eventInfo.event      = FRONTEND_OOB_US_TUNE_STATUS;
  eventInfo.x          = 0;
  eventInfo.y          = 0;
  eventInfo.z          = 0;
  eventInfo.dataLength = 0;
  eventInfo.dataPtr    = 0;

  lcsm_send_event( hLCSMdev_pod, POD_HOST_QUEUE, &eventInfo);

  return status;
}
#endif


int  RespOOBTxTuneCnf (uint32 tune_status)
{
//simple case beacause we're going to reject, we're not going to parse
LCSM_EVENT_INFO eventInfo;
unsigned char   myData[1];
unsigned char*  pMyData = NULL;
uint16 dataLen    = 1;
unsigned long tag = 0x9F8405;
int             status;
int sesnum;

vlg_bOutstandingOobTxTuneRequest = 0;

   sesnum = lookupHCSessionValue();
   if (sesnum == 0)
   return APDU_HC_FAILURE;
RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","sending RespOOBTxTuneCnf..\n");
// OOB_DS_QPSK_TUNE_OK;//OOB_DS_QPSK_TUNE_NA;
#ifdef USE_DSG
    if(!vlhal_oobtuner_Capable())
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","RespOOBTxTuneCnf: Parker Mode: responding with %d ################ \n", tune_status);
        myData[0] = tune_status;
        return hcSndApdu(sesnum, tag, dataLen, myData);
    }
    if(vlIsDsgOrIpduMode())
    {
        // return denied status in DSG mode
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","RespOOBTxTuneCnf: In DSG mode, responding with %d ################ \n", tune_status);
        myData[0] = tune_status;
        return hcSndApdu(sesnum, tag, dataLen, myData);
    }
#endif
    if(tune_status == OOB_US_QPSK_TUNE_BAD_PARAM)
    {
        myData[0] = OOB_US_QPSK_TUNE_BAD_PARAM;
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD"," ############### sending the OOB_US_QPSK_TUNE_BAD_PARAM ################ \n");
    }
    else
    {
        myData[0] = OOB_US_QPSK_TUNE_OK;
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ############### sending the OOB_DS_QPSK_TUNE_OK ################ \n");
    }
pMyData = (unsigned char* )myData;



//RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ############### sending the OOB_DS_QPSK_TUNE_OK ################ \n");
//RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ############### sending the OOB_DS_QPSK_TUNE_OK ################ \n");
  status = hcSndApdu(sesnum, tag, dataLen, pMyData); //AM_APDU_FROM_HOST is contained inside

  memset(&eventInfo, 0, sizeof(LCSM_EVENT_INFO));
  eventInfo.event      = FRONTEND_OOB_US_TUNE_STATUS_SENT;
  eventInfo.x          = 0;
  eventInfo.y          = 0;
  eventInfo.z          = 0;
  eventInfo.dataLength = 0;
  eventInfo.dataPtr    = 0;

  lcsm_send_event( hLCSMdev_pod, POD_HOST_QUEUE, &eventInfo);

  return status;
}





/*
 *    APDU in: 0x9F, 0x84, 0x06, "OOB_RX_tune_req"
 */
int  APDU_OOBRxTuneReq (USHORT sesnum, UCHAR * apkt, USHORT len)
{
    int      retvalue;
    USHORT   lengthIs;
    UCHAR*   pAdjusted;

    UCHAR  rf_rx_freq_value[2];
    UCHAR  rf_rx_data_rate;
    UCHAR  spectrum;
    USHORT turnerScriptToUse;

    uint32 baud=0;
    int status=0;

    LCSM_EVENT_INFO eventInfo;

    retvalue = APDU_HC_FAILURE;
    lengthIs = 0;
    pAdjusted = apkt;

    if ( apkt    == NULL )        return retvalue;
    if (*(pAdjusted+2) != 0x06 )  return retvalue;
    pAdjusted = LengthIs(apkt, &lengthIs ); //also indexes the pointer 1 or 2 bytes, fills in the length amount

    if (lengthIs == 0)            return retvalue;
    if (sesnum == 0x00)
        sesnum = lookupHCSessionValue();
    if (sesnum == 0)
        return retvalue;
    spectrum = 0; //default to non-inverted
#ifdef USE_DSG
    if(!vlhal_oobtuner_Capable())
    {
        int nReason = OOB_QPSK_TUNE_NO_TRANSMITTER;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","APDU_OOBRxTuneReq: OOB Tuner Mode: Returning %d\n", nReason);
        RespOOBRxTuneCnf(nReason);
        return APDU_HC_SUCCESS;
    }
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","APDU_OOBRxTuneReq: Calling vlGetDsgMode():%d\n",vlGetDsgMode());
    switch(vlGetDsgMode())
    {
        case VL_DSG_OPERATIONAL_MODE_BASIC_ONE_WAY:
        case VL_DSG_OPERATIONAL_MODE_ADV_ONE_WAY:
        case VL_DSG_OPERATIONAL_MODE_BASIC_TWO_WAY:
        case VL_DSG_OPERATIONAL_MODE_ADV_TWO_WAY:
        case VL_DSG_OPERATIONAL_MODE_IPDM:
        case VL_DSG_OPERATIONAL_MODE_IPDIRECT_UNICAST:
        {
            int nReason = OOB_QPSK_TUNE_OTHER_REASONS;
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","APDU_OOBRxTuneReq: Returning %d\n", nReason);
            RespOOBRxTuneCnf(nReason);
            return APDU_HC_SUCCESS;
        }
        break;
    }
#endif
    //processing block
    rf_rx_freq_value[1] = *pAdjusted++; //swap endian
    rf_rx_freq_value[0] = *pAdjusted++; //swap endian

    rf_rx_data_rate  = *pAdjusted++;
    spectrum = (rf_rx_data_rate & 0x01);
    MDEBUG (DPM_ALWAYS, "spectrum in HostControl =%x\n",spectrum);
    switch ( (rf_rx_data_rate & 0x00c0) >> 6)
    {
        case 0:
        case 1:
            baud = 2048;
            break;
        case 2:
            baud = 1544;
            break;
        case 3:
            baud = 3088;
            break;
    }

   turnerScriptToUse = DEFAULT_SCRIPT_TYPE;

    if ((gPODType & 0xffff0000) == 0x00)
    {
        switch(gPODType & 0x0000FF00)
        {
            case 0x0000:
            turnerScriptToUse = MOTOROLA_SCRIPT_TYPE;
            break;
            case 0x0100:
            turnerScriptToUse = SA_SCRIPT_TYPE;
            break;
            case 0x0200:
            turnerScriptToUse = SCM_SCRIPT_TYPE;
            break;
            case 0x0300:
            turnerScriptToUse = NDS_SCRIPT_TYPE;
            break;
            default:
            turnerScriptToUse = DEFAULT_SCRIPT_TYPE;
            break;
        }
    }


    //spec states (rf_rx_freq_value * 0.05) + 50; //Mhz
    //frontend wants units of Khz
    //thus (rf_rx_freq_value * 50) + 50000; //Khz

    memset(&eventInfo, 0, sizeof(LCSM_EVENT_INFO));
    eventInfo.event      = FRONTEND_OOB_DS_TUNE_REQUEST;
    eventInfo.x          = (*((USHORT*)rf_rx_freq_value) * 50) + 50000; //Khz
    eventInfo.y          = baud;

    //need to send the spectrum as inverted or non-inverted
    turnerScriptToUse   |= (spectrum << 31);
    eventInfo.z          = spectrum;//turnerScriptToUse;
    eventInfo.dataLength = 0;
    eventInfo.dataPtr    = 0; // no copying of dataPtr
    eventInfo.SrcQueue   = POD_HOST_QUEUE;
    gExpectingOOBevent = 1;

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","APDU_OOBRxTuneReq:eventInfo.x:%d \n",eventInfo.x);
    if(eventInfo.x < 70000/*KHz*/ || eventInfo.x > 130000/*kHz*/)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","APDU_OOBRxTuneReq:eventInfo.x:%d Invalid Frequency\n",eventInfo.x);
        RespOOBRxTuneCnf(OOB_DS_QPSK_TUNE_BAD_PARAM);
    }
    else
    {

        status = lcsm_send_event( hLCSMdev_pod, POD_HOST_QUEUE, &eventInfo);
            if ( status != 0 )
            {
        //      return (RespOOBRxTuneCnf(OOB_DS_QPSK_TUNE_FAILED));
            }
    }
/*
   if (TRUE == ndstytpe )
      {
      MDEBUG (DPM_ALWAYS, "pod > NDS_POD_DETECTED = freq parm = %u \n", *((USHORT*)rf_rx_freq_value) );
      MDEBUG (DPM_ALWAYS, "pod > NDS_POD_DETECTED = Testing for 500 \n");

      if ( *((USHORT*)rf_rx_freq_value) == 500 )   // TUNE_FREQ = 75 Mhz
         {
         MDEBUG (DPM_ALWAYS, "pod > NDS_POD_DETECTED = Found Match, sending quick event \n");
         //send a quick response of success
         memset(&eventInfo, 0, sizeof(LCSM_EVENT_INFO));
         eventInfo.event      = FRONTEND_OOB_DS_TUNE_STATUS;
         eventInfo.x          = OOB_DS_QPSK_TUNE_OK;
         eventInfo.y          = 0;
         eventInfo.z          = 0;
         eventInfo.dataLength = 0;
         eventInfo.dataPtr    = 0; // no copying of dataPtr
         //lcsm_send_timed_event( hLCSMdev_pod, POD_HOST_QUEUE, &eventInfo, 100 );
         status = lcsm_send_event( hLCSMdev_pod, POD_HOST_QUEUE, &eventInfo);
         }
      }
   else
      {
      memset(&eventInfo, 0, sizeof(LCSM_EVENT_INFO));
      eventInfo.event      = POD_HC_OOB_TUNE_RESP;
      eventInfo.x          = OOB_DS_QPSK_TUNE_TIMED_OUT;
      eventInfo.y          = 0;
      eventInfo.z          = 0;
      eventInfo.dataLength = 0;
      eventInfo.dataPtr    = 0; // no copying of dataPtr
//we have print statements
//5 sec is too too long
//2 sec is too short
      timerHandle = lcsm_send_timed_event( hLCSMdev_pod, POD_HOST_QUEUE, &eventInfo, 4000 ); //we have 500ms to get the lock else its replied with fail.
      }
*/
    if ( status != 0 )
    {
        //return (RespOOBRxTuneCnf(OOB_DS_QPSK_TUNE_FAILED));
    }

    vlg_bOutstandingOobRxTuneRequest = 1;
    return APDU_HC_SUCCESS;
}

/*
 *    APDU in: 0x9F, 0x84, 0x07, "OOB_RX_tune_cnf"
 */
int  APDU_OOBRxTuneCnf (USHORT sesnum, UCHAR * apkt, USHORT len)
{
//not implementing, this is an async reponse from events inside the tuning system
//those events are engaged by RespOOBRxTuneCnf (but beacuse its async we'll need to look-up the session

   return APDU_HC_FAILURE;

}

//short method for APDU_OOBRxTuneCnf
//just response
int  RespOOBRxTuneCnf (uint32 rtndStatus)
{
unsigned char   myData[1];
unsigned char*  pMyData = NULL;
uint16   dataLen  = 1;
unsigned long tag = 0x9F8407;
USHORT   sesnum;

sesnum = lookupHCSessionValue();  //need to look up session
   if (sesnum == 0)
      return APDU_HC_FAILURE;
vlg_bOutstandingOobRxTuneRequest = 0;
#ifdef USE_DSG
    if(!vlhal_oobtuner_Capable())
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","RespOOBRxTuneCnf: OOB Tuner Mode: responding with %d ################ \n", rtndStatus);
        myData[0] = rtndStatus;
        return hcSndApdu(sesnum, tag, dataLen, myData);
    }
    if(vlIsDsgOrIpduMode())
    {
        // return denied status in DSG mode
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","RespOOBRxTuneCnf: In DSG mode, responding with %d ################ \n", rtndStatus);
        myData[0] = rtndStatus;
        return hcSndApdu(sesnum, tag, dataLen, myData);
    }
#endif
   if (!gExpectingOOBevent)
      {
      return APDU_HC_SUCCESS;
      }
   gExpectingOOBevent = 0;


   // cancel the timeout event if we get another event
   if ( rtndStatus != OOB_DS_QPSK_TUNE_TIMED_OUT )
   {
       lcsm_cancel_timed_event( hLCSMdev_pod, timerHandle );
   }

   switch (rtndStatus)
       {
       case OOB_DS_QPSK_TUNE_OK:
          {
          myData[0] = OOB_DS_QPSK_TUNE_OK;
          break;
          }
       case OOB_DS_QPSK_TUNE_NA:
          {
          myData[0] = OOB_DS_QPSK_TUNE_NA;
          break;
          }
       case OOB_DS_QPSK_TUNE_FAILED:
          {
          // Brendan - 6/04 - even if the tuner can't get a lock we should respond with "tuning granted"
          // This shows the the pod has control of the OOB tuner.
          // Commented Sep-02-2008: myData[0] = OOB_DS_QPSK_TUNE_OK;
          //myData[0] = OOB_DS_QPSK_TUNE_BAD_PARAM; // Changed Sep-02-2008: OOB_DS_QPSK_TUNE_FAILED translates "Transmitter Busy"  which is incorrect //OOB_DS_QPSK_TUNE_FAILED
        myData[0] = OOB_DS_QPSK_TUNE_OK;
          break;
          }
       case OOB_DS_QPSK_TUNE_BAD_PARAM:
          {
          myData[0] = OOB_DS_QPSK_TUNE_BAD_PARAM;
          break;
          }
       case OOB_DS_QPSK_TUNE_TIMED_OUT:
          {
          myData[0] = OOB_DS_QPSK_TUNE_TIMED_OUT;
          break;
          }

       default:
          {
          myData[0] = OOB_DS_QPSK_TUNE_IGNORE;
          break;
          }
       }  //end switch
    if (myData[0] != OOB_DS_QPSK_TUNE_IGNORE)   //send back response
       {
       pMyData = (unsigned char* )&myData;
       return (hcSndApdu(sesnum, tag, dataLen, pMyData)); //AM_APDU_FROM_HOST is contained inside
       }
    else
       return APDU_HC_FAILURE;
}


/*
 *    APDU in: 0x9F, 0x84, 0x08, "inband_tune"
 */

// ####################  used for HOMING ############################
// #### Should only be accepted if we're in standby  ################
int  APDU_InbandTune (USHORT sesnum, UCHAR * apkt, USHORT len)
{
int      retvalue;
int      status;

USHORT   lengthIs;
UCHAR*   pAdjusted;

UCHAR  tune_type;
UCHAR  source_id[2];
USHORT sourceId = 0;
UCHAR  tune_freq_value[2];
UCHAR  modulation_value;
int time;
LCSM_EVENT_INFO eventInfo;

RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","APDU_InbandTune: ## 1\n");
retvalue = APDU_HC_FAILURE;
lengthIs = 0;
pAdjusted = apkt;

   if ( apkt    == NULL )
    return retvalue;
RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","APDU_InbandTune: ## 2\n");
   if (*(pAdjusted+2) != 0x08 )
    return retvalue;
RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","APDU_InbandTune: ## 3\n");
   pAdjusted = LengthIs(apkt, &lengthIs ); //also indexes the pointer 1 or 2 bytes, fills in the length amount

   if (lengthIs == 0)
      return retvalue;
RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","APDU_InbandTune: ## 4\n");
   if (sesnum == 0x00)
      sesnum = lookupHCSessionValue();
RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","APDU_InbandTune: ## 5\n");
   if (sesnum == 0)
      return retvalue;
RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","APDU_InbandTune: ## 6\n");
   tune_type = 0;
   modulation_value = 0;
   memset(source_id, 0, sizeof(source_id));

    memset(tune_freq_value, 0, sizeof(tune_freq_value));
     

//processing block
   tune_type = *pAdjusted++;
   tune_type &= 0x03;

   gExpectingIBevent = 1;
   gExpectingIBeventNext++;

   if (tune_type == 0)
      {
      source_id[0] = *pAdjusted++; //swap endian
      source_id[1] = *pAdjusted++; //swap endian
        sourceId = ((USHORT)source_id[0] << 8 ) | (USHORT)source_id[1] ;
      }
   else if (tune_type == 1)
      {
#ifdef USE_IPDIRECT_UNICAST
        int freq_convert;
#endif
      tune_freq_value[1]  = *pAdjusted++; //swap endian
      tune_freq_value[0]  = *pAdjusted++; //swap endian
      modulation_value    = *pAdjusted++;

#ifdef USE_IPDIRECT_UNICAST
      freq_convert = (((tune_freq_value[1] * 256) + tune_freq_value[0]) * 50);
      HostCtrlInbandTunerAcquire(freq_convert, modulation_value);
#endif
      }
#ifdef USE_IPDIRECT_UNICAST
   else if (tune_type == 2) // release tuner
      {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","release tuner \n");
        HostCtrlInbandTunerRelease();
        RespInbandTuneCnf(INBAND_TUNE_TUNER_RELEASE_ACK, 2);
      }
#endif
   else
      {
//      return (RespInbandTuneCnf(OOB_DS_QPSK_TUNE_BAD_PARAM, gExpectingIBeventNext));     //reject as invalid; Same value as IB band
      tune_type = 0;
      }
#if 0
//for reference
fields:
x == 0x0 for source_id, 0x1 for frequency
y == frequency/source_id
z == 0x0 for QAM64, 0x1 for QAM256
#endif

    memset(&eventInfo, 0, sizeof(LCSM_EVENT_INFO));
    // firmware upgrade tune
    if (firmupgradeFlag == TRUE )
    {
        // first change mrTopControl to the idle state
        //eventInfo.event      = MR_CHANNEL_CTRL_IDLE;
/*    eventInfo.event      = MR_CHANNEL_CTRL_IDLE;
        eventInfo.dataPtr    = NULL;
        eventInfo.dataLength = 0;
     //   lcsm_send_event( hLCSMdev_pod, SD_MR_CONTROL, &eventInfo );
       lcsm_send_event( hLCSMdev_pod, POD_HOST_QUEUE, &eventInfo );*/

        // then switch mrTopControl to the homing state
        //eventInfo.event = MR_HOMING_REQ;
     eventInfo.event = FRONTEND_INBAND_TUNE_REQUEST;
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","tune_freq_value:%d \n",tune_freq_value);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","modulation_value:%d \n",modulation_value);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","tune_freq_value:0x%08X \n",tune_freq_value);
    //modulation_value = 0;
        // freq/modulation tune
        if ( tune_type == 1 )
        {
            eventInfo.x = (*((USHORT*)tune_freq_value) * 50);
            //eventInfo.y = modulation_value + 1 /* as an added offset*/;
	    switch(modulation_value)
	    {
	      case 0:
		eventInfo.y = 8;
		break;
	      case 1:
		eventInfo.y = 16;
		break;
	      default:
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Modulation is out of range modulation_value = %d\n",modulation_value);
		break;
	    }
            eventInfo.z = 0;
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","eventInfo.x::: %deventInfo.y:%d \n",eventInfo.x,eventInfo.y);
        }
        // source id tune
        else
        {
            //int Mod=0, Freq=0;
            //vlHostInBandTuneSourceId((int) source_id,&Mod, &Freq);
            eventInfo.x = 0;
            eventInfo.y = 0;
            eventInfo.z = sourceId;
        }
        eventInfo.dataPtr = NULL;
        eventInfo.dataLength = 0;
        eventInfo.SrcQueue = POD_HOST_QUEUE;

        if(vlg_InbanTuneStart)
        {
            //RespInbandTuneCnf (0x20, 2);//Sending Busy
            RespInbandTuneCnf (0, 2);
        }
        else
        {
            vlg_InbanTuneStart = 1;

        // lcsm_send_event( hLCSMdev_pod, SD_MR_CONTROL, &eventInfo );
        //lcsm_send_event( hLCSMdev_pod, GATEWAY_QUEUE, &eventInfo );
            lcsm_send_event( hLCSMdev_pod, POD_HOST_QUEUE, &eventInfo );
            //RespInbandTuneCnf (0, 2);
        }
}

//no requirement to wait for acquire and lock
//no requirement to wait
//If the parmaters are accepted, then its successful, even if tuning hasn't finished
//else reply with a invalid of sometype.
//even though all the above are not required, our archetecture does require a return event
//before response can be given.

//setup our own timeout protection in case mr-control state machine throws the message away
  /* memset(&eventInfo, 0, sizeof(LCSM_EVENT_INFO));
   eventInfo.event      = POD_HC_IB_TUNE_RESP;
   eventInfo.x          = 0;
   eventInfo.y          = OOB_DS_QPSK_TUNE_FAILED;  //same as inband value
   eventInfo.z          = 0;
   eventInfo.dataLength = 0;
   eventInfo.dataPtr    = 0; // no copying of dataPtr*/
#if 0
vlg_InbanTuneSuccess = 0;
    if(eventInfo.x == 717000 && eventInfo.y == 1)
    {
    }
    else
    {
        time = 14;
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",">>>>>>>>>>>>>>>     Waiting Begin for %d Sec \n",time);
        sleep(time);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",">>>>>>>>>>>>>>>     Waiting End for %d Sec \n",time);
        if(vlg_InbanTuneSuccess == 0)
        {
            RespInbandTuneCnf(1, 1);

        }
    }
#endif
#if 0
   This timer is redundant and not needed


   //we have 3.5 sec, don't need to wait for lock. just protection
   //status = lcsm_send_timed_event( hLCSMdev_pod, POD_HOST_QUEUE, &eventInfo, 3500 );
   //if ( status != 0 )
      {
//      return (RespInbandTuneCnf(OOB_DS_QPSK_TUNE_FAILED, gExpectingIBeventNext));
      }
#endif
   return APDU_HC_SUCCESS;
}

// ####################  used for HOMING ############################
// #### Should only be accepted if we're in standby  ################

/*
 *    APDU in: 0x9F, 0x84, 0x09, "inband_tune_cnf"
 */
int  APDU_InbandTuneCnf (USHORT sesnum, UCHAR * apkt, USHORT len)
{
//not implementing, this is an reponse called by us not by an ADPU from POD to Host
//we will send this ADPU back to POD via short method. (below)
   return APDU_HC_FAILURE;

}

//short method for APDU_InbandTuneCnf
//just response
int  RespInbandTuneCnf (uint32 tuneStatus, uint32 seqNum)
{
unsigned char myData[2];
unsigned char*  pMyData = NULL;
uint16 dataLen = 2;
unsigned long tag = 0x9F8409;
USHORT sesnum;

   if (!gExpectingIBevent)
      {
      return APDU_HC_SUCCESS;
      }
//   else if (gExpectingIBeventNext != seqNum )
//      {
//      return APDU_HC_SUCCESS;
//      }
   gExpectingIBevent = 0;

   sesnum = lookupHCSessionValue();  //need to look up session
   if (sesnum == 0)
      return APDU_HC_FAILURE;

  //send back ADPU response
   /*
    *  Brendan- 5/20
    *  CableLabs says this should always respond affirmative when attempting a tune
    *  during firmware upgrade.  This is not indicative of actually getting a tuner
    *  lock but lets the cable card know that it has control of the IB tuner.
    */
#if 0
   if ( 1/*firmupgradeFlag == TRUE*/ )
   {
    myData[0] = seqNum;
    if(tuneStatus == 0)
    {
        myData[1] = OOB_DS_QPSK_TUNE_OK;
    }
    else
    {
           myData[1] = OOB_DS_QPSK_TUNE_FAILED;
    }
   }

   else
   {
       myData[0] = (tuneStatus)? OOB_DS_QPSK_TUNE_TIMED_OUT : OOB_DS_QPSK_TUNE_OK   ;
       pMyData = (unsigned char* )&myData;
   }
#endif

    myData[0] = seqNum;
    if(tuneStatus == 0)
    {
        myData[1] = INBAND_TUNE_OK;//INBAND_TUNE_OK;
    }
    else if(tuneStatus == 0x20)
    {
        myData[1] = INBAND_TUNE_TUNER_BUSY;
    }
    else
    {
           myData[1] = tuneStatus;

    }
       return (hcSndApdu(sesnum, tag, dataLen, myData)); //AM_APDU_FROM_HOST is contained inside
}

// #######################################################################

void setFirmupgradeFlag( Bool on )
{
    LCSM_EVENT_INFO eventInfo;

    if(on == TRUE)
    {
    memset(&eventInfo, 0, sizeof(LCSM_EVENT_INFO));
     eventInfo.event = FRONTEND_INBAND_TUNE_REQUEST;
    lcsm_send_event( hLCSMdev_pod, GATEWAY_QUEUE, &eventInfo );
    }
    firmupgradeFlag = on;
}

#ifdef __cplusplus
}
#endif

#ifdef USE_IPDIRECT_UNICAST
#define  RMF_PODSRV_EVENT_INB_TUNE 0xB003
#define  RMF_PODSRV_EVENT_INB_TUNE_RELEASE 0xB004

int HostCtrlInbandTunerAcquire(int frequency, UCHAR modulation)
{

  LCSM_EVENT_INFO eventInfo;

  memset(&eventInfo, 0, sizeof(LCSM_EVENT_INFO));
  eventInfo.event = FRONTEND_INBAND_TUNE_REQUEST;
  eventInfo.dataPtr = NULL;
  eventInfo.dataLength = 0;
  eventInfo.SrcQueue = POD_HOST_QUEUE;
  eventInfo.x = frequency;
  switch(modulation)
  {
    case 0:
      eventInfo.y = 8;
      break;
    case 1:
      eventInfo.y = 16;
      break;
    default:
      RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Modulation is out of range modulation_value = %d\n",modulation);
      break;
  }
  eventInfo.z = 0;

  lcsm_send_event( hLCSMdev_pod, POD_HOST_QUEUE, &eventInfo );
  return 0;
}

int HostCtrlInbandTunerRelease(void)
{
  LCSM_EVENT_INFO eventInfo;

  memset(&eventInfo, 0, sizeof(LCSM_EVENT_INFO));
  eventInfo.event = FRONTEND_INBAND_TUNE_RELEASE;
  eventInfo.dataPtr = NULL;
  eventInfo.dataLength = 0;
  eventInfo.SrcQueue = POD_HOST_QUEUE;
  lcsm_send_event( hLCSMdev_pod, POD_HOST_QUEUE, &eventInfo );
  return 0;
}
#endif
