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
 * @defgroup STB_HOST Host Interface
 * @ingroup SNMP_MGR
 * @ingroup STB_HOST
 * @{
 */
  
#ifndef __VL_GET_HOSTDATA_MIB_H
#define __VL_GET_HOSTDATA_MIB_H


#include "ocStbHostMibModule.h"
#include "SnmpIORM.h"
#include "ifTable.h"
#include "ipNetToPhysicalTable.h"
#include "vlSnmpHostInfo.h"

//Certification Info Includes
#include "cm_api.h"
#include "snmp_types.h"
#include "snmpManagerIf.h"

//#define CHRMAX 256


#define IFTABLE_MAX_ROW 3
#define ANALOG_SIGNAL 1
#define DIGITAL_SIGNAL 2
//#define MAX_IOD_LENGTH    128

/*  Table_free() it will free the table content */
void 
Table_free(netsnmp_tdata * table_data);
typedef enum{
     GETDATA_FAILED   = 0,
     GETDATA_SUCCESS  = 1
}GETDATA_STATUS;

/** ocStbHostAVInterfaceTable*/
  int vl_ocStbHostAVInterfaceTable_getdata(netsnmp_tdata * table_data);
  int ocStbHostAVInterfaceTable_copy_allData(netsnmp_tdata * table_data, SNMPocStbHostAVInterfaceTable_t* vlg_AVInterfaceTable, int nrows);

/** vl_ocStbHostInBandTunerTable */
  int vl_ocStbHostInBandTunerTable_getdata(netsnmp_tdata * table_data);
  int ocStbHostInBandTunerTable_copy_allData(netsnmp_tdata * table_data,int tunerAvInterfaceIx,               SNMPocStbHostInBandTunerTable_t* vl_ocStbHostInBandTunerTable_Data_obj);
  
/**      ocStbHostDVIHDMITable */


  int vl_ocStbHostDVIHDMITable_getdata(netsnmp_tdata* table_data);
  int ocStbHostDVIHDMITable_copy_allData(netsnmp_tdata * table_data, int HDMIInterfaceAvInterfaceIx           ,SNMPocStbHostDVIHDMITable_t* DVIHDMITable_obj);
/**      ocStbHostDVIHDMIAvailableVideoFormatTable */
  int vl_ocStbHostDVIHDMIAvailableVideoFormatTable_getdata(netsnmp_tdata* table_data);
  int ocStbHostDVIHDMIAvailableVideoFormatTable__createEntry_allData(netsnmp_tdata * table_data,  SNMPocStbHostDVIHDMIVideoFormatTable_t* DVIHDMIVideoFormatTable_obj,int DVIHDMIAVideoFormatIndex );


/** ocStbHostComponentVideoTable */

  int vl_ocStbHostComponentVideoTable_getdata(netsnmp_tdata * table_data);
  int ComponentVideoTable_copy_allData(netsnmp_tdata * table_data,int  ComponentVideoAvInterfaceIx,     SNMPocStbHostComponentVideoTable_t* ComponentVideoTable_obj);
  
/** ocStbHostRFChannelOutTable */

  int vl_ocStbHostRFChannelOutTable_getdata(netsnmp_tdata * table_data);
  int RFChannelOutTable_copy_allData(netsnmp_tdata* table_data, int RFChannelOutAvInterfaceIx,             SNMPocStbHostRFChannelOutTable_t* RFChannelOutTable_obj);
 
/** ocStbHostAnalogVideoTable */

  int vl_ocStbHostAnalogVideoTable_getdata(netsnmp_tdata *table_data);
  
  int AnalogVideoTable_copy_allData(netsnmp_tdata* table_data, int AnalogVideoAvInterfaceIx,             SNMPocStbHostAnalogVideoTable_t* AnalogVideoTable_obj);


/** ocStbHostProgramStatusTable*/
  int vl_ocStbHostProgramStatusTable_getdata(netsnmp_tdata * table_data);
  int ocStbHostProgramStatusTable_copy_allData(netsnmp_tdata* table_data,SNMPocStbHostProgramStatusTable_t* ProgramStatusTable_obj);
   
  int ocStbHostProgramStatusTable_DeafultIntilize(netsnmp_tdata* table_data,SNMPocStbHostProgramStatusTable_t* ProgramStatusTable_obj);
  
/** ocStbHostMpeg2ContentTable */

  int vl_ocStbHostMpeg2ContentTable_getdata(netsnmp_tdata *table_data);
  int ocStbHostMpeg2ContentTable_createEntry_allData(netsnmp_tdata * table_data, SNMPocStbHostMpeg2ContentTable_t *ProgramMPEG2_obj, int mpegtableindex);
  int ocStbHostMpeg2ContentTable_Default_Intilize(netsnmp_tdata * table_data, SNMPocStbHostMpeg2ContentTable_t *ProgramMPEG2_obj);

/** ocStbHostMpeg4ContentTable */

  int vl_ocStbHostMpeg4ContentTable_getdata(netsnmp_tdata *table_data);
  int ocStbHostMpeg4ContentTable_createEntry_allData(netsnmp_tdata * table_data, SNMPocStbHostMpeg4ContentTable_t *ProgramMPEG4_obj, int mpegtableindex);
  int ocStbHostMpeg4ContentTable_Default_Intilize(netsnmp_tdata * table_data, SNMPocStbHostMpeg4ContentTable_t *ProgramMPEG4_obj);

/** ocStbHostVc1ContentTable */

  int vl_ocStbHostVc1ContentTable_getdata(netsnmp_tdata *table_data);
  int ocStbHostVc1ContentTable_createEntry_allData(netsnmp_tdata * table_data, SNMPocStbHostVc1ContentTable_t *ProgramVC1_obj, int mpegtableindex);
  int ocStbHostVc1ContentTable_Default_Intilize(netsnmp_tdata * table_data, SNMPocStbHostVc1ContentTable_t *ProgramVC1_obj);

/**   ocStbHostSPDIfTable  */
  int vl_ocStbHostSPDIfTable_getdata(netsnmp_tdata *table_data);

  int ocStbHostSPDIfTable_createEntry_allData(netsnmp_tdata * table_data, SNMPocStbHostSPDIfTable_t *SPDIfTable_obj, int spdeftableindex);
  int ocStbHostSPDIfTable_Default_Intilize(netsnmp_tdata * table_data, SNMPocStbHostSPDIfTable_t *SPDIfTable_obj);  


/** ocStbHostIEEE1394Table    */
  int vl_ocStbHostIEEE1394Table_getdata(netsnmp_tdata * table_data);
    
  int ocStbHostIEEE1394Table_createEntry_allData(netsnmp_tdata * table_data, int IEEE1394AvInterfaceIx, IEEE1394Table_t *IEEE1394Table_obj);
  int ocStbHostIEEE1394Table_Default_Intilize(netsnmp_tdata * table_data, IEEE1394Table_t *IEEE1394Table_obj);


/** ocStbHostIEEE1394ConnectedDevicesTable */
  int vl_ocStbHostIEEE1394ConnectedDevicesTable_getdata(netsnmp_tdata * table_data);
  int ocStbHostIEEE1394ConnectedDevicesTable_createEntry_allData(netsnmp_tdata * table_data, IEEE1394ConnectedDevices_t *IEEE1394CDTable_obj, int Ieeetableindex, int ConnectedIndex );
  int ocStbHostIEEE1394ConnectedDevicesTable_Default_Intilize(netsnmp_tdata * table_data, IEEE1394ConnectedDevices_t *IEEE1394CDTable_obj);
 
/**new'ly add tables methods*/

 /**  ocStbHostSystemTempTable  */

int vl_ocStbHostSystemTempTable_getdata(netsnmp_tdata * table_data);
int ocStbHostSystemTempTable_createEntry_allData(netsnmp_tdata * table_data, SNMPocStbHostSystemTempTable_t *SystemTempTableInfolist, int SystemTempTableindex);

/**  ocStbHostSystemHomeNetworkTable  */

int vl_ocStbHostSystemHomeNetworkTable_getdata(netsnmp_tdata * table_data);
int ocStbHostSystemHomeNetworkTable_createEntry_allData(netsnmp_tdata * table_data, SNMPocStbHostSystemHomeNetworkTable_t *HomeNetworkInfolist, int HomeNetworkindex);


/**  ocStbHostSystemMemoryReportTable  */

int vl_ocStbHostSystemMemoryReportTable_getdata(netsnmp_tdata * table_data);
int ocStbHostSystemMemoryReportTable_createEntry_allData(netsnmp_tdata * table_data, SNMPocStbHostSystemMemoryReportTable_t *SystemMemoryInfolist, int SystemMemoryindex);

 /** ocStbHostCCAppInfoTable  */
int vl_ocStbHostCCAppInfoTable_load_getdata(netsnmp_tdata * table_data);
int ocStbHostCCAppInfoTable_createEntry_allData(netsnmp_tdata * table_data, SNMPocStbHostCCAppInfoTable_t *CCAppInfolist, int CCAppindex);

/** ocStbHostSoftwareApplicationInfoTable */

int vl_ocStbHostSoftwareApplicationInfoTable_getdata(netsnmp_tdata * table_data);
int ocStbHostSoftwareApplicationInfoTable_createEntry_allData(netsnmp_tdata * table_data, SNMPocStbHostSoftwareApplicationInfoTable_t *SAppInfolist, int SAppindex);

#ifdef __cplusplus
extern "C" {
#endif
    int ocStbHostSoftwareApplicationInfoTable_set_list(int nApps, SNMPocStbHostSoftwareApplicationInfoTable_t * pAppInfolist);
#ifdef __cplusplus
}
#endif

/**===============ifTable=====================*/
int vl_ifTable_getdata(netsnmp_tdata * table_data);
int ifTable_createEntry_allData(netsnmp_tdata * table_data, SNMPifTable_t *SifTablelist);
/**===============IpNetToPhysicalTable========*/
int vl_ipNetToPhysicalTable_getdata(netsnmp_tdata * table_data);
int ipNetToPhysicalTable_createEntry_allData(netsnmp_tdata * table_data, SNMPipNetToPhysicalTable_t *SipNetToPhysicallist);


/**-------------- VIVIDLOGIC certificationTable ----- gives the info about HDMI,IEEE1394,OCAP,M-CARD,COMMMONDOWNLOAD certification info---*/
int vl_VividlogiccertificationTable_getdata(netsnmp_tdata * table_data);
int Vividlogiccertification_createEntry_allData(netsnmp_tdata *
        table_data, vlMCardCertInfo_t *pcartinfo, int count);


/*=====================================================================================================*/
 /** SCALAR VARIABLE */
/* ======================================================================================================*/



  unsigned int Sget_ocStbHostSerialNumber(char* SerialNumber, int nChars);
  unsigned int Sget_ocStbHostHostID(char* HostID);
  unsigned int Sget_ocStbHostCapabilities(HostCapabilities_snmpt*  HostCapabilities_val); //enum
  unsigned int Sget_ocStbHostAvcSupport(Status_t* avcSupport);


/**     OCSTBHOST_EASOBJECTS     */
    // EasObjectsInfo_t *EasObjinfo;
  unsigned int Sget_ocStbEasMessageStateInfo( EasInfo_t* EasobectsInfo); 
/**     OCSTBHOST_DEVICESOFTWARE     */
  unsigned int Sget_ocStbHostDeviceSoftware(DeviceSInfo_t * DevSoftareInfo); 
 
/**     OCSTBHOST_SOFTWAREIMAGESTATUS     */

  unsigned int Sget_ocStbHostFirmwareDownloadStatus (FDownloadStatus_t* FirmwareStatus) ;

/** ocStbHostSoftwareApplicationInfoSigLastReadStatus ,ocStbHostSoftwareApplicationInfoSigLastReceivedTime */
  unsigned int
  Sget_ocStbHostSoftwareApplicationSigstatus(SNMPocStbHostSoftwareApplicationInfoTable_t*
  pswappStaticstatus);
  
/**     OCSTBHOST_SECURITYSUBSYSTEM     */
  unsigned int Sget_ocStbHostSecurityIdentifier(HostCATInfo_t* HostCAtypeinfo); 

/**     ocStbHostPowerStatus     */
  unsigned int Sget_ocStbHostPowerInf(HostPowerInfo_t* Hostpowerstatus);

/**     OCSTBHOST_USERSETTINGS     */
  unsigned int Sget_ocStbHostUserSettingsPreferedLanguage(char* UserSettingsPreferedLanguage);

/**     ocStbHostCardIpAddressType     */
  unsigned int Sget_ocStbHostCardInfo(CardInfo_t* CardIpInfo);

/** OCATBHOST_INFO */

  unsigned int Sget_ocstbHostInfo(SOcstbHOSTInfo_t* HostIpinfo);

 
  //void get_IP_MACadress(CardInfo_t* CardIpInfo);

/** OCATBHOST_CCMMI */

  unsigned int Sget_ocStbHostCMMIInfo(SCCMIAppinfo_t* CardIpInfo);


/** OCATBHOST_REBOOTINFO */
  unsigned int Sget_OcstbHostRebootInfo(SOcstbHostRebootInfo_t* HostRebootInfo);
  unsigned int SSet_OcstbHostRebootInfo();
/** OCATBHOST_MEMORYINFO */
  unsigned int Sget_OcstbHostMemoryInfo(SMemoryInfo_t* HostMemoryInfo); 

/** OCATBHOST_JVMINFO */
  unsigned int Sget_OcstbHostJVMInfo(SJVMinfo_t* HostJVMInfo);

/** HOSTQpskObjects*/ 
unsigned int Sget_ocStbHostQpskObjects(ocStbHostQpsk_t* QpskObj);
/** /** ocStbHostSnmpProxyInfo Card Info */
unsigned int Sget_ocStbHostSnmpProxyCardInfo(SocStbHostSnmpProxy_CardInfo_t *ProxyCardInfo);
/** SYSTEM INFO*/

/** ocStbHostDVIHDMI3DCompatibilityControl */
unsigned int Sset_ocStbHostDVIHDMI3DCompatibilityControl(long eCompatibilityType);
unsigned int Sset_ocStbHostDVIHDMI3DCompatibilityMsgDisplay(long eStatus);
  
/** Carousal_Info*/
unsigned int Sget_ocStbHostOobCarouselTimeoutCount(int * pTimeoutCount);
unsigned int Sget_ocStbHostInbandCarouselTimeoutCount(int * pTimeoutCount);
unsigned int Sget_ocStbHostPatPmtTimeoutCount(unsigned long *pat_timeout, unsigned long *pmt_timeout);

extern "C" unsigned int Set_ocStbHostSwApplicationInfoTable(int nApps, SNMPHOSTSoftwareApplicationInfoTable_t * pAppInfolist);
extern "C" unsigned int Set_ocStbHostJvmInfo(SJVMinfo_t * pJvmInfo);

//VL_HOST_SNMP_API_RESULT Sget_SystemInfo(VL_SNMP_SYSTEM_VAR_TYPE systype, void *_Systeminfo);



#endif //__VL_GET_HOSTDATA_MIB_H

/**
 * @}
 */



