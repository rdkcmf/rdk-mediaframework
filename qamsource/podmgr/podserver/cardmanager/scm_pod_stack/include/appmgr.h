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



#ifndef _PDT_APPMGR
#define    _PDT_APPMGR

#ifdef __cplusplus
extern "C" {
#endif

#include <global_queues.h>  //Hannah

#ifndef __UTILS_H
typedef unsigned char UCHAR;
#endif

#ifndef __UTILS_H
typedef unsigned short USHORT;
typedef unsigned int   ULONG;
#endif



// assumes lcsm_userSpaceInterface.h is #include'd
// assumes handle is global LCSM handle
#define REGISTER_RESOURCE( resourceId, queueId, maxSessions )    \
    {    \
        LCSM_EVENT_INFO msg;    \
        msg.event = POD_RESOURCE_PRESENT;    \
        msg.x = ( resourceId );    \
        msg.y = ( queueId );    \
        msg.z = ( maxSessions );    \
        msg.dataPtr    = NULL;      \
        msg.dataLength = 0;         \
        lcsm_send_event( handle, POD_RSMGR_QUEUE, &msg  );    \
    }

#define M_RESRC_MGR_ID            0x00010041
#define M_MMI_ID                  0x00400081

#define M_APP_INFO_ID_V1             0x00020081 //Both versions need to be supported as per CCIF June 02 2009
#define M_APP_INFO_ID_V2             0x00020082

#define M_APP_INFO_ID             M_APP_INFO_ID_V2 

#define M_SPEC_APP_ID             0x00900042
#define M_GEN_FEATURE_ID          0x002A0044    // 0x002A0043
#define M_HOMING_ID               0x00110042
#define M_SYS_TIME_ID             0x00240041

#define M_CA_ID                 0x00030081
#define M_CARD_RES_ID           0x002600C1
#define M_CARD_MIB_ACC_ID       0x005A0041
#define M_HOST_CTL_ID_V1        0x00200081
#define M_HOST_CTL_ID_V2        0x00200082
#ifdef USE_IPDIRECT_UNICAST
#define M_HOST_CTL_ID           M_HOST_CTL_ID_V2
#else
#define M_HOST_CTL_ID           M_HOST_CTL_ID_V1
#endif
#define M_CP_ID_V2          0x00B00102
#define M_CP_ID_V3          0x00B00103
#define M_CP_ID_V4          0x00B00104
#ifdef USE_IPDIRECT_UNICAST
#define M_CP_ID             M_CP_ID_V3
#else
#define M_CP_ID             M_CP_ID_V2
#endif

#define M_GEN_DIAG_ID_V1        0x01040081
#define M_GEN_DIAG_ID_V2        0x01040082
#define M_GEN_DIAG_ID        M_GEN_DIAG_ID_V2

#define M_SYSTEM_CONTROL_T1_ID    0x002B0041
#define M_SYSTEM_CONTROL_T2_ID    0x002B0081
#define M_SYSTEM_CONTROL_ID      M_SYSTEM_CONTROL_T2_ID
#define M_CARD_RES        0x002600C1

#define M_DSG_ID_V2            0x00040042
#define M_DSG_ID_V3            0x00040043

#ifdef USE_IPDIRECT_UNICAST
#define M_DSG_ID               M_DSG_ID_V3
#else
#define M_DSG_ID               M_DSG_ID_V2
#endif // USE_IPDIRECT_UNICAST

#define M_HOST_ADD_PRT     0x01000041
#define M_HEAD_END_COMM     0x002C0041

#define M_IPDM_ID_V1    0x00050041
#define M_IPDM_ID       M_IPDM_ID_V1

#define M_IPDIRECT_UNICAST_ID_V1    0xC0000081
#define M_IPDIRECT_UNICAST_ID       M_IPDIRECT_UNICAST_ID_V1

#define M_EXT_CHAN_ID_V1           0x00A00041
#define M_EXT_CHAN_ID_V2           0x00A00042
#define M_EXT_CHAN_ID_V3           0x00A00043
#define M_EXT_CHAN_ID_V4           0x00A00044
#define M_EXT_CHAN_ID_V5           0x00A00045
#define M_EXT_CHAN_ID_V6           0x00A00046
#define M_EXT_CHAN_ID              M_EXT_CHAN_ID_V6

#define M_LOW_SPEED_ID          0x00608043//0x00605043 //0x00608043
#define M_GEN_IPPV_ID        0x00800081
// Values of the 3rd byte of the APDU Tag Value

#define MAX_HOST_RESOURCES         30 //This value will increase when new resources are added

#define M_RES_MNGR_CLS           0x01   //Resource No.1
#define M_APP_INFO_CLS           0x02
#define M_CA_CLS                 0x03
#define M_DSG_CLS                0x04
#define M_IPDM_CLS               0x05
#define M_IPDIRECT_UNICAST_CLS   0x05
#define M_HOMING_CLS             0x11
#define M_HOST_CTL_CLS           0x20
#define M_SYS_TIME_CLS           0x24
#define M_CARD_RES_CLS              0x26
#define M_GEN_FET_CTL_CLS           0x2A
#define M_SYTEM_CTL_CLS           0x2B
#define M_HEAD_END_COMM_CLS           0x2C
#define M_MMI_CLS            0x40
#define M_LOW_SPD_COM_CLS        0x60
#define M_GEN_IPPV_SUP_CLS           0x80
#define M_SAS_CLS                       0x90
#define M_EXT_CHNL_CLS           0xA0
#define M_CP_CLS            0xB0
#define M_GEN_DIAG_SUP_CLS          0x0104//Resource No.18
#define M_CARD_MIB_ACC_CLS          0x5A//Resource No.19
#define M_HOST_ADD_PRT_CLS          0x100//Resource No.20

typedef struct ResClass_s
{
    int ResClass;
    unsigned char ResourceName[32];
}ResClass_t;

typedef struct ResSenInfo
{
    unsigned char OpenSesStatus;
    unsigned char SesNum;
    unsigned char NumSes;
    int ResourceId;
}ResSenInfo_t;
typedef struct CardHostResInfo
{
    int Index;
    ResSenInfo_t ResInf[MAX_HOST_RESOURCES];
}CardHostResInfo_t;
#define    EXIT_REQUEST                0xff

#define    POD_APPINFO_REQ                0x20
#define    POD_APPINFO_CNF                0x21
#define    POD_APPINFO_SERVERQUERY        0x22
#define    POD_APPINFO_SERVERREPLY        0x23

#define    POD_PROFILE_INQ                0x10
#define    POD_PROFILE_REPLY            0x11
#define    POD_PROFILE_CHANGED            0x12

#define    POD_SYSTIME_INQ                0x9f8442    // want whole 3-byte tag, thank you
#define    POD_SYSTIME                    0x43

#define    POD_CA_INFOINQ                0x30
#define    POD_CA_INFO                    0x31
#define    POD_CA_PMT                    0x32
#define    POD_CA_PMTREPLY                0x33
#define    POD_CA_UPDATE                0x34


#define    POD_FEATURE_LISTREQ            0x02
#define    POD_FEATURE_LIST            0x03
#define    POD_FEATURE_LISTCNF            0x04
#define    POD_FEATURE_LISTCHANGED        0x05
#define    POD_FEATURE_PARMREQ            0x06
#define    POD_FEATURE_PARM            0x07
#define    POD_FEATURE_PARMCNF            0x08

#define POD_DIAG_REQ                0x00
#define    POD_DIAG_CNF                0x01

#define    POD_SAS_CONNECTREQ            0x00
#define POD_SAS_CONNECTCNF            0x01
#define POD_SAS_DATAREQ                0x02
#define POD_SAS_DATAAV                0x03
#define POD_SAS_DATACNF                0x04
#define    POD_SAS_SERVERQUERY            0x05
#define    POD_SAS_SERVERREPLY            0x06

#define    POD_HOST_OOBTX_TUNEREQ        0x04
#define    POD_HOST_OOBTX_TUNECNF        0x05
#define    POD_HOST_OOBRX_TUNEREQ        0x06
#define    POD_HOST_OOBRX_TUNECNF        0x07
#define    POD_HOST_IBTUNE                0x08
#define    POD_HOST_IBTUNE_CNF            0x09

#define POD_IPPV_PROGRAMREQ            0x00
#define POD_IPPV_PROGRAMCNF            0x01
#define POD_IPPV_PURCHASEREQ        0x02
#define POD_IPPV_PURCHASECNF        0x03
#define POD_IPPV_CANCELREQ            0x04
#define POD_IPPV_CANCELCNF            0x05
#define POD_IPPV_HISTORYREQ            0x06
#define POD_IPPV_HISTORYCNF            0x07

#define POD_MMI_OPENREQ                0x20
#define POD_MMI_OPENCNF                0x21
#define POD_MMI_CLOSEREQ            0x22
#define POD_MMI_CLOSECNF            0x23

#define POD_HOMING_OPEN                    0x90
#define POD_HOMING_CANCELLED            0x91
#define POD_HOMING_OPENREPLY            0x92
#define POD_HOMING_ACTIVE                0x93
#define POD_HOMING_COMPLETE                0x94
#define POD_HOMING_FIRMUPGRADE            0x95
#define POD_HOMING_FIRMUPGRADE_REPLY    0x96
#define POD_HOMING_FIRMUPGRADE_COMPLETE    0x97

#define POD_SYSTEM_HOSTINFO_REQ            0x00
#define POD_SYSTEM_HOSTINFO_RESP        0x01
#define POD_SYSTEM_CODEVER_TABLE        0x02
#define POD_SYSTEM_CODEVER_TABLE_REPLY    0x03
#define POD_SYSTEM_HOSTDOWNLOAD_CTL        0x04
#define POD_SYSTEM_CODEVER_TABLE2        0x05

#define POD_CPROT_OPENREQ                0x00
#define POD_CPROT_OPENCNF                0x01
#define POD_CPROT_DATAREQ                0x02
#define POD_CPROT_DATACNF                0x03
#define POD_CPROT_SYNCREQ                0x04
#define POD_CPROT_SYNCCNF                0x05
#define POD_CPROT_VAL_REQ                0x06
#define POD_CPROT_VAL_CNF                0x07

#define POD_CARD_RES_STRM_PRF                0x10
#define POD_CARD_RES_STRM_PRF_CNF            0x11
#define POD_CARD_RES_PRM_PRF                0x12
#define POD_CARD_RES_PRM_PRF_CNF            0x13
#define POD_CARD_RES_ES_PRF                0x14
#define POD_CARD_RES_ES_PRF_CNF                0x15
#define POD_CARD_RES_REQ_PIDS                0x16
#define POD_CARD_RES_REQ_PIDS_CNF            0x17


#define POD_CARD_MIB_ACC_SNMP_REQ            0x00
#define POD_CARD_MIB_ACC_SNMP_REPLY            0x01
#define POD_CARD_MIB_ACC_ROOToid_REQ            0x02
#define POD_CARD_MIB_ACC_ROOToid_REPLY            0x03


#define POD_LOWSPD_COMMSCMD                0x00
#define POD_LOWSPD_CONN_DESCRIPTOR        0x01
#define POD_LOWSPD_COMMSREPLY            0x02
#define POD_LOWSPD_COMMS_SENDLAST        0x03
#define POD_LOWSPD_COMMS_SENDMORE        0x04
#define POD_LOWSPD_COMMS_RCVLAST        0x05
#define POD_LOWSPD_COMMS_RCVMORE        0x06

#define POD_XCHAN_NEWFLOW_REQ            0x00    // EXTENDED CHANNEL (1)
#define POD_XCHAN_NEWFLOW_CNF            0x01
#define POD_XCHAN_DELFLOW_REQ            0x02
#define POD_XCHAN_DELFLOW_CNF            0x03
#define POD_XCHAN_LOSTFLOW_IND            0x04
#define POD_XCHAN_LOSTFLOW_CNF            0x05
#define POD_XCHAN_INQ_DSGMODE            0x06    // EXTENDED CHANNEL (2)
#define POD_XCHAN_SET_DSGMODE            0x07
#define POD_XCHAN_DSGPKT_ERROR            0x08

typedef struct apdu
{
    UCHAR    tag1, tag2, tag3;// the actual apdu tag consists of 3 bytes.
    char    descr[31];// tag description
    SYSTEM_QUEUES    queue;// pod resource queue to invoke the resource thread
} APDU;

void AM_APDU_FROM_POD(USHORT SessNumber, UCHAR * pApdu, USHORT Length);

void init_session_entry (void);
USHORT find_available_session ( void );
int AM_POD_EJECTED( void );

/**************************************************
 *    APDU Functions to handle RESOURCE MANAGER
 *************************************************/
int     Host_ProfileChanged (USHORT sesnum);
int  APDU_ProfileInq (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_ProfileReply (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_ProfileChanged (USHORT sesnum, UCHAR * apkt, USHORT len);

/**************************************************
 *    APDU Functions to handle APPLICATION INFO
 *************************************************/
int  APDU_AppInfoReq (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_AppInfoCnf (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_ServerQuery (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_ServerReply (USHORT sesnum, UCHAR * apkt, USHORT len);

/**************************************************
 *    APDU Functions to handle SYSTEM TIME
 *************************************************/
int  APDU_SysTimeInq (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_SysTime (USHORT sesnum, UCHAR * apkt, USHORT len);

/**************************************************
 *    APDU Functions to handle CA SUPPORT
 *************************************************/
int  APDU_CAInfoInq (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_CAInfo (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_CAPmt (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_CAPmtReply (USHORT sesnum, UCHAR * apkt, USHORT len,UCHAR *pDatLenSize, unsigned long *pDataLen);
int  APDU_CAUpdate (USHORT sesnum, UCHAR * apkt, USHORT len);

/******************************************************
 *    APDU Functions to handle GENERIC FEATURE CONTROL
 *****************************************************/
int  APDU_Feature (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_FeatureListReq (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_FeatureList (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_FeatureListCnf (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_FeatureListChanged (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_FeatureParmReqFromCard(USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_FeatureParmFromCard (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_FeatureParmCnfFromCard (USHORT sesnum, UCHAR * apkt, USHORT len);
int featureGetList();
/********************************************************
 *    APDU Functions to handle GENERIC DIAGNOSTIC SUPPORT
 *******************************************************/
int  APDU_DiagReq (USHORT sesnum, UCHAR * apkt, USHORT len);
//int  APDU_DiagCnf (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_DiagCnf (USHORT sesnum, UCHAR Numdiags,UCHAR* ptr, USHORT Length);

/**************************************************
 *    APDU Functions to handle SAS
 *************************************************/
int  APDU_SASConnectReq (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_SASConnectCnf (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_SASDataReq (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_SASDataAv (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_SASDataCnf (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_SASServerQuery (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_SASServerReply (USHORT sesnum, UCHAR * apkt, USHORT len);

/**************************************************
 *    APDU Functions to handle HOST CONTROL
 *************************************************/
int  APDU_OOBTxTuneReq (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_OOBTxTuneCnf (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_OOBRxTuneReq (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_OOBRxTuneCnf (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_InbandTune (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_InbandTuneCnf (USHORT sesnum, UCHAR * apkt, USHORT len);

/**************************************************
 *    APDU Functions to handle EXTENDED CHANNEL
 *************************************************/
int  APDU_XCHAN2POD_newMpegFlowReq (USHORT sesnum, ULONG nPid);
int  APDU_XCHAN2POD_lostFlowInd(USHORT sesnum, ULONG flowId, UCHAR reason);
int  APDU_XCHAN2POD_deleteFlowReq (USHORT sesnum, ULONG flowId);
int  APDU_XCHAN2POD_SendIPUFlowReq (USHORT sesnum, unsigned char * pMacAddress,
                                    unsigned long nOptionLen, unsigned char * paOptions);
int  APDU_XCHAN2POD_newIPMFlowReq (USHORT sesnum, unsigned char bytReserved, unsigned long nGroupId);
#ifdef USE_IPDIRECT_UNICAST
int  APDU_XCHAN2POD_newIPDUFlowReq (USHORT sesnum, unsigned char * pcReserved, long int returnQueue);  // IPDU register flow
#endif
int  APDU_XCHAN2POD_SendSOCKETFlowReq (USHORT sesnum, unsigned char protocol,
                                       unsigned short nLocalPortNum,
                                       unsigned short nRemotePort,
                                       unsigned long  nRemoteAddrType,
                                       unsigned long  nAddressLength,
                                       unsigned char * pAddress,
                                       unsigned char  nConnectionTimeout);
int  APDU_NewFlowReq (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_NewFlowCnf (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_DeleteFlowReq (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_DeleteFlowCnf (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_LostFlowInd (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_LostFlowCnf (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_InqDSGMode (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_SetDSGMode (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_DSGPacketError (USHORT sesnum, UCHAR * apkt, USHORT len);

/**************************************************
 *    APDU Functions to handle GENERIC IPPV SUPPORT
 *************************************************/
int  APDU_ProgramReq (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_ProgramCnf (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_PurchaseReq (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_PurchaseCnf (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_CancelReq (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_CancelCnf (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_HistoryReq (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_HistoryCnf (USHORT sesnum, UCHAR * apkt, USHORT len);

/**************************************************
 *    APDU Functions to handle MMI
 *************************************************/
int  APDU_OpenMMIReq (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_OpenMMICnf (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_CloseMMIReq (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_CloseMMICnf (USHORT sesnum, UCHAR * apkt, USHORT len);

/**************************************************
 *    APDU Functions to handle HOMING
 *************************************************/
int  APDU_OpenHoming (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_HomingCancelled (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_OpenHomingReply (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_HomingActive (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_HomingComplete (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_FirmUpgrade (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_FirmUpgradeReply (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_FirmUpgradeComplete (USHORT sesnum, UCHAR * apkt, USHORT len);
int  SendAPDUProfileInq(  );
/**************************************************
 *    APDU Functions to handle SYSTEM CONTROL
 *************************************************/
int  APDU_HostInfoReq (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_HostInfoResponse (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_CodeVerTable (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_CodeVerTableReply (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_HostDownloadCtl (USHORT sesnum, int apkt, USHORT len);
int  APDU_HostDownloadCmd (USHORT sesnum, UCHAR * apkt, USHORT len);

/**************************************************
 *    APDU Functions to handle COPY PROTECTION
 *************************************************/
int  APDU_CPOpenReq (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_CPOPenCnf (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_CPDataReq (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_CPDataCnf (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_CPSyncReq (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_CPSyncCnf (USHORT sesnum, UCHAR * apkt, USHORT len);

/**************************************************
 *    APDU Functions to handle LOW SPEED COMMS
 *************************************************/
int  APDU_CommsCmd (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_ConnDescriptor (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_CommsReply (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_CommsSendLast (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_CommsSendMore (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_CommsRcvLast (USHORT sesnum, UCHAR * apkt, USHORT len);
int  APDU_CommsRcvMore (USHORT sesnum, UCHAR * apkt, USHORT len);

#ifdef __cplusplus
}
#endif

#endif

