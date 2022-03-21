/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2018 RDK Management
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

/** \file pfcevents.h  Houses the implementation of the events classes. An event class is used part of the process to
   generate events. It is a good idea to implement a corresponding event handler class. */

#ifndef PFCEVENTS_H
#define PFCEVENTS_H

#include "diagMsgEvent.h"


typedef enum 
{
    	system_power_on = 4025,
    	system_power_standby = 4026,

}CMSystemEventType;

/** An implementation of the MPEG event manager.
 *
 * MPEG events will be handled here: e.g. PSI table update, PSIP table update,
 * A90 table update, table retrieving timeout, etc.
 */
typedef enum
{
	CM_Mpeg_PMT_updated = 5001,				//!< PMT updated.
	CM_Mpeg_section_found = 5020,
	CM_Mpeg_PMT_WITH_CA = 5027,               //!< PMT updated.
} CMMpegEventType;


typedef struct
{
	unsigned short prgNum;// Program Number
	unsigned char LTSID; // tunner ltsid
	unsigned char CCI; //cci information of the program
	unsigned char EMI;// EMI part of cci
	unsigned char APS;// APS part of cci
	unsigned char CIT;// CIT part of cci
	unsigned char RCT;//
	unsigned char CCIAuthStatus;// if this flag is set to 1 then CCI Auth success else if set to 0 then failure
}CardManagerCCIData_t;

typedef struct
{
	unsigned short prgNum;// Program Number
	unsigned char LTSID; // tunner ltsid
	unsigned char CaEncrypFlag;// Ca encryption flag , if this is set to '1' then the program is CA encrypted is set '0' then no CA encryption
}CardManagerCAEncryptionData_t;
typedef struct
{
	unsigned short usElmPid;//Elementary stream Pid...
	unsigned char  ucElm_ca_enable_flag;// if this flag is set to 1 then data in ucElm_ca_enable at Elm stream level is valid
	// This data is defined in CCIF 2.0 spec ca_enable (7 bits)
	unsigned char  ucElm_ca_enable;// this data is valid only when the ucElm_ca_enable_flag is set to '1'
}CardManagerCAPmtElmStreams_t;
typedef struct
{
	unsigned char  ucLtsid;
	unsigned short usProgramNumber;
	unsigned short usSourceId;
	unsigned char ucProgram_ca_enable_flag;// if this flag is set to 1 then data in ucProgram_ca_enable at Program level is valid
	// This data is defined in CCIF 2.0 spec ca_enable (7 bits)
	unsigned char ucProgram_ca_enable; // this data is valid only when the ucProgram_ca_enable_flag is set to '1'
	unsigned long ulNumOfElmStreams;// Number of elm streams
	CardManagerCAPmtElmStreams_t *p_stElmStreams;// free this allocated memory after coping it
}CardManagerCAPmtReply_t;

typedef enum
{
	CardMgr_Card_Detected = 0x1770,	
	CardMgr_Card_Removed,
	CardMgr_Card_Ready,	
	CardMgr_CP_CCI_Changed, // Card Manager received the CCI it posts the CCI_data with ltsid
	CardMgr_CP_CCI_Error, // Card Manager  CP Authentication Error posts  with  ltsid	
	CardMgr_DSG_Cable_Card_VCT_ID,
	CardMgr_Generic_Feature_Changed,
	CardMgr_DSG_UCID,
	CardMgr_Device_In_Non_DSG_Mode,
	CardMgr_CA_Encrypt_Flag, // Card Manager Posts this event for the selected program with CardManagerCAEncryptionData_t
	CardMgr_CA_Pmt_Reply,// Card Manager Posts this event for the selected program with CardManagerCAPmtReply_t
	CardMgr_DSG_IP_ACQUIRED,
	CardMgr_CP_DHKey_Changed,
	CardMgr_CA_Update,							
	CardMgr_DSG_Entering_DSG_Mode,
	CardMgr_DSG_Leaving_DSG_Mode,
	CardMgr_DOCSIS_TLV_217,
	CardMgr_DSG_DownStream_Scan_Failed,
	CardMgr_POD_IP_ACQUIRED,
	CardMgr_Card_Mib_Access_Root_OID,
	CardMgr_Card_Mib_Access_Snmp_Reply,
	CardMgr_Card_Mib_Access_Shutdown,
	CardMgr_OOB_Downstream_Acquired,
	CardMgr_DSG_Operational_Mode,
	CardMgr_DSG_Downstream_Acquired,
	CardMgr_VCT_Channel_Map_Acquired,
	CardMgr_CP_Res_Opened, //Copy Protection Resource opened
	CardMgr_Host_Auth_Sent,//Host AuthKey sent
	CardMgr_Bnd_Fail_Card_Reasons,//Binding Failure: Card reasons
	CardMgr_Bnd_Fail_Invalid_Host_Cert,//Binding Failure: Invalid Host Cert
	CardMgr_Bnd_Fail_Invalid_Host_Sig, //Binding Failure: Invalid Host Signature
	CardMgr_Bnd_Fail_Invalid_Host_AuthKey, //Binding Failure: Invalid Host AuthKey
	CardMgr_Bnd_Fail_Other, //Binding Failure: Other
	CardMgr_Card_Val_Error_Val_Revoked, //Card Validation Error: Validation revoked
	CardMgr_Bnd_Fail_Incompatible_Module, //Binding Failure. Incompatible module
	CardMgr_Bnd_Comp_Card_Host_Validated, //Binding Complete: Card/Host Validated
	CardMgr_CP_Initiated, //Copy Protection initiated
	CardMgr_Card_Image_DL_Complete, //Card Image Download Complete
	CardMgr_CDL_Host_Img_DL_Cmplete, //Host Image Download Complete
	CardMgr_CDL_CVT_Error, //Common Download CVT Error
	CardMgr_CDL_Host_Img_DL_Error, //Host Image Download Error, <P1>
	CardMgr_IpDm_Cable_Card_VCT_ID,
	CardMgr_IpDirectUnicast_Cable_Card_VCT_ID,
	CardMgr_GenericFeature_Cable_Card_VCT_ID,
} CardMgrEventType;


