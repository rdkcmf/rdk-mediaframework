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

/**
 * @file snmpIntrfGetAvOutData.cpp
 *
 * @brief Aim of this source file is to retrieve the information about audio/video, dvi/hdmi, 
 * number of tuners, interface, port type, spdif, mpeg2, analog video etc and fill these data to 
 * corresponding content table.
 *
 */

#include <stdio.h>
//#include "cMisc.h"
#include "core_events.h"
#include "snmpIntrfGetAvOutData.h"
#include "vlHalSnmpTunerInterface.h"
#include "vlHalSnmpAvOutInterface.h"
#include "vlHalSnmpHdmiInterface.h"
#include "vlHalSnmpUtils.h"
//#include "vlOcapDebug.h"
//#include "vlCCIInfo.h"
#include "string.h"
#include "ipcclient_impl.h" 
#include "vlEnv.h"

#ifdef USE_1394
#include "FirebusAppExportsForOcap.h" 
#include "cNMDefines.h" // for 1394 port status Avlist
#include "vlpluginapp_hal1394api.h"
#endif //USE_1394
extern "C" unsigned char vl_isFeatureEnabled(unsigned char *featureString);

#define vlStrCpy(pDest, pSrc, nDestCapacity)            \
            strcpy(pDest, pSrc)

#define vlMemCpy(pDest, pSrc, nCount, nDestCapacity)            \
            memcpy(pDest, pSrc, nCount)

#define VL_ZERO_MEMORY(x) \
            memset(&x,0,sizeof(x))  

extern SNMPocStbHostAVInterfaceTable_t vlg_AVInterfaceTable[SNMP_MAX_ROWS];

//extern int GetCCIInfoElement(CardManagerCCIData_t* pCCIData,unsigned long LTSID,unsigned long programNumber);

/**
 * @brief This function is used to get the number of available tuners by using hal snmp api call.
 * In case of failure, it takes the count of tuners from the config file "MPE.SYS.NUM.TUNERS"
 *
 * @param[out] pNumOfTuners Total number of tuners.
 *
 * @return Upon successful completion, this function returns total number of available tuners.
 */
int SnmpGetTunerCount(unsigned int * pNumOfTuners)
{
    // PXG2V2-1327: It is illegal for this function to return a value less than or equal to zero. Hence the over-zealous exception handling.
    unsigned int retStatus = 0;
    if(NULL == pNumOfTuners) return vl_env_get_int(1, "MPE.SYS.NUM.TUNERS"); // Can't return zero here as that would make it illegal.
    
    retStatus = HAL_SNMP_Get_Tuner_Count(pNumOfTuners);
    if(retStatus <= 0)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP", "%s: HAL_SNMP_Get_Tuner_Count() call failed with error = %u.\n", __FUNCTION__, retStatus);
        
    }
    if((int)(*pNumOfTuners) > 0)
    {
        // PXG2V2-1327: return value only if count of tuners is greater than zero.
        return *pNumOfTuners;
    }
    // PXG2V2-1327 : return value from config file in cases where HAL / IPC API fails due to race conditions, etc.
    *pNumOfTuners = vl_env_get_int(1, "MPE.SYS.NUM.TUNERS");
    return *pNumOfTuners;
}

/**
 * @brief Constructor for snmpAvOutInterface class.
 * Currently empty default constructor.
 */
snmpAvOutInterface::snmpAvOutInterface()
{
    
}

/**
 * @brief Destructor for snmpAvOutInterface class.
 * Currently empty default destructor.
 */
snmpAvOutInterface::~snmpAvOutInterface()
{
    
}


#define X1_NUM_TUNERS 6 

/**
 * @brief This function finds the number of tuner and updates AV Interface Table for each tuner.
 * Further it gets Display count and AV out interface info for each display.
 * Then this function fills the interface Table with these details.
 *
 * @param[out] ptAVInterfaceTable Interface Table.
 * @param[out] ptSizeOfResList Number of tuners.
 *
 * @return None
 */
void snmpAvOutInterface::Snmp_getallAVdevicesindex(SNMPocStbHostAVInterfaceTable_t* ptAVInterfaceTable, int* ptSizeOfResList)
{
    if(ptAVInterfaceTable == NULL)
        return;
    
    unsigned int maxNumOfDiplays = 0;
    unsigned int retStatus;
     unsigned int numOfTuners = 0;
    unsigned int tunerIter = 0;

///** TUNER  **/
     retStatus = SnmpGetTunerCount(&numOfTuners);
     //retStatus = X1_NUM_TUNERS;

    //numOfTuners is now zero. X1_NUM_TUNERS should be assigned to numTuners instead of retStatus
    
    for(tunerIter=0; tunerIter<numOfTuners; tunerIter++)
    {
        ptAVInterfaceTable[tunerIter].vl_ocStbHostAVInterfaceType  = ocStbHostScte40FatRx;

             vlMemCpy(ptAVInterfaceTable[tunerIter].vl_ocStbHostAVInterfaceDesc,"Digital Capable Tuner",(strlen("Digital Capable Tuner")+1),API_CHRMAX);

        ptAVInterfaceTable[tunerIter].vl_ocStbHostAVInterfaceStatus = OCSTBHOSTAVINTERFACESTATUS_ENABLED;

        ptAVInterfaceTable[tunerIter].resourceIdInfo.tunerIndex = tunerIter;
    }

    *ptSizeOfResList = numOfTuners;

///** AvOut **/

    maxNumOfDiplays = get_display_handle_count();
    HAL_SNMP_AVOutInterfaceInfo objAVOutInterfaceInfo[maxNumOfDiplays];
    get_avOutinterface_info((void*)objAVOutInterfaceInfo, maxNumOfDiplays);

    fillUp_AvOutInterface_Table(ptAVInterfaceTable, (void*)objAVOutInterfaceInfo, ptSizeOfResList, maxNumOfDiplays);

    
}

/**
 * @brief This function is used to get DVI hdmi info such as output type (dvi/hdmi), connection status, 
 * repeater status, video resolution, format and then it updates to DVI HDMI Table.
 *
 * @param[in] vlghandle Handle for SNMP Interface.
 * @param[in] avintercacetype AV Interface Type e.g. COMPOSITE, SVIDEO, HDMI etc.
 * @param[out] ptDviHdmiTable Pointer to DVI HDMI Table.
 *
 * @return None
 */
void snmpAvOutInterface::snmp_get_DviHdmi_info(unsigned long vlghandle, AV_INTERFACE_TYPE avintercacetype, SNMPocStbHostDVIHDMITable_t* ptDviHdmiTable)
{
    if(ptDviHdmiTable == NULL)
        return;

    unsigned int retStatus;
    SNMP_INTF_DVIHDMI_Info dviHdmiInfo;
    retStatus = HAL_SNMP_Get_DviHdmi_info(&dviHdmiInfo);

    fillUp_DviHdmiTable(ptDviHdmiTable, (void*)&dviHdmiInfo, retStatus);

}

/** 
 * @brief This function is used to get the data content in the DVI HDMI Video Format Table (from hal display module).
 * 
 * @param[in] pnCount Data count.
 * @param[out] vl_DVIHDMIVideoFormatTable DVI HDMI Video Table containing video information.
 *
 * @return None
 */
void snmpAvOutInterface::snmp_get_ocStbHostDVIHDMIVideoFormatTableData(SNMPocStbHostDVIHDMIVideoFormatTable_t* vl_DVIHDMIVideoFormatTable, int *pnCount)
{
    if(vl_DVIHDMIVideoFormatTable == NULL)
        return;

    unsigned int retStatus;
    retStatus = HAL_SNMP_ocStbHostDVIHDMIVideoFormatTableData(vl_DVIHDMIVideoFormatTable, pnCount);
}

/** 
 * @brief This function is used to get the Component video information like Aspect ratio, format of video and mute status.
 *
 * @param[in] vlghandle Handle for SNMP Interface.
 * @param[in] avintercacetype AV Interface Type e.g. Composite or component.
 * @param[out] ptComponentVideoTable Component video table with current video information.
 *
 * @return None
 */
