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
#include <vlHalSnmpHdmiInterface.h>
#include <vlHalSnmpUtils.h>

// gtena: define module tag with HAL Media
#define __MTAG__ VL_HALMEDIA

#define vlMemSet(p,v,s,t) memset(p,v,s)


unsigned int HAL_SNMP_Get_DviHdmi_info(SNMP_INTF_DVIHDMI_Info* ptDviHdmiInfo)
{
	return 1;
}

unsigned int HAL_SNMP_ocStbHostDVIHDMIVideoFormatTableData(SNMPocStbHostDVIHDMIVideoFormatTable_t* vl_DVIHDMIVideoFormatTable, int *pnCount)
{
	return 1;
}

unsigned int HAL_SNMP_Get_Display_Handle_Count(unsigned int* ptDispHandleCount)
{
	unsigned int retStatus;
       retStatus = 0;

	return retStatus;

}

unsigned int HAL_SNMP_Get_AvOut_Info(void* ptAVOutInterfaceInfo, unsigned int maxNumOfDiplays)
{
	return 1;
}

typedef enum _mpe_MediaAspectRatio
{
    MPE_ASPECT_RATIO_UNKNOWN = -1,
    MPE_ASPECT_RATIO_4_3 = 2,
    MPE_ASPECT_RATIO_16_9,
    MPE_ASPECT_RATIO_2_21_1
} mpe_MediaAspectRatio;

unsigned int HAL_SNMP_Get_ComponentVideo_info(void* pComponentInfo)
{

	return 1;

}

unsigned int HAL_SNMP_Get_Mpeg2Content_info(void* pMpeg2ContentInfo)
{
	return 1;
	
}

unsigned int HAL_SNMP_Get_RFChan_info()
{
	unsigned int retStatus = 1;

	return retStatus;
}

unsigned int HAL_SNMP_Get_AnalogVideo_info()
{
	unsigned int retStatus = 1;

	return retStatus;
}

unsigned int HAL_SNMP_Get_Spdif_info(void *pSpdifInfo)
{
    return 1;
}

unsigned int HAL_SNMP_Get_ProgramStatus_info(void* pConnDetails, unsigned int listIter)
{
	
	return 0;
}

void HAL_SNMP_Get_DevListSize(unsigned int* devListSize)
{
return;
}

unsigned int HAL_SNMP_Get_Tuner_Info(void* ptSnmpHALTunerInfo, unsigned long tunerIndex)
{
    return 0;
}

unsigned int AddToConnList(ConnDetails* pConnDetails)
{
    return 0;
}


unsigned int HAL_SNMP_Get_Tuner_Count(unsigned int* ptTunerCount)
{
    return 0;
}

unsigned int RemFromConnList(unsigned long devIndex, deviceType devType)
{
    return 1;
}