/** An implementation of the Common Download event manager.
 *
 * Common download events will be handled here: e.g. CVT table triggers and their source, Commondownload internal events, etc.
 */

typedef struct _tagDownLEngine_TuneEventData
{
    int frequency;
    int modulationType;
    int programNumber;
    int carouselpid;
}
DownLEngine_TuneEventData;

typedef struct _tagDownLEngine_DownLoadProgress
{
    int Num_Total;
    int Num_Acquired;
}
DownLEngine_DownLoadProgress;


typedef enum
{
	// Triggers events .... handled by the Commondownload Manager
	CommonDownL_CVT_OOB =7000,	// CVT trigger from OOB FDC
	CommonDownL_CVT_DSG,		// CVT trigger from DSG
	CommonDownL_CVC_ConfigFile,	// CommonDownload trigger from configfile(eCM)
	CommonDownL_CVC_SNMP,		// CommonDownload trigger from SNMP(eCM)
	CommonDownL_CVT_SNMP,		// CommonDownload CVT ..trigger from SNMP(eCM) as per spec
	CommonDownL_Sys_Ctr_Ses_Open, // Sys Control session is opened
	CommonDownL_Sys_Ctr_Ses_Close, // Sys Control session is opened
	//CVT validation events
	//This event is posted from common download Manager to the CVT validator
	CommonDownL_CVT_Validation =0x10,// CommonDownload CVT Validation event.
	//This event is posted from CVT validator to the commondownload Manager
	CommonDownL_CVT_Validated , // CVT validated
	CommonDownL_Cert_Check , // Snmp Certificate check
	CommonDownL_Test_case,//For testing purpose

	//Download Engine events
	CommonDownL_Engine_start = 0x20,// CommonDownload Engine start the image download
	CommonDownL_Engine_started = 0x21,// CommonDownload Engine start the image download
	CommonDownL_Engine_complete =0x22, //CommonDownload Engine completed the the image download.
	CommonDownL_Engine_Error = 0x23,

	/*
	Following eventtypes belongs to PFC_EVENT_CATEGORY_DOWNL_ENGINE_INBAND_TUNE Category
	These events will be posted by the Download engine and will be transferred to the Java layer for processing
	*/
	DownLEngine_TUNE_START    = 0x25,
	DownLEngine_DOWNLOAD_COMPLETE,//0x26
	DownLEngine_DOWNLOAD_FAILED,//0x27
	DownLEngine_DOWNLOAD_ABORTED,//0x28
	DownLEngine_DOWNLOAD_PROGRESS, //0x29

	//Image update events
	CommonDownL_Image_Update_start = 0x30, //CommonDownload Manger posts this event to update the new Image..

	CommonDownL_Deferred_DL_start  = 0x40,// This event is generated by the trigger of Monitor App.
	CommonDownL_Deferred_DL_notify = 0x41,// This event is notified tothe Monitor Applicaton for the deferred download

	CommonDownL_Engine_complete_t2v2 = 0x1000,// This event will be posted for version 2 type 2 CVT. when download is complete

} CommonDownLEventType;

typedef enum
{
	TableManager_AIT_present = 10000,        // AIT present in tuned freq. program_num will indicate which program.
	TableManager_AIT_notPresent,    // AIT not present in freq
	TableManager_AIT_available,    // AIT is available to read
	TableManager_PAT_available,    // PAT is available to read
	TableManager_PMT_available,
	TableManager_MGT_available,
	TableManager_TVCT_available,
	TableManager_CVCT_available,
	TableManager_SVCT_available,
	TableManager_LVCT_available,
	TableManager_NIT_available,
	TableManager_EIT_available,
	TableManager_AEIT_available,
	TableManager_ETT_available,
	TableManager_AETT_available,
	TableManager_RRT_available,
	TableManager_STT_available,
	TableManager_EAS_available,
	TableManager_all_PMT_collected,
	TableManager_section_data_availble, // section data is collected to read
	TableManager_Database_Updated,
}TableManagerEventType;

#endif // __PFCEVENTS_H__