void snmpAvOutInterface::snmp_get_ComponentVideo_info(unsigned long vlghandle, AV_INTERFACE_TYPE avintercacetype, SNMPocStbHostComponentVideoTable_t* ptComponentVideoTable)
{
    if(ptComponentVideoTable == NULL)
        return;

    unsigned int retStatus;
    HAL_SNMP_ComponentVideoTable_t componentInfo;
    retStatus = HAL_SNMP_Get_ComponentVideo_info((void*)&componentInfo);

    if(componentInfo.ComponentVideoConstrainedStatus)
    {
        ptComponentVideoTable->vl_ocStbHostComponentVideoConstrainedStatus = STATUS_TRUE;
    }
    else
    {
           ptComponentVideoTable->vl_ocStbHostComponentVideoConstrainedStatus = STATUS_FALSE;
    }

    // Jun-24-2011: Bug fix : Removed switch case statements
    ptComponentVideoTable->vl_ocStbHostComponentOutputFormat = (ComponentOutputFormat_t)(componentInfo.ComponentOutputFormat);
    ptComponentVideoTable->vl_ocStbHostComponentAspectRatio  = (ComponentAspectRatio_t )(componentInfo.ComponentAspectRatio );
    // Jun-24-2011: Bug fix : Removed switch case statements

    if(componentInfo.ComponentVideoMuteStatus)
    {
            ptComponentVideoTable->vl_ocStbHostComponentVideoMuteStatus = STATUS_TRUE;
    }
    else
    {
            ptComponentVideoTable->vl_ocStbHostComponentVideoMuteStatus = STATUS_FALSE;
    }


    return;
}

/** 
 * @brief This function is used to get the mpeg content info data.
 *
 * If CCI event is found, it gets the CCI Data information for the current LTSID and Program Number,
 * e.g. program Number, total number of Streams, Audio/Video pid.
 * In case of no CCI event, content table is saved with the default values.
 *
 * @param[in] pysicalID Content Table ID
 * @param[in] Mpegactivepgnumber MPEG Active Program Number
 * @param[out] ptMpeg2ContentTable MPEG2 content table
 *
 * @return None
 */
void snmpAvOutInterface::snmp_get_Mpeg2Content_info(SNMPocStbHostMpeg2ContentTable_t* ptMpeg2ContentTable, unsigned int pysicalID, int Mpegactivepgnumber)
{
    if(ptMpeg2ContentTable == NULL)
        return;

    unsigned int retStatus;
    CardManagerCCIData_t sCCIData;
    memset(&sCCIData, 0, sizeof(CardManagerCCIData_t));
    HAL_SNMP_Mpeg2ContentTable_t mpeg2ContentInfo;
    retStatus = HAL_SNMP_Get_Mpeg2Content_info((void*)&mpeg2ContentInfo);

     // Get the CCIData information if received from card for the current LTSID and Program Number
        if(IPC_CLIENT_GetCCIInfoElement(&sCCIData,mpeg2ContentInfo.LTSID,mpeg2ContentInfo.programNumber) == 0)
        {
            mpeg2ContentInfo.cciValue = (HAL_SNMP_Mpeg2ContentCCIValue_t)(sCCIData.EMI);
            mpeg2ContentInfo.apsValue = (HAL_SNMP_Mpeg2ContentAPSValue_t)(sCCIData.APS);
            mpeg2ContentInfo.citStatus = (sCCIData.CIT) ? true : false;
            mpeg2ContentInfo.broadcastFlagStatus = (sCCIData.RCT) ? true : false;
            mpeg2ContentInfo.epnStatus = (sCCIData.RCT) ? true : false;
        }
        else //If no CCI event put default values
        {
            mpeg2ContentInfo.cciValue            = mpeg2ContentInfo.cciValue;
            mpeg2ContentInfo.apsValue            = mpeg2ContentInfo.apsValue;
            mpeg2ContentInfo.citStatus           = 0;
            mpeg2ContentInfo.broadcastFlagStatus = 0;
            mpeg2ContentInfo.epnStatus           = 0;
        }

    
    fillUp_Mpeg2ContentTable(ptMpeg2ContentTable, (void*)&mpeg2ContentInfo, retStatus);
    
    return;

}

/**
 * @brief This function is used to get the RF Channel info like Program number, Spectral bandwidth, Symbol rate etc.
 *
 * @param[in] vlghandle Handle for SNMP Interface.
 * @param[in] avintercacetype AV Interface Type e.g. COMPOSITE, SVIDEO, HDMI etc.
 * @param[in] vl_RFChannelOutTable Table of information about RF Channels.
 *
 * @return None
 */
void snmpAvOutInterface::snmp_get_RFChan_info(unsigned long vlghandle, AV_INTERFACE_TYPE avintercacetype, SNMPocStbHostRFChannelOutTable_t* vl_RFChannelOutTable)
{
    if(vl_RFChannelOutTable == NULL)
        return;

    unsigned int retStatus;
    retStatus = HAL_SNMP_Get_RFChan_info();

    return;
}

/**
 * @brief This function is used to get the Analog video info such as aspect ratio, resolution.
 *
 * @param[in] vlghandle Handle for SNMP Interface.
 * @param[in] avintercacetype AV Interface Type e.g. COMPOSITE, SVIDEO etc.
 * @param[in] vl_AnalogVideoTable Table of informations about Analog Video channel.
 *
 * @return None
 */
void snmpAvOutInterface::snmp_get_AnalogVideo_info(unsigned long vlghandle, AV_INTERFACE_TYPE avintercacetype, SNMPocStbHostAnalogVideoTable_t* vl_AnalogVideoTable)
{
    if(vl_AnalogVideoTable == NULL)
        return;

    unsigned int retStatus;
    retStatus = HAL_SNMP_Get_AnalogVideo_info();

    return;
}

/**
 * @brief This function is used to get the SPDIF Audio info like sample rate, Dolby/DTS support etc.
 *
 * @param[in] vlghandle Handle for SNMP Interface.
 * @param[in] avintercacetype AV Interface Type e.g. COMPOSITE, SVIDEO, HDMI etc.
 * @param[in] vl_SPDIfTable SPDIF Audio channel info table.
 *
 * @return None
 */
void snmpAvOutInterface::snmp_get_Spdif_info(unsigned long vlghandle, AV_INTERFACE_TYPE avinterfacetype, SNMPocStbHostSPDIfTable_t* vl_SPDIfTable)
{
    if(vl_SPDIfTable == NULL)
        return;

    if(AV_INTERFACE_TYPE_SPDIF != avinterfacetype)
        return;
    
    unsigned int retStatus;
    HAL_SNMP_SpdifTable_t spdifInfo;
    
    
    retStatus = HAL_SNMP_Get_Spdif_info((void*)&spdifInfo);
    
    fillUp_SpdifTable(vl_SPDIfTable, (void*)&spdifInfo, retStatus);

    return;
}

/**
 * @brief Currently Not Implemented
 *
 */
void snmpAvOutInterface::SnmpGet1394Table(/*&pNum1394Interfaces, &interfaceInfo*/)
{
}

/**
 * @brief Currently Not Implemented
 *
 */
void snmpAvOutInterface::SnmpGet1394AllDevicesTable(/*&pNumDevices, connDevInfo*/)
{
}

/**
 * @brief Currently Not Implemented
 *
 */
void snmpAvOutInterface::SnmpIs1394PortStreaming(/*PORT_ONE_INFO, &PIStreaming*/)
{
}

/**
 * @brief Currently Not Implemented
 *
 */
void snmpAvOutInterface::GetDecoderSNMPInfo(/*&decoderSNMPInfo_status*/)
{

}

/**
 * @brief Currently Not Implemented
 *
 */
void snmpAvOutInterface::getMpeg2ContentTable(/*pysicalID, Mpegactivepgnumber*/)
{
}

/**
 * @brief Currently Not Implemented
 *
 */
void snmpAvOutInterface::SNMPGetApplicationInfo(/*appInfo*/)
{
}

/**
 * @brief Currently Not Implemented
 *
 */
void snmpAvOutInterface::SNMPGet_ocStbHostHostID(/*HostID, 255*/)
{
}

/**
 * @brief Currently Not Implemented
 *
 */
void snmpAvOutInterface::SNMPGet_ocStbHostCapabilities(/*hostcap*/)
{
}

/**
 * @brief Currently Not Implemented
 *
 */
void snmpAvOutInterface::SNMPget_ocStbHostSecurityIdentifier(/*&HostCaTypeInfo*/)
{
}

