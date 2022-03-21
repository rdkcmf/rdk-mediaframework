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

/** @defgroup SNMP_INT_AV SNMP Interface AV Data
 * This module is use to get the info of data like host id, capabilities, 
 * firmware etc. from AV out port.
 * @ingroup SNMP_INTRF
 * @{
 */
  
#ifndef __SNMP_AVOUT_INTERFACE__
#define __SNMP_AVOUT_INTERFACE__

#include "SnmpIORM.h"

class snmpAvOutInterface
{

       public:
        
        snmpAvOutInterface();
        ~snmpAvOutInterface();
        void Snmp_getallAVdevicesindex(SNMPocStbHostAVInterfaceTable_t* vl_AVInterfaceTable, int* sizeoflist);
        void snmp_get_DviHdmi_info(unsigned long vlghandle, AV_INTERFACE_TYPE avintercacetype, SNMPocStbHostDVIHDMITable_t* ptDviHdmiTable);
        void snmp_get_ocStbHostDVIHDMIVideoFormatTableData(SNMPocStbHostDVIHDMIVideoFormatTable_t* vl_DVIHDMIVideoFormatTable, int *pnCount);
        void snmp_get_ComponentVideo_info(unsigned long vlghandle, AV_INTERFACE_TYPE avintercacetype, SNMPocStbHostComponentVideoTable_t* vl_ComponentVideoTable);
        void snmp_get_RFChan_info(unsigned long vlghandle, AV_INTERFACE_TYPE avintercacetype, SNMPocStbHostRFChannelOutTable_t* vl_RFChannelOutTable);
        void snmp_get_AnalogVideo_info(unsigned long vlghandle, AV_INTERFACE_TYPE avintercacetype, SNMPocStbHostAnalogVideoTable_t* vl_AnalogVideoTable);
        void snmp_get_Spdif_info(unsigned long vlghandle, AV_INTERFACE_TYPE avinterfacetype, SNMPocStbHostSPDIfTable_t* vl_SPDIfTable);
        void SnmpGet1394Table(/*&pNum1394Interfaces, &interfaceInfo*/);
        void SnmpGet1394AllDevicesTable(/*&pNumDevices, connDevInfo*/);
        void SnmpIs1394PortStreaming(/*PORT_ONE_INFO, &PIStreaming*/);
        void GetDecoderSNMPInfo(/*&decoderSNMPInfo_status*/);
        void getMpeg2ContentTable(/*pysicalID, Mpegactivepgnumber*/);
        void SNMPGetApplicationInfo(/*appInfo*/);
        void SNMPGet_ocStbHostHostID(/*HostID, 255*/);
        void SNMPGet_ocStbHostCapabilities(/*hostcap*/);
        void SNMPget_ocStbHostSecurityIdentifier(/*&HostCaTypeInfo*/);
        void CommonDownloadMgrSnmpHostFirmwareStatus(/*&objfirmwarestatus*/);
        void SNMPGetCableCardCertInfo(/*&Mcardcertinfo*/);    
        void get_display_device();    
        unsigned int get_display_handle_count();
//        unsigned long* get_display_handles(/*numDisplays, pDisplayHandles, 0*/);
        void get_avOutinterface_info(void* ptAVOutInterfaceInfo, unsigned int maxNumOfDiplays);    
        void SNMPGetCardIdBndStsOobMode(/*&oobInfo*/);    
        void SNMPGetGenFetResourceId(/*&nLong*/);        
        void SNMPGetGenFetTimeZoneOffset(/*&(vl_ProxyCardInfo->ocStbHostCardTimeZoneOffset)*/);        
        void SNMPGetGenFetDayLightSavingsTimeDelta(/*vl_ProxyCardInfo->ocStbHostCardDaylightSavingsTimeDelta*/);    
        void SNMPGetGenFetDayLightSavingsTimeEntry(/*(unsigned long*)&(vl_ProxyCardInfo->ocStbHostCardDaylightSavingsTimeEntry)*/);        
        void SNMPGetGenFetDayLightSavingsTimeExit(/*(unsigned long*)&(vl_ProxyCardInfo->ocStbHostCardDaylightSavingsTimeExit)*/);        
        void SNMPGetGenFetEasLocationCode(/*&nChar, &nShort*/);        
        void SNMPGetGenFetVctId(/*&nShort*/);        
        void SNMPGetCpAuthKeyStatus(/*(int*)&(vl_ProxyCardInfo->ocStbHostCardCpAuthKeyStatus)*/);        
        void SNMPGetCpCertCheck(/*(int*)&(vl_ProxyCardInfo->ocStbHostCardCpCertificateCheck)*/);        
        void SNMPGetCpCciChallengeCount(/*(unsigned long*)&(vl_ProxyCardInfo->ocStbHostCardCpCciChallengeCount)*/);    
        void SNMPGetCpKeyGenerationReqCount(/*(unsigned long*)&(vl_ProxyCardInfo->ocStbHostCardCpKeyGenerationReqCount)*/);                
        void snmp_Sys_request(/*VL_SNMP_REQUEST_GET_SYS_HOST_MEMORY_INFO, &snmpHostMemoryInfo*/);        
        void GetOOBCarouselTimeoutCount();
        void GetInbandCarouselTimeoutCount();
        void fillUp_AvOutInterface_Table(SNMPocStbHostAVInterfaceTable_t* ptAVInterfaceTable, 
                    void* ptAVOutInterfaceInfo, 
                    int* ptSizeOfResList,
                    unsigned int maxNumOfDiplays);
        
        void fillUp_DviHdmiTable(SNMPocStbHostDVIHDMITable_t* ptDviHdmiTable, void* dviHdmiInfo, unsigned int retStatus);
        
        void fillUp_SpdifTable(SNMPocStbHostSPDIfTable_t* vl_SPDIfTable, void* spdifInfo, unsigned int retStatus);
                
        int snmp_get_ProgramStatus_info(programInfo *vlg_programInfo, int *numberpg, int *vlg_Signal_Status);

        void snmp_get_Mpeg2Content_info(SNMPocStbHostMpeg2ContentTable_t* ptMpeg2ContentTable, unsigned int pysicalID, int Mpegactivepgnumber);

        void fillUp_Mpeg2ContentTable(SNMPocStbHostMpeg2ContentTable_t* ptMpeg2ContentTable, void* mpeg2ContentInfo, unsigned int retStatus);

    private:
};

extern "C" int SnmpGetTunerCount(unsigned int * pNumOfTuners);

#endif //__SNMP_AVOUT_INTERFACE__

/** @{ */