/**
 * @brief Currently Not Implemented
 *
 */
void snmpAvOutInterface::CommonDownloadMgrSnmpHostFirmwareStatus(/*&objfirmwarestatus*/)
{
}

/**
 * @brief Currently Not Implemented
 *
 */
void snmpAvOutInterface::SNMPGetCableCardCertInfo(/*&Mcardcertinfo*/)
{
}

/**
 * @brief Currently Not Implemented
 *
 */
void snmpAvOutInterface::get_display_device()
{
}

/**
 * @brief This function is used to get the number of display handles using hal snmp api.
 *
 * @return displayHandleCount Returns total count for display handles.
 */
unsigned int snmpAvOutInterface::get_display_handle_count()
{
    unsigned int result = 0;
    unsigned int displayHandleCount = 0;
    result = HAL_SNMP_Get_Display_Handle_Count(&displayHandleCount);
    return displayHandleCount;
}

#if 0
unsigned long* snmpAvOutInterface::get_display_handles(/*numDisplays, pDisplayHandles, 0*/)
{
    unsigned int result = 0;
    unsigned int maxNumOfDiplays = 0;
    maxNumOfDiplays = get_display_handle_count();
    unsigned long dispHandles[maxNumOfDiplays];
    result = HAL_SNMP_Get_Display_Handles(dispHandles, maxNumOfDiplays);

    return dispHandles;
}
#endif

/** 
 * @brief This function is used to get information about AV output ports for a given number of connected displays.
 * It also provides details such as port status, port type (composite,HDMI etc.).
 *
 * @param[in] maxNumOfDiplays Number of displays connected to AV outport.
 * @param[out] ptAVOutInterfaceInfo Address where AV out information is stored.
 *
 * @return None
 */
void snmpAvOutInterface::get_avOutinterface_info(void* ptAVOutInterfaceInfo, unsigned int maxNumOfDiplays)
{
    if(ptAVOutInterfaceInfo == NULL)
        return;

    unsigned int result = 0;
    unsigned int dispIter;
    unsigned int portIter;
        
    result = HAL_SNMP_Get_AvOut_Info(ptAVOutInterfaceInfo, maxNumOfDiplays);

    return;

}

/**
 * @brief This function is used to get the video information like Aspect ratio, format of video and mute status.
 *  It also finds the tuner count, add/remove the connection details from the list, 
 *  updates program info like audio/video pid, program status, active program number etc.
 *
 * @param[out] vlg_programInfo Global struct contains Component video table info 
 * about physicalSrcId, videopid, audiopid etc.
 * @param[out] numberpg Number of Program channels.
 * @param[out] vlg_Signal_Status Signal Status of each programs
 *
 * @retval On Success, it returns 1, else it returns 0.
 */
int snmpAvOutInterface::snmp_get_ProgramStatus_info(programInfo *vlg_programInfo, int *numberpg, int *vlg_Signal_Status)
{
    unsigned int retStatus = 0;
    unsigned int devListSize = 0;
    unsigned int listIter;
    ConnDetails connDetails;

    if(vlg_programInfo == NULL)
        return 0;

    HAL_SNMP_Get_DevListSize(&devListSize);
// if already present remove and then add to connlist [revisit]
#if 1
	//TunerData tuneParams;
	unsigned int numOfTuners = 0;
	unsigned int tunerIter = 0;
	unsigned int programnumber = 1;
	unsigned int tunerLTSId = 0;
	unsigned int srcId = 0;
	
	retStatus = SnmpGetTunerCount(&numOfTuners);
	SNMP_DEBUGPRINT("%s : %d devlistsize = %d, tunercount=%d\n", __FUNCTION__, __LINE__, devListSize, numOfTuners);
	if(devListSize == numOfTuners)
	{
		//Remove
		for(tunerIter=0; tunerIter<numOfTuners; tunerIter++)
		{
			RemFromConnList(tunerIter, devTuner);
		}
	}
	else
	{
		//ADD
		for(tunerIter=0; tunerIter<numOfTuners; tunerIter++)
		{
			memset(&connDetails, 0, sizeof(ConnDetails));
#if 0
    		if (HAL_SNMP_Get_Tuner_Info(tunerIter, &programnumber, &tunerLTSId, &srcId) != RMF_SUCCESS)
			{
			    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP",
			             "%s: getTunerInfo call failed.\n",
			             __FUNCTION__);
				connDetails.programNum = programnumber;
				connDetails.tunerLTSID = tunerLTSId;
			}
			else
			{
				connDetails.programNum = programnumber;
				connDetails.tunerLTSID = tunerLTSId;
			}
#endif
			connDetails.programNum = programnumber;
			connDetails.tunerLTSID = tunerLTSId;
			connDetails.isDecode = TRUE;
			connDetails.devIndex = tunerIter; //tunerId
			connDetails.decoderIndex = 0;
			connDetails.devType = devTuner;
			connDetails.vidDecStatus = TRUE;
			connDetails.audDecStatus = TRUE;
			connDetails.videoPid = 0;
			connDetails.audioPid = 0;
			connDetails.pcrPid = 0;
			connDetails.progStatus = TRUE;
			connDetails.CCIData = 0;
			connDetails.pcrlockStatus = 0;
			connDetails.dcontinuities = 0;
			connDetails.pts_errors = 0;
			connDetails.ContentPktErrors = 0;
			connDetails.ContentPipelineErrors = 0;
	
			AddToConnList(&connDetails);
		}
    	HAL_SNMP_Get_DevListSize(&devListSize);
	}
#endif
	SNMP_DEBUGPRINT("%s : %d devlistsize = %d, tunercount=%d\n", __FUNCTION__, __LINE__, devListSize, numOfTuners);
    
    if(devListSize != 0)
    {
        
        for(listIter=0; listIter<devListSize; listIter++)
        {    
            HAL_SNMP_Get_ProgramStatus_info((void*)&connDetails, listIter);
    
            vlg_programInfo[listIter].physicalSrcId = connDetails.devIndex;
            vlg_programInfo[listIter].pcrPid =  connDetails.pcrPid;
            vlg_programInfo[listIter].videoPid = connDetails.videoPid;
            vlg_programInfo[listIter].audioPid = connDetails.audioPid;
            vlg_programInfo[listIter].pgstatus = connDetails.progStatus;
            vlg_programInfo[listIter].activepgnumber = connDetails.programNum;
    
            if(connDetails.devType == devTuner)        
                vlg_programInfo[listIter].mediaType = SnmpMediaEngineAVPlayBack; //SnmpMediaEngineTuner;
            else if(connDetails.devType == devDISK)
                vlg_programInfo[listIter].mediaType = SnmpMediaEngineDVR;
    
            vlg_programInfo[listIter].snmpVDecoder = connDetails.vidDecStatus;
            vlg_programInfo[listIter].snmpADecoder = connDetails.audDecStatus;
    
    
        }
        *numberpg = listIter;
        
        
        *vlg_Signal_Status = 2;
    }
    else ///default values 
    {
        vlg_programInfo[0].physicalSrcId = 0;
        vlg_programInfo[0].pcrPid =  1;
        vlg_programInfo[0].videoPid = 1;
        vlg_programInfo[0].audioPid = 1;
        vlg_programInfo[0].pgstatus = 1;
        vlg_programInfo[0].mediaType = SnmpMediaEngineAVPlayBack;
        vlg_programInfo[0].snmpVDecoder = 1;
        vlg_programInfo[0].snmpADecoder = 1;
        vlg_programInfo[0].activepgnumber = 1;
        *numberpg = 1;
        *vlg_Signal_Status = 2;
    }
	return 1;
}

/**
 * @brief Currently Not Implemented
 *
 */
void snmpAvOutInterface::SNMPGetCardIdBndStsOobMode(/*&oobInfo*/)
{
}

/**
 * @brief Currently Not Implemented
 *
 */
void snmpAvOutInterface::SNMPGetGenFetResourceId(/*&nLong*/)
{
}

/**
 * @brief Currently Not Implemented
 *
 */
void snmpAvOutInterface::SNMPGetGenFetTimeZoneOffset(/*&(vl_ProxyCardInfo->ocStbHostCardTimeZoneOffset)*/)
{
}

/**
 * @brief Currently Not Implemented
 *
 */
void snmpAvOutInterface::SNMPGetGenFetDayLightSavingsTimeDelta(/*vl_ProxyCardInfo->ocStbHostCardDaylightSavingsTimeDelta*/)
{
}

/**
 * @brief Currently Not Implemented
 *
 */
void snmpAvOutInterface::SNMPGetGenFetDayLightSavingsTimeEntry(/*(unsigned long*)&(vl_ProxyCardInfo->ocStbHostCardDaylightSavingsTimeEntry)*/)
{
}

/**
 * @brief Currently Not Implemented
 *
 */
void snmpAvOutInterface::SNMPGetGenFetDayLightSavingsTimeExit(/*(unsigned long*)&(vl_ProxyCardInfo->ocStbHostCardDaylightSavingsTimeExit)*/)
{
}

/**
 * @brief Currently Not Implemented
 *
 */
void snmpAvOutInterface::SNMPGetGenFetEasLocationCode(/*&nChar, &nShort*/)
{
}

/**
 * @brief Currently Not Implemented
 *
 */
void snmpAvOutInterface::SNMPGetGenFetVctId(/*&nShort*/)
{
}

/**
 * @brief Currently Not Implemented
 *
 */
void snmpAvOutInterface::SNMPGetCpAuthKeyStatus(/*(int*)&(vl_ProxyCardInfo->ocStbHostCardCpAuthKeyStatus)*/)
{
}

/**
 * @brief Currently Not Implemented
 *
 */
void snmpAvOutInterface::SNMPGetCpCertCheck(/*(int*)&(vl_ProxyCardInfo->ocStbHostCardCpCertificateCheck)*/)
{
}

/**
 * @brief Currently Not Implemented
 *
 */
void snmpAvOutInterface::SNMPGetCpCciChallengeCount(/*(unsigned long*)&(vl_ProxyCardInfo->ocStbHostCardCpCciChallengeCount)*/)
{
}

/**
 * @brief Currently Not Implemented
 *
 */
void snmpAvOutInterface::SNMPGetCpKeyGenerationReqCount(/*(unsigned long*)&(vl_ProxyCardInfo->ocStbHostCardCpKeyGenerationReqCount)*/)
{
}

/**
 * @brief Currently Not Implemented
 *
 */
void snmpAvOutInterface::snmp_Sys_request(/*VL_SNMP_REQUEST_GET_SYS_HOST_MEMORY_INFO, &snmpHostMemoryInfo*/)
{
}

/**
 * @brief Currently Not Implemented
 *
 */
void snmpAvOutInterface::GetOOBCarouselTimeoutCount()
{
}

/**
 * @brief Currently Not Implemented
 *
 */
void snmpAvOutInterface::GetInbandCarouselTimeoutCount()
{
}

/**
 * @brief This function is used to get info from input pointer and fills the data such as 
 * port type, port status, interface type (e.g. AV Composite, HDMI, SVideo.) etc to the 
 * Audio Video Interface Table.
 *
 * @param[in] ptAvOutIntrfcInfo Struct Pointer where AV info is present.
 * @param[in] ptSizeOfResList Number of Tuners connected.
 * @param[in] maxNumOfDiplays Number of Displays.
 * @param[out] ptAVInterfaceTable Table to which AV interface info is retrieved.
 *
 * @return None
 */
void snmpAvOutInterface::fillUp_AvOutInterface_Table(SNMPocStbHostAVInterfaceTable_t* ptAVInterfaceTable, 
                    void* ptAvOutIntrfcInfo, 
                    int* ptSizeOfResList,
                    unsigned int maxNumOfDiplays)
{
    unsigned int dispIter;
    unsigned int numAvOutPorts = 0;
    unsigned int resIter = (*ptSizeOfResList);
    unsigned int portIter = 0;

    if((ptAVInterfaceTable == NULL) || (ptAvOutIntrfcInfo == NULL))
        return;

    HAL_SNMP_AVOutInterfaceInfo* ptAVOutInterfaceInfo; 
    ptAVOutInterfaceInfo = (HAL_SNMP_AVOutInterfaceInfo*)ptAvOutIntrfcInfo;
    
    for(dispIter = 0; dispIter < maxNumOfDiplays; dispIter++)
    {
        numAvOutPorts = ptAVOutInterfaceInfo->numAvOutPorts;
        *ptSizeOfResList = (*ptSizeOfResList) + numAvOutPorts;

        for(portIter = 0 ; portIter < ptAVOutInterfaceInfo->numAvOutPorts; portIter++)
        {
        
            ptAVInterfaceTable[resIter].resourceIdInfo.dispResData.hDisplay = ptAVOutInterfaceInfo->avOutPortInfo[portIter].deviceIndex;
        
            if(ptAVOutInterfaceInfo->avOutPortInfo[portIter].portStatus)
            {
                ptAVInterfaceTable[resIter].vl_ocStbHostAVInterfaceStatus =  OCSTBHOSTAVINTERFACESTATUS_ENABLED ;
            
            }
            else
            {
                ptAVInterfaceTable[resIter].vl_ocStbHostAVInterfaceStatus = OCSTBHOSTAVINTERFACESTATUS_DISABLED;
                
            }
        
            switch(ptAVOutInterfaceInfo->avOutPortInfo[portIter].avOutPortType)
            {
        
                case HAL_AV_PORT_COMPOSITE:
        
                    ptAVInterfaceTable[resIter].resourceIdInfo.dispResData.DisplayPortType = AV_INTERFACE_TYPE_COMPOSITE;

                    ptAVInterfaceTable[resIter].vl_ocStbHostAVInterfaceType  = ocStbHostBbVideoOut;
        
                    vlMemCpy(ptAVInterfaceTable[resIter].vl_ocStbHostAVInterfaceDesc,"Composite Video Out",(strlen("Composite Video Out")+1),API_CHRMAX);
                    break;
                case HAL_AV_PORT_SVIDEO:

                    ptAVInterfaceTable[resIter].resourceIdInfo.dispResData.DisplayPortType = AV_INTERFACE_TYPE_SVIDEO;

                    ptAVInterfaceTable[resIter].vl_ocStbHostAVInterfaceType  = ocStbHostSVideoOut;

                    vlMemCpy(ptAVInterfaceTable[resIter].vl_ocStbHostAVInterfaceDesc,"S-Video Basedband Out",(strlen("S-Video Basedband Out")+1),API_CHRMAX);
        
                    break;
                case HAL_AV_PORT_RF:

                    ptAVInterfaceTable[resIter].resourceIdInfo.dispResData.DisplayPortType = AV_INTERFACE_TYPE_RF;

                    ptAVInterfaceTable[resIter].vl_ocStbHostAVInterfaceType  = ocStbHostRFOutCh;

                    vlMemCpy(ptAVInterfaceTable[resIter].vl_ocStbHostAVInterfaceDesc,"NTSC(ch 3 or 4) Out",(strlen("NTSC(ch 3 or 4) Out")+1),API_CHRMAX);
        
                    break;
                case HAL_AV_PORT_COMPONENT:

                    ptAVInterfaceTable[resIter].resourceIdInfo.dispResData.DisplayPortType = AV_INTERFACE_TYPE_COMPONENT;

                    ptAVInterfaceTable[resIter].vl_ocStbHostAVInterfaceType  = ocStbHostHdComponentOut;

                    vlMemCpy(ptAVInterfaceTable[resIter].vl_ocStbHostAVInterfaceDesc,"HdComponentOut",(strlen("HdComponentOut")+1),API_CHRMAX);
        
                    break;
                case HAL_AV_PORT_HDMI:
            
                    ptAVInterfaceTable[resIter].resourceIdInfo.dispResData.DisplayPortType = AV_INTERFACE_TYPE_HDMI;

                    ptAVInterfaceTable[resIter].vl_ocStbHostAVInterfaceType  = ocStbHostHdmiOut;

                    vlMemCpy(ptAVInterfaceTable[resIter].vl_ocStbHostAVInterfaceDesc,"DVI/HDMIOUT",(strlen("DVI/HDMIOUT")+1),API_CHRMAX);
        
                    break;
                case HAL_AV_PORT_L_R:
    
                    ptAVInterfaceTable[resIter].resourceIdInfo.dispResData.DisplayPortType = AV_INTERFACE_TYPE_L_R;

                    ptAVInterfaceTable[resIter].vl_ocStbHostAVInterfaceType  = /*hrDeviceOther*/ocStbHostBbAudioOut;

                    vlMemCpy(ptAVInterfaceTable[resIter].vl_ocStbHostAVInterfaceDesc,"Composite Audio Out",(strlen("Composite Audio Out")+1),API_CHRMAX);
        
                    break;
                case HAL_AV_PORT_SPDIF:
                    
                    ptAVInterfaceTable[resIter].resourceIdInfo.dispResData.DisplayPortType = AV_INTERFACE_TYPE_SPDIF;

                    vlMemCpy(ptAVInterfaceTable[resIter].vl_ocStbHostAVInterfaceDesc,"Toslink SPDIF RCA audio output",(strlen("Toslink SPDIF RCA audio output")+1),API_CHRMAX);

                    ptAVInterfaceTable[resIter].vl_ocStbHostAVInterfaceType  = ocStbHostToslinkSpdifOut;

                    break;
        
                case HAL_AV_PORT_DVI:

                    vlMemCpy(ptAVInterfaceTable[resIter].vl_ocStbHostAVInterfaceDesc,"DVI-OUT",(strlen("DVI-OUT")+1),API_CHRMAX);

                    ptAVInterfaceTable[resIter].vl_ocStbHostAVInterfaceType  = ocStbHostDviOut;

                    break;
                default :
                    //RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP", "==> Error port type does not match or wrong port type %d\n", ptAVOutInterfaceInfo->avOutPortInfo[portIter].avOutPortType);
                    const char * pszOutputName = "RDK-Output";
                    vlMemCpy(ptAVInterfaceTable[resIter].vl_ocStbHostAVInterfaceDesc,pszOutputName,(strlen(pszOutputName)+1),API_CHRMAX);
                    ptAVInterfaceTable[resIter].vl_ocStbHostAVInterfaceType  = hrDeviceOther;
                    break;
            }
            resIter++;
            if(resIter > SNMP_MAX_ROWS)
            {
                resIter = 0; /*if rows increses than 20 , making the count as 0*/
            }
    
        }//for loop
    
    
    }//for loop

///* 1394 Av index**/  
        
        /** TO get the 1394 port and stream details for AVinterfacelist*/
#ifdef USE_1394
    if(vl_isFeatureEnabled((unsigned char *)"FEATURE.1394.SUPPORT"))
    {
        int pNum1394Interfaces;
        pNum1394Interfaces = 16;
        VL_1394_INTERFACE_INFO interfaceInfo;
        //vlMemSet(&interfaceInfo, 0, sizeof(interfaceInfo) , sizeof(interfaceInfo));
        memset(&interfaceInfo, 0, sizeof(interfaceInfo));

        int result = -1;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP","!!!! calling CHAL1394_SnmpGet1394Table\n");
        result = CHAL1394_SnmpGet1394Table(&pNum1394Interfaces, &interfaceInfo);
        if(result == 0)
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP","!!!! CHAL1394_SnmpGet1394Table returned pNum1394Interfaces : %d, resIter=%d\n", pNum1394Interfaces, resIter);

        //ptAVInterfaceTable.resourceIdInfo.Ieee1394port = interfaceInfo.portNum;
            vlMemCpy(ptAVInterfaceTable[resIter].vl_ocStbHostAVInterfaceDesc,"1394",(strlen("1394")+1),API_CHRMAX);
            ptAVInterfaceTable[resIter].vl_ocStbHostAVInterfaceType  = ocStbHost1394Out;
            ptAVInterfaceTable[resIter].vl_ocStbHostAVInterfaceStatus =  OCSTBHOSTAVINTERFACESTATUS_ENABLED ; //default setting as enable
            resIter++;
            if(resIter > SNMP_MAX_ROWS)
            {
                resIter = 0; /*if rows increses than 20 , making the count as 0, as we supports only 20 rows, ( we normaly won't get into this case , worst case)*/
            }

        } //if condtion for 1394 initialization
        else
        {           //IT shows the 1394 items are Disabled ,i.e Not activated (no certification keys)
        //No activiation so PORTS failed
        //ptAVInterfaceTable.resourceIdInfo.Ieee1394port = 99;
            vlMemCpy(ptAVInterfaceTable[resIter].vl_ocStbHostAVInterfaceDesc,"1394-OUT",(strlen("1394-OUT")+1),API_CHRMAX);
            ptAVInterfaceTable[resIter].vl_ocStbHostAVInterfaceType  = ocStbHost1394Out;
            ptAVInterfaceTable[resIter].vl_ocStbHostAVInterfaceStatus =  OCSTBHOSTAVINTERFACESTATUS_DISABLED; //default setting as enable
            resIter++;
            if(resIter > SNMP_MAX_ROWS)
            {
                resIter = 0; /*if rows increses than 20 , making the count as 0, as we supports only 20 rows, ( we normaly won't get into this case , worst case)*/
            }
                 //No activiation so PORTS failed
        }
       *ptSizeOfResList = resIter; //includes all res index and 1394 index 
       //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP","CIOResourceMgr:: pAvailableResourceVector.size()= %d ----------\t *ptSizeOfResList \n",resIter, * ptSizeOfResList);
    }
#endif // USE_1394

        
}

/**
 * @brief This function is used to retrieve information such as output type (DVI/HDMI), Connection status,
 *  Repeater Status, HDCP Status, Color type (rgb,ycc422,ycc444), video FrameRate, audio video format,
 *  sample rate etc and fills these info to the DVI HDMI Table.
 *
 * @param[in] dviHdmiInfo Pointer which contains information about DVI and HDMI.
 * @param[in] retStatus Return state of HAL_SNMP_Get_DviHdmi_info() function.
 * @param[out] ptDviHdmiTable Table which stores all information about DVI HDMI 
 *  including video audio format, sample rate etc.
 *
 * @return None
 */
void snmpAvOutInterface::fillUp_DviHdmiTable(SNMPocStbHostDVIHDMITable_t* ptDviHdmiTable, void* dviHdmiInfo, unsigned int retStatus)
{
    if((ptDviHdmiTable == NULL) || (dviHdmiInfo == NULL))
        return;

    SNMP_INTF_DVIHDMI_Info* ptDviHdmiInfo;
    ptDviHdmiInfo = (SNMP_INTF_DVIHDMI_Info*)dviHdmiInfo;


    if(retStatus)
    {
        if(ptDviHdmiInfo->isHdmi)
        {
            ptDviHdmiTable->vl_ocStbHostDVIHDMIOutputType = TYPE_HDMI;
        }
        else
        {
            ptDviHdmiTable->vl_ocStbHostDVIHDMIOutputType = TYPE_DVI;
        }

        if(ptDviHdmiInfo->connStatus)
        {
            ptDviHdmiTable->vl_ocStbHostDVIHDMIConnectionStatus =STATUS_TRUE;
        }
        else
        {
            ptDviHdmiTable->vl_ocStbHostDVIHDMIConnectionStatus = STATUS_FALSE;
        }

        if(ptDviHdmiInfo->repStatus)
        {
            ptDviHdmiTable->vl_ocStbHostDVIHDMIRepeaterStatus = STATUS_TRUE;
        }
        else
        {
            ptDviHdmiTable->vl_ocStbHostDVIHDMIRepeaterStatus = STATUS_FALSE;
        }

        if(ptDviHdmiInfo->videoXmisStatus)
        {
            ptDviHdmiTable->vl_ocStbHostDVIHDMIVideoXmissionStatus = STATUS_TRUE;
        }
        else
        {
            ptDviHdmiTable->vl_ocStbHostDVIHDMIVideoXmissionStatus = STATUS_FALSE;
        }

        if(ptDviHdmiInfo->bHDCPEnabled)
        {
            ptDviHdmiTable->vl_ocStbHostDVIHDMIHDCPStatus = STATUS_TRUE;

        }
        else
        {
            ptDviHdmiTable->vl_ocStbHostDVIHDMIHDCPStatus = STATUS_FALSE;
        }

        if(ptDviHdmiInfo->videoMuteStatus)
        {
            ptDviHdmiTable->vl_ocStbHostDVIHDMIVideoMuteStatus = STATUS_TRUE;
        }
        else
        {
            ptDviHdmiTable->vl_ocStbHostDVIHDMIVideoMuteStatus = STATUS_FALSE;
        }

        switch(ptDviHdmiInfo->videoFormat)
        {

            case DISP_FORMAT_480I:
                ptDviHdmiTable->vl_ocStbHostDVIHDMIOutputFormat = OUTPUTFORMAT_FORMAT480I    ;
                break;
            case DISP_FORMAT_480P:
                ptDviHdmiTable->vl_ocStbHostDVIHDMIOutputFormat = OUTPUTFORMAT_FORMAT480P ;
                break;
            case DISP_FORMAT_720P:
                ptDviHdmiTable->vl_ocStbHostDVIHDMIOutputFormat = OUTPUTFORMAT_FORMAT720P ;
                break;
            case DISP_FORMAT_1080I:
                ptDviHdmiTable->vl_ocStbHostDVIHDMIOutputFormat = OUTPUTFORMAT_FORMAT1080I ;
                break;
            case DISP_FORMAT_1080P:
                ptDviHdmiTable->vl_ocStbHostDVIHDMIOutputFormat = OUTPUTFORMAT_FORMAT1080p ;
                break;    
            case DISP_FORMAT_2160P:
                ptDviHdmiTable->vl_ocStbHostDVIHDMIOutputFormat = OUTPUTFORMAT_FORMAT2160p ;
                break;    
            case DISP_FORMAT_PassThru:
                ptDviHdmiTable->vl_ocStbHostDVIHDMIOutputFormat = OUTPUTFORMAT_FORMAT1080p ;
                break;
        }

        switch(ptDviHdmiInfo->aspectRatio)
        {

            case ASPECT_RATIO_4_3:
                ptDviHdmiTable->vl_ocStbHostDVIHDMIAspectRatio = ASPECTRATIO_FOURBYTHREE;
            break;
            case ASPECT_RATIO_16_9:
                ptDviHdmiTable->vl_ocStbHostDVIHDMIAspectRatio = ASPECTRATIO_SIXTEENBYNINE;
                break;
            case MAX_ASPECT_RATIO:
                ptDviHdmiTable->vl_ocStbHostDVIHDMIAspectRatio = ASPECTRATIO_OTHER;
                break;
        }

    
            //DVIHDMIAudioFormat_t
        switch(ptDviHdmiInfo->audioFormat)
        {
            case AUDIO_FMT_MPEG12:
            /* MPEG layer 1 and layer 2*/
            ptDviHdmiTable->vl_ocStbHostDVIHDMIAudioFormat = OCSTBHOSTDVIHDMIAUDIOFORMAT_MPEG1L1L2;
            break;
            case AUDIO_FMT_MP3:
            /* MPEG Layer 3        */
            ptDviHdmiTable->vl_ocStbHostDVIHDMIAudioFormat = OCSTBHOSTDVIHDMIAUDIOFORMAT_MPEG1L3;
            break;
            case AUDIO_FMT_AAC:
            /* Advanced Audio Coding. Part of MPEG-4 */
            ptDviHdmiTable->vl_ocStbHostDVIHDMIAudioFormat = OCSTBHOSTDVIHDMIAUDIOFORMAT_MPEG4;
            break;
            case AUDIO_FMT_AAC_PLUS:
            /* AAC plus SBR. aka MPEG-4 High Efficiency (AAC-HE) */
            ptDviHdmiTable->vl_ocStbHostDVIHDMIAudioFormat = OCSTBHOSTDVIHDMIAUDIOFORMAT_EAC3;
            break;
            case AUDIO_FMT_AC3:
            /* Dolby Digital AC3 audio */
            ptDviHdmiTable->vl_ocStbHostDVIHDMIAudioFormat =
            OCSTBHOSTDVIHDMIAUDIOFORMAT_AC3;
            break;
            case AUDIO_FMT_DTS:
            /* Digital Surround sound. */
            ptDviHdmiTable->vl_ocStbHostDVIHDMIAudioFormat = OCSTBHOSTDVIHDMIAUDIOFORMAT_DTS;
            break;
            case AUDIO_FMT_LPCM:
            /*PULSE CODE MODULATION. */
            ptDviHdmiTable->vl_ocStbHostDVIHDMIAudioFormat = OCSTBHOSTDVIHDMIAUDIOFORMAT_LPCM;
            break;
            default:
            ptDviHdmiTable->vl_ocStbHostDVIHDMIAudioFormat = OCSTBHOSTDVIHDMIAUDIOFORMAT_OTHER;
            //                OCSTBHOSTDVIHDMIAUDIOFORMAT_LPCM=2,
            //            OCSTBHOSTDVIHDMIAUDIOFORMAT_MPEG2=7,
            //            OCSTBHOSTDVIHDMIAUDIOFORMAT_MPEG4=8,
            //            OCSTBHOSTDVIHDMIAUDIOFORMAT_ATRAC=10
        }
    
            //DVIHDMIAudioSampleRate_t
        switch(ptDviHdmiInfo->audioSampleRate)
        {
            case AUDIO_Sample_Rate_Other:
                ptDviHdmiTable->vl_ocStbHostDVIHDMIAudioSampleRate = OCSTBHOSTDVIHDMIAUDIOSAMPLERATE_OTHER;
                break;
            case AUDIO_Sample_Rate_32kHz:
                ptDviHdmiTable->vl_ocStbHostDVIHDMIAudioSampleRate = OCSTBHOSTDVIHDMIAUDIOSAMPLERATE_SAMPLERATE32KHZ;
                break;
            case AUDIO_Sample_Rate_44kHz:
                ptDviHdmiTable->vl_ocStbHostDVIHDMIAudioSampleRate =OCSTBHOSTDVIHDMIAUDIOSAMPLERATE_SAMPLERATE44KHZ;
                break;
            case AUDIO_Sample_Rate_48kHz:
                ptDviHdmiTable->vl_ocStbHostDVIHDMIAudioSampleRate = OCSTBHOSTDVIHDMIAUDIOSAMPLERATE_SAMPLERATE48KHZ;
                break;
            case AUDIO_Sample_Rate_88kHz:
                ptDviHdmiTable->vl_ocStbHostDVIHDMIAudioSampleRate = OCSTBHOSTDVIHDMIAUDIOSAMPLERATE_SAMPLERATE88KHZ;
                break;
            case AUDIO_Sample_Rate_96kHz:
                ptDviHdmiTable->vl_ocStbHostDVIHDMIAudioSampleRate = OCSTBHOSTDVIHDMIAUDIOSAMPLERATE_SAMPLERATE96KHZ;
                break;
            case AUDIO_Sample_Rate_176kHz:
                ptDviHdmiTable->vl_ocStbHostDVIHDMIAudioSampleRate = OCSTBHOSTDVIHDMIAUDIOSAMPLERATE_SAMPLERATE176KHZ;
                break;
            case AUDIO_Sample_Rate_192kHz:
        
                ptDviHdmiTable->vl_ocStbHostDVIHDMIAudioSampleRate = OCSTBHOSTDVIHDMIAUDIOSAMPLERATE_SAMPLERATE192KHZ;
                break;
            default:
                ptDviHdmiTable->vl_ocStbHostDVIHDMIAudioSampleRate = OCSTBHOSTDVIHDMIAUDIOSAMPLERATE_OTHER;
        }
        
        ptDviHdmiTable->vl_ocStbHostDVIHDMIAudioChannelCount = ptDviHdmiInfo->audioChanCount;
        
    switch(ptDviHdmiInfo->connectedDeviceHDCPStatus){
        case HDCP_DEVICE_STATUS_NON_COMPLIANT:
            ptDviHdmiTable->vl_ocStbHostDVIHDMIHostDeviceHDCPStatus = HDCPSTATUS_NONHDCPDEVICE;
            break;
        case HDCP_DEVICE_STATUS_COMPLIANT:
            ptDviHdmiTable->vl_ocStbHostDVIHDMIHostDeviceHDCPStatus= HDCPSTATUS_COMPLIANTHDCPDEVICE ;
            break;
        case HDCP_DEVICE_STATUS_REVOKED:
            ptDviHdmiTable->vl_ocStbHostDVIHDMIHostDeviceHDCPStatus = HDCPSTATUS_REVOKEDHDCPDEVICE;
            break;
        }
    
        if(ptDviHdmiInfo->audioMuteStatus)
        {
            ptDviHdmiTable->vl_ocStbHostDVIHDMIAudioMuteStatus = STATUS_TRUE;
        }
        else
        {
            ptDviHdmiTable->vl_ocStbHostDVIHDMIAudioMuteStatus = STATUS_FALSE;
        }
    
        switch(ptDviHdmiInfo->audioSampleSize){
        case AUDIO_Sample_Size_Unknown:
            ptDviHdmiTable->vl_ocStbHostDVIHDMIAudioSampleSize = notValid;
            break;
        case AUDIO_Sample_Size_16:
            ptDviHdmiTable->vl_ocStbHostDVIHDMIAudioSampleSize= sample16Bit ;
            break;
        case AUDIO_Sample_Size_20:
            ptDviHdmiTable->vl_ocStbHostDVIHDMIAudioSampleSize = sample20Bit;
            break;
        case AUDIO_Sample_Size_24:
            ptDviHdmiTable->vl_ocStbHostDVIHDMIAudioSampleSize = sample24Bit;
            break;
        }
    
        switch(ptDviHdmiInfo->colorSpace){
        case HDMI_AVIFRAME_COLOR_SPACE_RGB:
            ptDviHdmiTable->vl_ocStbHostDVIHDMIColorSpace = rgb;
            break;
        case HDMI_AVIFRAME_COLOR_SPACE_YCbCr422:
            ptDviHdmiTable->vl_ocStbHostDVIHDMIColorSpace= ycc422 ;
            break;
        case HDMI_AVIFRAME_COLOR_SPACE_YCbCr444:
            ptDviHdmiTable->vl_ocStbHostDVIHDMIColorSpace = ycc444;
            break;
    //        case HDMI_AVIFRAME_COLOR_SPACE_RSVD:
    //                    ptDviHdmiTable->vl_ocStbHostDVIHDMIColorSpace = rsvd;
            //break;
        }
        switch(ptDviHdmiInfo->videoFrameRate){
        case VIDEO_Frame_Rate_23_976:
            ptDviHdmiTable->vl_ocStbHostDVIHDMIFrameRate = frameRateCode1;
            break;
        case VIDEO_Frame_Rate_24:
            ptDviHdmiTable->vl_ocStbHostDVIHDMIFrameRate= frameRateCode2 ;
            break;
        case VIDEO_Frame_Rate_29_97:
            ptDviHdmiTable->vl_ocStbHostDVIHDMIFrameRate = frameRateCode3;
            break;
        case VIDEO_Frame_Rate_30:
            ptDviHdmiTable->vl_ocStbHostDVIHDMIFrameRate = frameRateCode4;
            break;
        case VIDEO_Frame_Rate_59_94:
            ptDviHdmiTable->vl_ocStbHostDVIHDMIFrameRate = frameRateCode5;
            break;
        case VIDEO_Frame_Rate_60:
        default :
            ptDviHdmiTable->vl_ocStbHostDVIHDMIFrameRate = frameRateCode6;
        break;
        }
    
    } 
    else
    {    
        ptDviHdmiTable->vl_ocStbHostDVIHDMIOutputType        = TYPE_HDMI;
        ptDviHdmiTable->vl_ocStbHostDVIHDMIConnectionStatus    = STATUS_FALSE;
        ptDviHdmiTable->vl_ocStbHostDVIHDMIRepeaterStatus    = STATUS_FALSE;
        ptDviHdmiTable->vl_ocStbHostDVIHDMIVideoXmissionStatus  = STATUS_FALSE;
        ptDviHdmiTable->vl_ocStbHostDVIHDMIOutputFormat        = OUTPUTFORMAT_FORMAT480I    ;
        ptDviHdmiTable->vl_ocStbHostDVIHDMIAspectRatio        = ASPECTRATIO_OTHER;
        ptDviHdmiTable->vl_ocStbHostDVIHDMIHDCPStatus        = STATUS_FALSE;
        ptDviHdmiTable->vl_ocStbHostDVIHDMIVideoMuteStatus    = STATUS_TRUE;
        ptDviHdmiTable->vl_ocStbHostDVIHDMIAudioMuteStatus    = STATUS_FALSE;
        ptDviHdmiTable->vl_ocStbHostDVIHDMIAudioFormat = OCSTBHOSTDVIHDMIAUDIOFORMAT_OTHER;
        ptDviHdmiTable->vl_ocStbHostDVIHDMIAudioSampleRate = OCSTBHOSTDVIHDMIAUDIOSAMPLERATE_OTHER;
    }
    
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP","%s : returning %d bytes of EDID data\n", __FUNCTION__, ptDviHdmiInfo->edid_len);
    vlMemCpy(ptDviHdmiTable->vl_ocStbHostDVIHDMIEdid, ptDviHdmiInfo->edid, ptDviHdmiInfo->edid_len, sizeof(ptDviHdmiTable->vl_ocStbHostDVIHDMIEdid));
    ptDviHdmiTable->vl_ocStbHostDVIHDMIEdid_len = ptDviHdmiInfo->edid_len;
    if(ptDviHdmiInfo->edid_len > 0)
    {
        snprintf(ptDviHdmiTable->vl_ocStbHostDVIHDMIEdidVersion, sizeof(ptDviHdmiTable->vl_ocStbHostDVIHDMIEdidVersion), "%u.%u", ptDviHdmiInfo->edid[18], ptDviHdmiInfo->edid[19]);
    }
    else
    {
        vlStrCpy(ptDviHdmiTable->vl_ocStbHostDVIHDMIEdidVersion, "0.0", sizeof(ptDviHdmiTable->vl_ocStbHostDVIHDMIEdidVersion));
    }
    ptDviHdmiTable->vl_ocStbHostDVIHDMIEdidVersion_len = strlen(ptDviHdmiTable->vl_ocStbHostDVIHDMIEdidVersion);
    ptDviHdmiTable->vl_ocStbHostDVIHDMIMaxDeviceCount = ptDviHdmiInfo->maxDevCount;
    ptDviHdmiTable->vl_ocStbHostDVIHDMILipSyncDelay = ptDviHdmiInfo->lipSyncDelay;

    ptDviHdmiTable->vl_ocStbHostDVIHDMIFeatures_len = ptDviHdmiInfo->features_len;
    vlMemCpy(ptDviHdmiTable->vl_ocStbHostDVIHDMIFeatures, ptDviHdmiInfo->features, ptDviHdmiInfo->features_len, sizeof(ptDviHdmiTable->vl_ocStbHostDVIHDMIFeatures));
    
    ptDviHdmiTable->vl_ocStbHostDVIHDMICecFeatures_len = ptDviHdmiInfo->cecFeatures_len;
    vlMemCpy(ptDviHdmiTable->vl_ocStbHostDVIHDMICecFeatures, ptDviHdmiInfo->cecFeatures, ptDviHdmiInfo->cecFeatures_len, sizeof(ptDviHdmiTable->vl_ocStbHostDVIHDMICecFeatures));
}

/**
 * @brief This function receives the input parameter from where it fills the data of MPEG2 video 
 * like program number, audio/video pid, PCR LockStatus, CCI value to the MPEG2 Content table
 *
 * @param[in] mpeg2ContentInfo Pointer containing the info about MPEG2 video.
 * @param[in] retStatus Return status for HAL_SNMP_Get_Mpeg2Content_info() function.
 * @param[out] ptMpeg2ContentTable Content table where all info about MPEG2 video will be stored.
 *
 * @return None
 */
void snmpAvOutInterface::fillUp_Mpeg2ContentTable(SNMPocStbHostMpeg2ContentTable_t* ptMpeg2ContentTable, void* mpeg2ContentInfo, unsigned int retStatus)
{

    if((ptMpeg2ContentTable == NULL) || (mpeg2ContentInfo == NULL))
        return;
    
    HAL_SNMP_Mpeg2ContentTable_t* ptMpeg2Info = (HAL_SNMP_Mpeg2ContentTable_t*)mpeg2ContentInfo;

    ptMpeg2ContentTable->vl_ocStbHostMpeg2ContentProgramNumber     = ptMpeg2Info->programNumber ;
    ptMpeg2ContentTable->vl_ocStbHostMpeg2ContentTransportStreamID = ptMpeg2Info->transportStreamID;
    ptMpeg2ContentTable->vl_ocStbHostMpeg2ContentTotalStreams      = ptMpeg2Info->totalStreams ;
    ptMpeg2ContentTable->vl_ocStbHostMpeg2ContentPCRPID            = ptMpeg2Info->pcrPID;
    ptMpeg2ContentTable->vl_ocStbHostMpeg2ContentSelectedVideoPID  = ptMpeg2Info->selectedVideoPID;
    ptMpeg2ContentTable->vl_ocStbHostMpeg2ContentSelectedAudioPID  = ptMpeg2Info->selectedAudioPID;

    if(ptMpeg2Info->PCRLockStatus)
    {
        ptMpeg2ContentTable->vl_ocStbHostMpeg2ContentPCRLockStatus = MPEG2_PCRLOCKSTATUS_LOCKED;
    }
    else
    {
        ptMpeg2ContentTable->vl_ocStbHostMpeg2ContentPCRLockStatus = MPEG2_PCRLOCKSTATUS_NOTLOCKED;
    }

    ptMpeg2ContentTable->vl_ocStbHostMpeg2ContentDecoderPTS         = ptMpeg2Info->decoderPTS;
    ptMpeg2ContentTable->vl_ocStbHostMpeg2ContentDiscontinuities    = ptMpeg2Info->discontinuities;
    ptMpeg2ContentTable->vl_ocStbHostMpeg2ContentPktErrors          = ptMpeg2Info->pktErrors;
    ptMpeg2ContentTable->vl_ocStbHostMpeg2ContentPipelineErrors     = ptMpeg2Info->pipelineErrors;
    ptMpeg2ContentTable->vl_ocStbHostMpeg2ContentDecoderRestarts    = ptMpeg2Info->decoderRestarts;
    
    switch(ptMpeg2Info->otherAudioPIDs)
    {
        case 1:
        ptMpeg2ContentTable->vl_ocStbHostMpeg2ContentOtherAudioPIDs = STATUS_TRUE ;// 1;
        break;
        case 0:
        ptMpeg2ContentTable->vl_ocStbHostMpeg2ContentOtherAudioPIDs = STATUS_FALSE ; // 2
        break;
        default :
        //if found noOfotherAudioPids, here it returns number of other audiopids
        ptMpeg2ContentTable->vl_ocStbHostMpeg2ContentOtherAudioPIDs = STATUS_TRUE ; // 2
        break;
    }
    
    switch(ptMpeg2Info->cciValue)
    {
        case   0:
        ptMpeg2ContentTable->vl_ocStbHostMpeg2ContentCCIValue = OCSTBHOSTMPEG2CONTENTCCIVALUE_COPYFREELY;
        break;
        case   1:
        ptMpeg2ContentTable->vl_ocStbHostMpeg2ContentCCIValue = OCSTBHOSTMPEG2CONTENTCCIVALUE_COPYNOMORE;
        break;
        case   2:
        ptMpeg2ContentTable->vl_ocStbHostMpeg2ContentCCIValue = OCSTBHOSTMPEG2CONTENTCCIVALUE_COPYONEGENERATION;
        break;
        case   3:
        ptMpeg2ContentTable->vl_ocStbHostMpeg2ContentCCIValue = OCSTBHOSTMPEG2CONTENTCCIVALUE_COPYNEVER;
        break;
        case 4:
        default :
        ptMpeg2ContentTable->vl_ocStbHostMpeg2ContentCCIValue = OCSTBHOSTMPEG2CONTENTCCIVALUE_UNDEFINED;
        break;
    }
    switch(ptMpeg2Info->apsValue)
    {
        case   0:
        ptMpeg2ContentTable->vl_ocStbHostMpeg2ContentAPSValue = OCSTBHOSTMPEG2CONTENTAPSVALUE_NOMACROVISION;
        break;
        case   1:
        ptMpeg2ContentTable->vl_ocStbHostMpeg2ContentAPSValue = OCSTBHOSTMPEG2CONTENTAPSVALUE_TYPE1;
        break;
        case   2:
        ptMpeg2ContentTable->vl_ocStbHostMpeg2ContentAPSValue = OCSTBHOSTMPEG2CONTENTAPSVALUE_TYPE2;
        break;
        case   3:
        ptMpeg2ContentTable->vl_ocStbHostMpeg2ContentAPSValue = OCSTBHOSTMPEG2CONTENTAPSVALUE_TYPE3;
        break;
        default :
        ptMpeg2ContentTable->vl_ocStbHostMpeg2ContentAPSValue = OCSTBHOSTMPEG2CONTENTAPSVALUE_NOTDEFINED;
        break;
    }
    
    
    switch(ptMpeg2Info->broadcastFlagStatus)
    {
        case 1:
        ptMpeg2ContentTable->vl_ocStbHostMpeg2ContentBroadcastFlagStatus = STATUS_TRUE ;// 1;
        break;
        case 0:
        ptMpeg2ContentTable->vl_ocStbHostMpeg2ContentBroadcastFlagStatus  = STATUS_FALSE ; // 2
        break;
        default :
        ptMpeg2ContentTable->vl_ocStbHostMpeg2ContentBroadcastFlagStatus = STATUS_FALSE ;
        break;
    }
    
    switch(ptMpeg2Info->epnStatus)
    {
        case 1:
        ptMpeg2ContentTable->vl_ocStbHostMpeg2ContentEPNStatus = STATUS_TRUE ;// 1;
        break;
        case 0:
        ptMpeg2ContentTable->vl_ocStbHostMpeg2ContentEPNStatus = STATUS_FALSE ; // 2
        break;
        default :
        ptMpeg2ContentTable->vl_ocStbHostMpeg2ContentEPNStatus = STATUS_FALSE ; // 2
        break;
    }
        
    switch(ptMpeg2Info->citStatus)
    {
        case 1:
        ptMpeg2ContentTable->vl_ocStbHostMpeg2ContentCITStatus = STATUS_TRUE ;// 1;
        break;
        case 0:
        ptMpeg2ContentTable->vl_ocStbHostMpeg2ContentCITStatus = STATUS_FALSE ; // 2
        break;
        default :
        ptMpeg2ContentTable->vl_ocStbHostMpeg2ContentCITStatus = STATUS_FALSE ; // 2
        break;
    }
}

/**
 * @brief This function receives the input parameter from where it gets the data of SPDIF stream
 * like format, mute status and fills these data to the SPDIF Content table
 *
 * @param[in] spdifInfo Pointer which contains SPDIF stream informations.
 * @param[in] retStatus Return status of HAL_SNMP_Get_Spdif_info() function call.
 * @param[out] vl_SPDIfTable Table where SPDIF info will be stored.
 *
 * @return None
 */
void snmpAvOutInterface::fillUp_SpdifTable(SNMPocStbHostSPDIfTable_t* vl_SPDIfTable, void* spdifInfo, unsigned int retStatus)
{
    if((vl_SPDIfTable == NULL) || (spdifInfo == NULL))
    {
        return;
    }
    
    HAL_SNMP_SpdifTable_t* ptSpdifInfo;
    ptSpdifInfo = (HAL_SNMP_SpdifTable_t*)spdifInfo;
    
    if(retStatus)
    {
        switch(ptSpdifInfo->audioFormat)
        {
            case AUDIO_FMT_LPCM:
                vl_SPDIfTable->vl_ocStbHostSPDIfAudioFormat = OCSTBHOSTSPDIFAUDIOFORMAT_LPCM;
                break;
            case AUDIO_FMT_AC3:
                vl_SPDIfTable->vl_ocStbHostSPDIfAudioFormat =                   OCSTBHOSTSPDIFAUDIOFORMAT_AC3;
                break;
            case AUDIO_FMT_DTS:
                vl_SPDIfTable->vl_ocStbHostSPDIfAudioFormat =
                OCSTBHOSTSPDIFAUDIOFORMAT_DTS;
                break;
            case AUDIO_FMT_OTHER:
                vl_SPDIfTable->vl_ocStbHostSPDIfAudioFormat =OCSTBHOSTSPDIFAUDIOFORMAT_OTHER;
                break;
        }
        
        if(ptSpdifInfo->audioMuteStatus)
        {
            vl_SPDIfTable->vl_ocStbHostSPDIfAudioMuteStatus = STATUS_TRUE;
        }
        else
        {
            vl_SPDIfTable->vl_ocStbHostSPDIfAudioMuteStatus = STATUS_FALSE;
        }
        
    }
}
