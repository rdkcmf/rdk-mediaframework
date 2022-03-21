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
#include "persistentconfig.h"
#include "utils.h"
#include "link.h"
#include "transport.h"
#include "session.h"
#include "podhighapi.h"
#include "appmgr.h"
#include "applitest.h"

#include "diag-main.h"
#include "hal_diag.h"

#include "ip_types.h"
#include "xchanResApi.h"

#ifndef SNMP_IPC_CLIENT
#include "vlSnmpEcmApi.h"
#include "vlSnmpHostInfo.h"
#endif

#include <rdk_debug.h>
#include "rmf_osal_mem.h"
#include "rmf_osal_util.h"
#include "hostInfo.h"

#if USE_SYSRES_MLT
#include "mlt_malloc.h"
#endif

#ifdef GLI
#include "libIBus.h"
#include "sysMgr.h"
#endif

#define __MTAG__ VL_CARD_MANAGER


extern unsigned char * SysCtlGetHWVerId(void);
extern unsigned char * SysCtlGetVendorId(void);
extern int cdl_group_id(uint8 *pData, uint32 dataLen);

static Diag_status_e GetDiagStatus(HAL_DIAG_API_STATUS Ret)
{
    Diag_status_e status;
    switch(Ret)
    {
    case HAL_DIAG_API_STATUS_SUCCESS:
        status = DIAG_GRANTED;
        break;
    case HAL_DIAG_API_STATUS_FAILURE:
        status = DIAG_OTHER_REASON;
        break;
    case HAL_DIAG_API_STATUS_NOT_SUPPORTED:
        status = DIAG_NOT_IMPLEMENTED;
        break;
    case HAL_DIAG_API_STATUS_NULL_PARAMETER:
        status = DIAG_OTHER_REASON;
        break;
    case HAL_DIAG_API_STATUS_NOT_IMPLEMENTED:
        status = DIAG_NOT_IMPLEMENTED;
        break;
    default:
        status = DIAG_OTHER_REASON;
        break;
    }
    return status;
}

unsigned char memory_report(unsigned char *pData,unsigned long *pSize)
{
    Diag_status_e status = DIAG_OTHER_REASON;
    HAL_DIAG_API_STATUS Ret = HAL_DIAG_API_STATUS_SUCCESS;
    sHostMemoryAllocInfo    HostMemoryAllocInfo = {0};
    int ii = 0;
    unsigned long numBytes = 0;

    if(pData == NULL )
    {
        return (unsigned char)status;
    }
#ifdef USE_HAL_DIAG
    Ret = HAL_DIAG_GetHostMemInfo(&HostMemoryAllocInfo);
#endif // USE_HAL_DIAG

    status = GetDiagStatus(Ret);

    if(status != DIAG_GRANTED)
    {
        return (unsigned char)status;
    }
    pData[numBytes++] = HostMemoryAllocInfo.numMemTypes;

    for(ii = 0; ii < HostMemoryAllocInfo.numMemTypes; ii++)
    {
        pData[numBytes++] = (unsigned char)HostMemoryAllocInfo.memoryInfo[ii].memType;
        pData[numBytes++] = (unsigned char)((HostMemoryAllocInfo.memoryInfo[ii].memSizeKBytes&0xFF000000) >> 24);
        pData[numBytes++] = (unsigned char)((HostMemoryAllocInfo.memoryInfo[ii].memSizeKBytes&0x00FF0000) >> 16);
        pData[numBytes++] = (unsigned char)((HostMemoryAllocInfo.memoryInfo[ii].memSizeKBytes&0x0000FF00) >> 8);
        pData[numBytes++] = (unsigned char)((HostMemoryAllocInfo.memoryInfo[ii].memSizeKBytes&0xFF));
    }
    *pSize = numBytes;
    return (unsigned char)status;

}
static softwareVerInfo_t    SoftWareVerInfo,*pSftRep;
unsigned char software_ver_report(unsigned char *pData,unsigned long *pSize)
{
    Diag_status_e status = DIAG_OTHER_REASON;
    HAL_DIAG_API_STATUS Ret =  HAL_DIAG_API_STATUS_SUCCESS;
    //softwareVerInfo_t    SoftWareVerInfo,*pSftRep;
    unsigned long numBytes = 0;
    int ii=0,jj=0;

    if(pData == NULL  )
    {
        return (unsigned char)status;
    }

#ifdef USE_HAL_DIAG
    Ret = HAL_DIAG_GetSoftwareVerInfo(&SoftWareVerInfo);
#endif // USE_HAL_DIAG

    status = GetDiagStatus(Ret);

    if(status != DIAG_GRANTED)
    {
        return (unsigned char)status;
    }
    pData[numBytes++] = SoftWareVerInfo.number_of_applications;
    for(ii = 0; ii < SoftWareVerInfo.number_of_applications; ii++)
    {
        pData[numBytes++] = (unsigned char )( (SoftWareVerInfo.SoftVer[ii].application_version_number & 0xFF00) >> 8);
        pData[numBytes++] = (unsigned char ) (SoftWareVerInfo.SoftVer[ii].application_version_number & 0xFF) ;
        pData[numBytes++] = (unsigned char ) (SoftWareVerInfo.SoftVer[ii].application_status_flag );

        pData[numBytes++] = (unsigned char ) (SoftWareVerInfo.SoftVer[ii].application_name_length );
        /*
        for( jj = 0 ; ii < SoftWareVerInfo.SoftVer[ii].application_name_length; jj++)
            pData[numBytes++] = SoftWareVerInfo.SoftVer[ii].application_name_byte[jj];

        pData[numBytes++] = (unsigned char ) (SoftWareVerInfo.SoftVer[ii].application_sign_length );
        for( jj = 0 ; ii < SoftWareVerInfo.SoftVer[ii].application_sign_length; jj++)
            pData[numBytes++] = SoftWareVerInfo.SoftVer[ii].application_sign_byte[jj];
        */

        for( jj = 0 ; jj < SoftWareVerInfo.SoftVer[ii].application_name_length; jj++)
            pData[numBytes++] = SoftWareVerInfo.SoftVer[ii].application_name_byte[jj];

        pData[numBytes++] = (unsigned char ) (SoftWareVerInfo.SoftVer[ii].application_sign_length );
        for( jj = 0 ; jj < SoftWareVerInfo.SoftVer[ii].application_sign_length; jj++)
            pData[numBytes++] = SoftWareVerInfo.SoftVer[ii].application_sign_byte[jj];

    }
    *pSize = numBytes;
    return (unsigned char)status;

}
extern "C" VL_HOST_SNMP_API_RESULT vlSnmpGetHostInfo(VL_SNMP_VAR_TYPE eSnmpVarType, void * _pvSNMPInfo);
unsigned char firmware_ver__report(unsigned char *pData,unsigned long *pSize)
{
    Diag_status_e status = DIAG_OTHER_REASON;
    /*HAL_DIAG_API_STATUS Ret= HAL_DIAG_API_STATUS_SUCCESS;
    sFirmwareVerInfo_t FirmwareVerInfo;
    firmware_ver_report_t *pFirmRep;*/
    unsigned long numBytes = 0;
    char szStackVersion[256] ={0};
    int verlen = 0;
    int day=0,month=0,year=0;
#ifdef USE_FULL_SNMP
    int ii=0;
#endif

    if(pData == NULL)
    {
        return (unsigned char)status;
    }

#ifdef USE_HAL_DIAG
    //Ret = HAL_DIAG_GetFirmwareVerInfo(&FirmwareVerInfo);
#endif // USE_HAL_DIAG

#ifdef USE_FULL_SNMP
    {
        VL_SNMP_VAR_DEVICE_SOFTWARE Softwareobj;

        vlSnmpGetHostInfo(VL_SNMP_VAR_TYPE_ocStbHostDeviceSoftware, &Softwareobj);
		
        char * pszBootFileName = Softwareobj.SoftwareFirmwareImageName;
        if(0 == strlen(pszBootFileName))
        {
            pszBootFileName = Softwareobj.SoftwareFirmwareVersion;
        }
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS", "%s: ##%29s : %29s ##\n", __FUNCTION__, "Software Firmware Version", pszBootFileName);
        memset(szStackVersion, 0, sizeof(szStackVersion));
        strncpy(szStackVersion ,pszBootFileName,sizeof(szStackVersion)-1); 
        verlen = strlen(szStackVersion);
        day   = Softwareobj.nSoftwareFirmwareDay;
        month = Softwareobj.nSoftwareFirmwareMonth;
        year  = Softwareobj.nSoftwareFirmwareYear;
        }
#endif
    status = GetDiagStatus(/*Ret*/HAL_DIAG_API_STATUS_SUCCESS);

    if(status != DIAG_GRANTED)
    {
        return (unsigned char)status;
    }
    pData[numBytes++] =  verlen;//FirmwareVerInfo.firmware_version_length;

#ifdef USE_FULL_SNMP
   for(ii = 0; ii < verlen;/*FirmwareVerInfo.firmware_version_length*/ ii++)
    {
        pData[numBytes++] = szStackVersion[ii];//FirmwareVerInfo.firmware_version_byte[ii];
    }
#endif   
    pData[numBytes++] = (unsigned char)((year & 0xFF00) >> 8);
    pData[numBytes++] = (unsigned char)((year & 0xFF) );
    pData[numBytes++] = month;//FirmwareVerInfo.firmware_month;
    pData[numBytes++] = day;//FirmwareVerInfo.firmware_day;
    *pSize = numBytes;
    return (unsigned char)status;

}


unsigned char MAC_address_report(unsigned char *pData,unsigned long *pSize)
{
    Diag_status_e status = DIAG_OTHER_REASON;
    HAL_DIAG_API_STATUS Ret;
    VL_SNMP_API_RESULT SnmpRet;
    VL_MAC_ADDRESS_STRUCT EcmMacAddress;
    sMacAddrReport    MacAddrReport;
    unsigned long numBytes = 0,ii,jj;
    VL_HOST_IP_INFO HostIpInfo;
    Ret = HAL_DIAG_API_STATUS_SUCCESS;
    if(pData == NULL)
    {
        return (unsigned char)status;
    }

#ifdef USE_HAL_DIAG
    //Ret = HAL_DIAG_GetMACAddressInfo(&MacAddrReport);
#endif // USE_HAL_DIAG
    vlXchanGetDsgIpInfo(&HostIpInfo);
    memset(&EcmMacAddress,0, sizeof(EcmMacAddress));
    SnmpRet = VL_SNMP_API_RESULT_FAILED;
//    #ifdef ENABLE_SNMP
    // MAR-14-2011: Do not use SNMP for eCM MAC address // SnmpRet = vlSnmpEcmGetMacAddress( &EcmMacAddress);
    {
        /*char cmd[1024] = {0};
        snprintf(cmd, sizeof(cmd), "%s", "snmpwalk -OQ -v 2c -c public 192.168.100.1"
		" .1.3.6.1.2.1.2.2.1.6.2 | cut -f2 -d= | "
		"sed 's/:/ /g' > /tmp/ecm_mac.txt");
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: Executing cmd:\n%s \n", __FUNCTION__, cmd);
        system((const char*)cmd);*/

        FILE* fp = popen("PATH=${PATH}:/sbin:/usr/sbin /lib/rdk/getDeviceDetails.sh read ecm_mac", "r");

        if(NULL != fp)
        {
            char * pszRet = fgets(EcmMacAddress.szMacAddress, sizeof(EcmMacAddress.szMacAddress), fp);
            EcmMacAddress.szMacAddress[sizeof(EcmMacAddress.szMacAddress)-1] = '\0';
            char * temp = strchr(EcmMacAddress.szMacAddress,'\n');
            if(temp)
            {
                *temp = '\0';
            }
            
            if(NULL != pszRet)
            {
                 RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "MAC_address_report: Read eCM MAC '%s' address string from /lib/rdk/getDeviceDetails.sh\n",
                    EcmMacAddress.szMacAddress);
            }
            else
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","MAC_address_report: Could not read eCM MAC address string from /lib/rdk/getDeviceDetails.sh'\n");
            }
            int nRet = sscanf(EcmMacAddress.szMacAddress, "%02X %02X %02X %02X %02X %02X",
                EcmMacAddress.aBytMacAddress+0,
                EcmMacAddress.aBytMacAddress+1,
                EcmMacAddress.aBytMacAddress+2,
                EcmMacAddress.aBytMacAddress+3,
                EcmMacAddress.aBytMacAddress+4,
                EcmMacAddress.aBytMacAddress+5);
            
            if(VL_MAC_ADDR_SIZE != nRet)
            {
                 RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "MAC_address_report: Space separated read of '%s' failed. Attempting colon separated read.\n", EcmMacAddress.szMacAddress);
                
                nRet = sscanf(EcmMacAddress.szMacAddress, "%02x:%02x:%02x:%02x:%02x:%02x",
                    EcmMacAddress.aBytMacAddress+0,
                    EcmMacAddress.aBytMacAddress+1,
                    EcmMacAddress.aBytMacAddress+2,
                    EcmMacAddress.aBytMacAddress+3,
                    EcmMacAddress.aBytMacAddress+4,
                    EcmMacAddress.aBytMacAddress+5);
            }
            if(VL_MAC_ADDR_SIZE == nRet)
            {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","MAC_address_report: Read eCM MAC '%02x:%02x:%02x:%02x:%02x:%02x' address bytes from /lib/rdk/getDeviceDetails.sh\n",
                    EcmMacAddress.aBytMacAddress[0],
                    EcmMacAddress.aBytMacAddress[1],
                    EcmMacAddress.aBytMacAddress[2],
                    EcmMacAddress.aBytMacAddress[3],
                    EcmMacAddress.aBytMacAddress[4],
                    EcmMacAddress.aBytMacAddress[5]);
                SnmpRet = VL_SNMP_API_RESULT_SUCCESS;
#ifdef GLI
		IARM_Bus_SYSMgr_EventData_t eventData;
		eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_ECM_MAC;
		eventData.data.systemStates.state = 1;
		eventData.data.systemStates.error = 0;
		snprintf(eventData.data.systemStates.payload,
			sizeof(eventData.data.systemStates.payload),
			"%02x:%02x:%02x:%02x:%02x:%02x",
			EcmMacAddress.aBytMacAddress[0],
			EcmMacAddress.aBytMacAddress[1],
			EcmMacAddress.aBytMacAddress[2],
			EcmMacAddress.aBytMacAddress[3],
			EcmMacAddress.aBytMacAddress[4],
			EcmMacAddress.aBytMacAddress[5]);
			IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME,
				(IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE,
				(void *)&eventData, sizeof(eventData));
#endif
            }
            else
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","MAC_address_report: Could not read eCM MAC address bytes from /lib/rdk/getDeviceDetails.sh\n");
            }
            pclose(fp);
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","MAC_address_report: Could not start /lib/rdk/getDeviceDetails.sh\n");
        }
    }
//    #else
//    SnmpRet = VL_SNMP_API_RESULT_FAILED;
//    #endif

     if(VL_SNMP_API_RESULT_SUCCESS != SnmpRet)
     {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","MAC_address_report: vlSnmpEcmGetMacAddress returned Error:%d \n",SnmpRet);
        Ret =  HAL_DIAG_API_STATUS_FAILURE;
     }

    status = GetDiagStatus(Ret );

    if(status != DIAG_GRANTED)
    {
        return (unsigned char)status;
    }
    pData[numBytes++] = 2;//MacAddrReport.numMACAddresses;

    {
        pData[numBytes++] = MAC_ADDRESS_TYPE_HOST;//(unsigned char)MacAddrReport.macAddrInfo[ii].macAddrType;
        pData[numBytes++] = 6;//(unsigned char)MacAddrReport.macAddrInfo[ii].macAddrLen;

        for(jj=0; jj < 6;/*MacAddrReport.macAddrInfo[ii].macAddrLen;*/ jj++)
            pData[numBytes++] = (unsigned char)HostIpInfo.aBytMacAddress[jj];//MacAddrReport.macAddrInfo[ii].macAddr[jj];
    }
    {
        pData[numBytes++] = MAC_ADDRESS_TYPE_DOCSIS;//(unsigned char)MacAddrReport.macAddrInfo[ii].macAddrType;
        pData[numBytes++] = 6;//(unsigned char)MacAddrReport.macAddrInfo[ii].macAddrLen;

        for(jj=0; jj < VL_MAC_ADDR_SIZE;/*MacAddrReport.macAddrInfo[ii].macAddrLen;*/ jj++)
            pData[numBytes++] = (unsigned char)EcmMacAddress.aBytMacAddress[jj];//MacAddrReport.macAddrInfo[ii].macAddr[jj];
    }

    *pSize = numBytes;
    return (unsigned char)status;

}


unsigned char FAT_status_report(unsigned char *pData,unsigned long *pSize,unsigned char ltsid)
{
    Diag_status_e status = DIAG_OTHER_REASON;
    HAL_DIAG_API_STATUS Ret = HAL_DIAG_API_STATUS_SUCCESS;
    sFATInfo    FATInfo = {0};
    int ii = 0;
    unsigned long numBytes = 0;

    if(pData == NULL)
    {
        return (unsigned char)status;
    }

#ifdef USE_HAL_DIAG
    Ret = HAL_DIAG_GetFATInfo(ltsid,&FATInfo);
#endif // USE_HAL_DIAG

    status = GetDiagStatus(Ret);

    if(status != DIAG_GRANTED)
    {
        return (unsigned char)status;
    }
    pData[numBytes++] =  (0xF0 | ((FATInfo.PCR_lock & 0x1) << 3) | ((FATInfo.modulation_mode & 0x3) << 1) | ( FATInfo.carrier_lock_status & 0x1) );
    pData[numBytes++] =  (unsigned char)((FATInfo.SNR & 0xFF00) >> 8);
    pData[numBytes++] =  (unsigned char)(FATInfo.SNR & 0xFF);
    pData[numBytes++] =  (unsigned char)((FATInfo.signal_level & 0xFF00) >> 8);
    pData[numBytes++] =  (unsigned char)(FATInfo.signal_level & 0xFF);

    *pSize = numBytes;
    return (unsigned char)status;

}
unsigned char FDC_status_report(unsigned char *pData,unsigned long *pSize)
{
    Diag_status_e status = DIAG_OTHER_REASON;
    HAL_DIAG_API_STATUS Ret = HAL_DIAG_API_STATUS_SUCCESS;
    sFDCInfo    FDCInfo = {0};
    unsigned long numBytes = 0;


    if(pData == NULL)
    {
        return (unsigned char)status;
    }

#ifdef USE_HAL_DIAG
    Ret = HAL_DIAG_GetFDCInfo(&FDCInfo);
#endif // USE_HAL_DIAG

    status = GetDiagStatus(Ret);

    if(status != DIAG_GRANTED)
    {
        return (unsigned char)status;
    }
    pData[numBytes++] =  (unsigned char )( (FDCInfo.FDC_center_freq & 0xFF00) >> 8);
    pData[numBytes++] =  (unsigned char )( (FDCInfo.FDC_center_freq & 0xFF) );
    pData[numBytes++] =  (unsigned char )( 0xFC | ((FDCInfo.carrier_lock_status & 0x1) << 1) );

    *pSize = numBytes;
    return (unsigned char)status;

}
unsigned char current_channel_report(unsigned char *pData,unsigned long *pSize)
{
    Diag_status_e status = DIAG_OTHER_REASON;
    HAL_DIAG_API_STATUS Ret = HAL_DIAG_API_STATUS_SUCCESS;
    sCurrentChannelInfo_t    CurrentChannelInfo = {0};
    int ii;
    unsigned long numBytes = 0;

    if(pData == NULL)
    {
        return (unsigned char)status;
    }

#ifdef USE_HAL_DIAG
    Ret = HAL_DIAG_GetCurrentChannelInfo(&CurrentChannelInfo);
#endif // USE_HAL_DIAG

    status = GetDiagStatus(Ret);

    if(status != DIAG_GRANTED)
    {
        return (unsigned char)status;
    }

    pData[numBytes++] = (unsigned char)( 0xC0 | ((CurrentChannelInfo.channel_type & 0x1) << 5) | ( (CurrentChannelInfo.authorization_flag & 1) << 4) | ((CurrentChannelInfo.purchasable_flag & 1) << 3) | ( (CurrentChannelInfo.purchased_flag & 1) << 2) | ( (CurrentChannelInfo.preview_flag & 1) << 1) |
    (CurrentChannelInfo.preview_flag & 1) );

    pData[numBytes++] = (unsigned char) ( (CurrentChannelInfo.current_channel & 0xFF00) >> 8 );
    pData[numBytes++] = (unsigned char) ( (CurrentChannelInfo.current_channel & 0xFF)  );


    *pSize = numBytes;
    return (unsigned char)status;

}

unsigned char i1394_port_report(unsigned char *pData,unsigned long *pSize)
{
    Diag_status_e status = DIAG_OTHER_REASON;
    HAL_DIAG_API_STATUS Ret = HAL_DIAG_API_STATUS_SUCCESS;
    sI1394PortInfo    i1394PortInfo = {0};
    int ii = 0;
    unsigned long numBytes = 0;

    if(pData == NULL)
    {
        return (unsigned char)status;
    }

#ifdef USE_HAL_DIAG
    Ret = HAL_DIAG_GetI1394PortInfo(&i1394PortInfo);
#endif // USE_HAL_DIAG

    status = GetDiagStatus(Ret);

    if(status != DIAG_GRANTED)
    {
        return (unsigned char)status;
    }


    pData[numBytes++] =  ( 0xC0 | ((i1394PortInfo.loop_status & 1) << 5) | ((i1394PortInfo.root_status & 1) << 4) |
                      ((i1394PortInfo.cycle_master_status & 1) << 3) | ((i1394PortInfo.host_a_d_source_selection_status & 1) << 2) | ((i1394PortInfo.port_1_connection_status & 1) << 1) | (i1394PortInfo.port_2_connection_status & 1)   );

    pData[numBytes++] = (unsigned char )((i1394PortInfo.total_number_of_nodes & 0xFF00) >> 8 );
    pData[numBytes++] = (unsigned char )(i1394PortInfo.total_number_of_nodes & 0xFF);
    pData[numBytes++] = i1394PortInfo.number_of_connected_devices;

      for( ii = 0; ii < i1394PortInfo.number_of_connected_devices; ii++)
    {
        pData[numBytes++] = ((i1394PortInfo.DeviceInfo[ii].device_subunit_type & 0x1F) << 3) | ((i1394PortInfo.DeviceInfo[ii].device_a_d_source_selection_status & 1) << 2) | 0x3;

        pData[numBytes++] = (unsigned char ) ((i1394PortInfo.DeviceInfo[ii].eui_64_hi & 0xFF000000) >> 24);
        pData[numBytes++] = (unsigned char ) ((i1394PortInfo.DeviceInfo[ii].eui_64_hi & 0xFF0000) >> 16);
        pData[numBytes++] = (unsigned char ) ((i1394PortInfo.DeviceInfo[ii].eui_64_hi & 0xFF00) >> 8);
        pData[numBytes++] = (unsigned char ) (i1394PortInfo.DeviceInfo[ii].eui_64_hi & 0xFF);

        pData[numBytes++] = (unsigned char ) ((i1394PortInfo.DeviceInfo[ii].eui_64_lo & 0xFF000000) >> 24);
        pData[numBytes++] = (unsigned char ) ((i1394PortInfo.DeviceInfo[ii].eui_64_lo & 0xFF0000) >> 16);
        pData[numBytes++] = (unsigned char ) ((i1394PortInfo.DeviceInfo[ii].eui_64_lo & 0xFF00) >> 8);
        pData[numBytes++] = (unsigned char ) (i1394PortInfo.DeviceInfo[ii].eui_64_lo & 0xFF);
    }
    *pSize = numBytes;
    return (unsigned char)status;
}





unsigned char DVI_status_report(unsigned char *pData,unsigned long *pSize)
{
    Diag_status_e status = DIAG_OTHER_REASON;
    HAL_DIAG_API_STATUS Ret = HAL_DIAG_API_STATUS_SUCCESS;
    DVIInfo_t    DVIInfo = {0};
    unsigned long numBytes = 0;

    if(pData == NULL)
    {
        return (unsigned char)status;
    }

#ifdef USE_HAL_DIAG
    Ret = HAL_DIAG_GetDviInfo(&DVIInfo);


    status = GetDiagStatus(Ret);

    if(status != DIAG_GRANTED)
    {
        return (unsigned char)status;
    }

#endif // USE_HAL_DIAG

    pData[numBytes++] = (unsigned char )(0xE0 | ( (DVIInfo.connection_status & 0x3) << 4) | ( (DVIInfo.host_HDCP_status & 0x1) << 2)  | ( (DVIInfo.device_HDCP_status & 0x3) << 1) );

    pData[numBytes++] = (unsigned char )( (DVIInfo.horizontal_lines & 0xFF00) >> 8);
    pData[numBytes++] = (unsigned char )( (DVIInfo.horizontal_lines & 0xFF) );

    pData[numBytes++] = (unsigned char )( (DVIInfo.vertical_lines & 0xFF00) >> 8);
    pData[numBytes++] = (unsigned char )( (DVIInfo.vertical_lines & 0xFF) );

    pData[numBytes++] = DVIInfo.frame_rate;

    pData[numBytes++] = ((DVIInfo.aspect_ratio & 0x3) << 6) | ((DVIInfo.aspect_ratio & 0x1) << 5) | 0x1F;

    *pSize = numBytes;
    return (unsigned char)status;

}
unsigned char eCM_status_report(unsigned char *pData,unsigned long *pSize)
{
    Diag_status_e status = DIAG_OTHER_REASON;
    HAL_DIAG_API_STATUS Ret;
    s_eCMStatusInfo    eCMStatusInfo;
    unsigned long numBytes = 0;
    VL_SNMP_API_RESULT SnmpRet = VL_SNMP_API_RESULT_INVALID;
    unsigned long DsFreq = 0, DsPower = 0, DsCarrierLockStatus = 0;
    unsigned long UsFreq =0, UsPower = 0, UsCarrierLockStatus = 0;

#ifndef SNMP_IPC_CLIENT
    VL_SNMP_ECM_DS_MODULATION_TYPE eDsModType;
    VL_SNMP_ECM_US_MODULATION_TYPE eUsModType;
#endif //SNMP_IPC_CLIENT
    unsigned char channel_s_cdma_status = 0;
    unsigned char upstream_symbol_rate = 0;


    Ret = HAL_DIAG_API_STATUS_SUCCESS;

    if(pData == NULL)
    {
        return (unsigned char)status;
    }

#ifdef USE_HAL_DIAG
    //Ret = HAL_DIAG_GeteCMStatusInfo(&eCMStatusInfo);
#endif // USE_HAL_DIAG
//    #ifdef ENABLE_SNMP
#ifndef SNMP_IPC_CLIENT
     SnmpRet = vlSnmpEcmGetDsFrequency(&DsFreq);
      if(VL_SNMP_API_RESULT_SUCCESS != SnmpRet)
      {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","eCM_status_report: vlSnmpEcmGetDsFrequency returned Error:%d \n",SnmpRet);
      }
     SnmpRet = vlSnmpEcmGetDsPower(&DsPower);
    if(VL_SNMP_API_RESULT_SUCCESS != SnmpRet)
      {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","eCM_status_report: vlSnmpEcmGetDsPower returned Error:%d \n",SnmpRet);
      }
     SnmpRet = vlSnmpEcmGetDsModulationType(&eDsModType);
    if(VL_SNMP_API_RESULT_SUCCESS != SnmpRet)
      {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","eCM_status_report: vlSnmpEcmGetDsModulationType returned Error:%d \n",SnmpRet);
      }
     SnmpRet = vlSnmpEcmGetDsLockStatus(&DsCarrierLockStatus);
    if(VL_SNMP_API_RESULT_SUCCESS != SnmpRet)
      {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","eCM_status_report: vlSnmpEcmGetDsLockStatus returned Error:%d \n",SnmpRet);
      }



     SnmpRet = vlSnmpEcmGetUsModulationType( &eUsModType);
    if(VL_SNMP_API_RESULT_SUCCESS != SnmpRet)
      {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","eCM_status_report: vlSnmpEcmGetUsModulationType returned Error:%d \n",SnmpRet);
      }
     SnmpRet = vlSnmpEcmGetUsFrequency(&UsFreq);
    if(VL_SNMP_API_RESULT_SUCCESS != SnmpRet)
      {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","eCM_status_report: vlSnmpEcmGetUsFrequency returned Error:%d \n",SnmpRet);
      }
     SnmpRet = vlSnmpEcmGetUsPower(&UsPower);
    if(VL_SNMP_API_RESULT_SUCCESS != SnmpRet)
      {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","eCM_status_report: vlSnmpEcmGetUsPower returned Error:%d \n",SnmpRet);
      }
    #else
    SnmpRet = VL_SNMP_API_RESULT_FAILED;
    #endif
//#endif //SNMP_IPC_CLIENT
    status = GetDiagStatus(Ret);

    if(status != DIAG_GRANTED)
    {
        return (unsigned char)status;
    }
    DsFreq = DsFreq/(1000*1000);
    pData[numBytes++] = (unsigned char)( (DsFreq & 0xFF00) >> 8);
    pData[numBytes++] = (unsigned char) (DsFreq & 0xFF);

    pData[numBytes++] = (unsigned char)( (DsPower & 0xFF00) >> 8);
    pData[numBytes++] = (unsigned char) (DsPower & 0xFF);
#ifndef SNMP_IPC_CLIENT
    pData[numBytes++] = (unsigned char) ( ((DsCarrierLockStatus & 0x1) << 7) | 0x60 |((channel_s_cdma_status & 0x3) << 3) |(eUsModType & 0x7) ) ;
#endif //SNMP_IPC_CLIENT
    UsFreq = UsFreq/(1000*1000);
    pData[numBytes++] = (unsigned char)( (UsFreq & 0xFF00) >> 8);
    pData[numBytes++] = (unsigned char) (UsFreq & 0xFF);

    pData[numBytes++] = (unsigned char)( (UsPower & 0xFF00) >> 8);
    pData[numBytes++] = (unsigned char) (UsPower & 0xFF);

    pData[numBytes++] = (unsigned char) (upstream_symbol_rate);

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","eCM_status_report: Number of bytes:%d \n",numBytes);
    *pSize = numBytes;
    return (unsigned char)status;

}




unsigned char HDMI_port_status_report(unsigned char *pData,unsigned long *pSize)
{
    Diag_status_e status = DIAG_OTHER_REASON;
    HAL_DIAG_API_STATUS Ret = HAL_DIAG_API_STATUS_SUCCESS;
    s_HDMIPortStatus    HDMIPortStatus = {0};
    unsigned long numBytes = 0;

    if(pData == NULL )
    {
        return (unsigned char)status;
    }

#ifdef USE_HAL_DIAG
    Ret = HAL_DIAG_GetHDMIPortInfo(&HDMIPortStatus);
#endif // USE_HAL_DIAG

    status = GetDiagStatus(Ret);

    if(status != DIAG_GRANTED)
    {
        return (unsigned char)status;
    }


    pData[numBytes++] = ( (( HDMIPortStatus.device_type & 0x1) << 7) | (( HDMIPortStatus.color_space & 0x3) << 5) | (( HDMIPortStatus.connection_status & 0x3) << 3) | (( HDMIPortStatus.host_HDCP_status & 0x1) << 2) | (( HDMIPortStatus.device_HDCP_status & 0x3) << 1) );

    pData[numBytes++] = ( ( HDMIPortStatus.horizontal_lines & 0xFF00 ) >> 8 );
    pData[numBytes++] = ( HDMIPortStatus.horizontal_lines & 0xFF );
    pData[numBytes++] = ( ( HDMIPortStatus.vertical_lines & 0xFF00 ) >> 8 );
    pData[numBytes++] = ( HDMIPortStatus.vertical_lines & 0xFF );

    pData[numBytes++] = HDMIPortStatus.frame_rate;

    pData[numBytes++] = ((HDMIPortStatus.aspect_ratio & 0x3) << 6) | ((HDMIPortStatus.prog_inter_type & 0x1) << 5) | 0x1F;

    pData[numBytes++] = (( HDMIPortStatus.audio_sample_size & 0x3) << 6)| (( HDMIPortStatus.audio_format & 0x7) << 3) | ( HDMIPortStatus.audio_sample_freq &0x7);

    *pSize = numBytes;
    return (unsigned char)status;

}
unsigned char RDC_status_report(unsigned char *pData,unsigned long *pSize)
{
    Diag_status_e status = DIAG_OTHER_REASON;
    HAL_DIAG_API_STATUS Ret = HAL_DIAG_API_STATUS_SUCCESS;
    sRDCInfo    RDCInfo = {0};
    unsigned long numBytes = 0;

    if(pData == NULL)
    {
        return (unsigned char)status;
    }


#ifdef USE_HAL_DIAG
    Ret = HAL_DIAG_GetRDCInfo(&RDCInfo);
#endif // USE_HAL_DIAG

    status = GetDiagStatus(Ret);

    if(status != DIAG_GRANTED)
    {
        return (unsigned char)status;
    }
    pData[numBytes++] = (unsigned char)((RDCInfo.RDC_center_freq&0xFF00) >> 8);
    pData[numBytes++] = (unsigned char)(RDCInfo.RDC_center_freq&0xFF);
    pData[numBytes++] = (unsigned char)(RDCInfo.RDC_transmitter_power_level);

    pData[numBytes++] = (unsigned char)( 0xFC | (RDCInfo.RDC_data_rate & 0x3)   );

    *pSize = numBytes;
    return (unsigned char)status;

}



unsigned char net_address_report(unsigned char *pData,unsigned long *pSize)
{
    Diag_status_e status = DIAG_OTHER_REASON;
    HAL_DIAG_API_STATUS Ret;
    sOCHD2NetAddressInfo    OCHD2NetAddressInfo;
    unsigned long numBytes = 0,ii,jj;
    unsigned long HostIpAddSizeBytes =0, EcmIpAddSizeBytes = 0;
    VL_HOST_IP_INFO HostIpInfo;
    VL_SNMP_API_RESULT SnmpRet = VL_SNMP_API_RESULT_INVALID;
    VL_IP_ADDRESS_STRUCT IpAddress;
    int iHostTypeError = 0,iEcmTypeError = 0;
    //int iHostRetError = 0,iEcmRetError = 0;
    int iEcmRetError = 0;
    int numAddress = 2;
    Ret = HAL_DIAG_API_STATUS_SUCCESS;

    if(pData == NULL )
    {
        return (unsigned char)status;
    }
    memset(&IpAddress, 0, sizeof(IpAddress));
    memset(&OCHD2NetAddressInfo, 0, sizeof(OCHD2NetAddressInfo));

#ifdef USE_HAL_DIAG
    //Ret = HAL_DIAG_GetOCHD2PortInfo(&OCHD2NetAddressInfo);
#endif // USE_HAL_DIAG
   vlXchanGetDsgIpInfo(&HostIpInfo);

    if(HostIpInfo.ipAddressType == VL_XCHAN_IP_ADDRESS_TYPE_IPV6)
    {
        HostIpAddSizeBytes =  16;

    }
    else if(HostIpInfo.ipAddressType == VL_XCHAN_IP_ADDRESS_TYPE_IPV4)
    {
        HostIpAddSizeBytes =  4;

    }
    else
    {
        iHostTypeError = 1;

    }
//    #ifdef ENABLE_SNMP
#ifndef SNMP_IPC_CLIENT
     SnmpRet = vlSnmpEcmGetIpAddress(&IpAddress);
#endif // SNMP_IPC_CLIENT
//    #else
//    SnmpRet = VL_SNMP_API_RESULT_FAILED;
//    #endif

     if(VL_SNMP_API_RESULT_SUCCESS != SnmpRet)
     {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","net_address_report: vlSnmpEcmGetIpAddress returned Error:%d \n",SnmpRet);
        iEcmRetError = 1;
     }
    if(IpAddress.eIpAddressType == VL_XCHAN_IP_ADDRESS_TYPE_IPV6)
    {
        EcmIpAddSizeBytes = 16;

    }
    else if(IpAddress.eIpAddressType == VL_XCHAN_IP_ADDRESS_TYPE_IPV4)
    {
        EcmIpAddSizeBytes = 4;

    }
    else
    {
        iEcmTypeError = 1;

    }

    if( (iEcmTypeError &&  iHostTypeError) || ( iEcmRetError) )
    {
        Ret = HAL_DIAG_API_STATUS_FAILURE;
    }
    status = GetDiagStatus(Ret);
    if(status != DIAG_GRANTED)
    {
        return (unsigned char)status;
    }

    if(iEcmRetError)
        numAddress--;
    

    pData[numBytes++] = numAddress;// OCHD2NetAddressInfo.number_of_addresses;
    //for( ii = 0; ii < OCHD2NetAddressInfo.number_of_addresses; ii++)
    if( (iHostTypeError == 0) )
    {
        unsigned char Buf[16];
        int nBufCapacity = 16;
        pData[numBytes++] = MAC_ADDRESS_TYPE_HOST;//(unsigned char )OCHD2NetAddressInfo.netaddr[ii].net_address_type;
        pData[numBytes++] = (unsigned char )HostIpAddSizeBytes;//(unsigned char )OCHD2NetAddressInfo.netaddr[ii].number_of_bytes_net;

        for(jj = 0; jj < HostIpAddSizeBytes; jj++ )
        {
            if(HostIpInfo.ipAddressType == VL_XCHAN_IP_ADDRESS_TYPE_IPV6)
                pData[numBytes++] = (unsigned char )HostIpInfo.aBytIpV6GlobalAddress[jj];
            else if(HostIpInfo.ipAddressType == VL_XCHAN_IP_ADDRESS_TYPE_IPV4)
                pData[numBytes++] = (unsigned char )HostIpInfo.aBytIpAddress[jj];
        }
        if(HostIpInfo.ipAddressType == VL_XCHAN_IP_ADDRESS_TYPE_IPV6)
        {
            memset(Buf,0,sizeof(Buf));
            if(nBufCapacity != vlXchanGetV6SubnetMaskFromPlen(HostIpInfo.nIpV6GlobalPlen, &Buf[0], nBufCapacity))
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlXchanGetV6SubnetMaskFromPlen Returned Error !!! \n");
            }
        }
        pData[numBytes++] = (unsigned char )HostIpAddSizeBytes;//(unsigned char )OCHD2NetAddressInfo.netaddr[ii].number_of_bytes_subnet;
        for(jj = 0; jj < HostIpAddSizeBytes; jj++ )
        {
            if(HostIpInfo.ipAddressType == VL_XCHAN_IP_ADDRESS_TYPE_IPV6)
            pData[numBytes++] = (unsigned char )Buf[jj];//OCHD2NetAddressInfo.netaddr[ii].sub_net_address_byte[jj];
            else if(HostIpInfo.ipAddressType == VL_XCHAN_IP_ADDRESS_TYPE_IPV4)
                pData[numBytes++] = (unsigned char)HostIpInfo.aBytSubnetMask[jj];
        }
    }

#ifndef SNMP_IPC_CLIENT
    if( (iEcmTypeError == 0) && (iEcmRetError == 0) )
    {
        pData[numBytes++] = MAC_ADDRESS_TYPE_DOCSIS;//(unsigned char )OCHD2NetAddressInfo.netaddr[ii].net_address_type;
        pData[numBytes++] = (unsigned char )EcmIpAddSizeBytes;//(unsigned char )OCHD2NetAddressInfo.netaddr[ii].number_of_bytes_net;
        for(jj = 0; jj < EcmIpAddSizeBytes; jj++ )
        {
            if(HostIpInfo.ipAddressType == VL_XCHAN_IP_ADDRESS_TYPE_IPV6)
                pData[numBytes++] = (unsigned char )IpAddress.aBytIpV6GlobalAddress[jj];
            else if(HostIpInfo.ipAddressType == VL_XCHAN_IP_ADDRESS_TYPE_IPV4)
                pData[numBytes++] = (unsigned char )IpAddress.aBytIpAddress[jj];
        }

        pData[numBytes++] = (unsigned char )EcmIpAddSizeBytes;//(unsigned char )OCHD2NetAddressInfo.netaddr[ii].number_of_bytes_subnet;
        for(jj = 0; jj < EcmIpAddSizeBytes; jj++ )
        {
            if(HostIpInfo.ipAddressType == VL_XCHAN_IP_ADDRESS_TYPE_IPV6)
                pData[numBytes++] = (unsigned char )IpAddress.aBytIpV6SubnetMask[jj];
            else if(HostIpInfo.ipAddressType == VL_XCHAN_IP_ADDRESS_TYPE_IPV4)
                pData[numBytes++] = (unsigned char)IpAddress.aBytSubnetMask[jj];
        }
    }
#endif //SNMP_IPC_CLIENT

    *pSize = numBytes;
    return (unsigned char)status;

}
static HomeNetworkInfo_t    HomeNetworkInfo;
unsigned char home_network_report(unsigned char *pData,unsigned long *pSize)
{
    Diag_status_e status = DIAG_OTHER_REASON;
    HAL_DIAG_API_STATUS Ret = HAL_DIAG_API_STATUS_SUCCESS;
    //HomeNetworkInfo_t    HomeNetworkInfo;
    unsigned long numBytes = 0,ii,jj;


    if(pData == NULL)
    {
        return (unsigned char)status;
    }

#ifdef USE_HAL_DIAG
    Ret = HAL_DIAG_GetHomeNetworkInformation(&HomeNetworkInfo);
#endif // USE_HAL_DIAG

    status = GetDiagStatus(Ret);

    if(status != DIAG_GRANTED)
    {
        return (unsigned char)status;
    }


    pData[numBytes++] = HomeNetworkInfo.max_clients;
    pData[numBytes++] = HomeNetworkInfo.host_DRM_status;
    pData[numBytes++] = HomeNetworkInfo.connected_clients;
    for(ii = 0; ii < HomeNetworkInfo.connected_clients; ii++)
    {
        for(jj = 0 ; jj < 6; jj++)
            pData[numBytes++] = HomeNetworkInfo.Client[ii].client_mac_address[jj];

        pData[numBytes++] = HomeNetworkInfo.Client[ii].number_of_bytes_net;
        for(jj = 0 ; jj < HomeNetworkInfo.Client[ii].number_of_bytes_net; jj++)
            pData[numBytes++] = HomeNetworkInfo.Client[ii].client_IP_address_byte[jj];

        pData[numBytes++] = HomeNetworkInfo.Client[ii].client_DRM_status;
    }

    *pSize = numBytes;
    return (unsigned char)status;

}

unsigned char host_information_report(unsigned char *pData,unsigned long *pSize)
{
    Diag_status_e status = DIAG_OTHER_REASON;
    HAL_DIAG_API_STATUS Ret = HAL_DIAG_API_STATUS_SUCCESS;
    sHostInformation    HostInformation;
    unsigned long numBytes = 0,ii;
    char vendor[128]="";
    char model[128]="";
    char *pszStr = NULL;
    int VenRetErr = 0,ModRetErr =  0;
    int VenLen = 0, ModLen = 0;
    unsigned long RsId;

    Ret = HAL_DIAG_API_STATUS_SUCCESS;

    if(pData == NULL)
    {
        return (unsigned char)status;
    }

#ifdef USE_HAL_DIAG
//    Ret = HAL_DIAG_GetHostInformation(&HostInformation);
#endif // USE_HAL_DIAG

      /* stub */
       memset(vendor, 0, sizeof(vendor));
       strncpy(vendor, pszStr, sizeof(vendor)-1);
       memset(model, 0, sizeof(model));
       strncpy(model, pszStr, sizeof(model)-1); 



    if(vendor)
        VenLen = strlen(vendor);
    if(vendor)
        ModLen = strlen(model);

    pData[numBytes++] = VenLen;// HostInformation.vendor_name_length;
    if(vendor)
    {
        for(ii = 0; ii < VenLen; ii++)
            pData[numBytes++] = vendor[ii];//HostInformation.vendor_name_character[ii];
    }

    pData[numBytes++] = ModLen;//HostInformation.model_name_length;
    if(model)
    {
        for(ii = 0; ii < ModLen; ii++)
            pData[numBytes++] = model[ii];//HostInformation.model_name_character[ii];
    }
    RsId = vldiagGetOpenedRsId();
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Resource id = 0x%08X, type = 0x%08X, version = 0x%08X\n", RsId, ((RsId & 0xFFC0)>>6), (RsId & 0x3F));
    if( ((RsId & 0x3F) == 2/*ver*/) && (((RsId & 0xFFC0)>>6) == 2) )//type 2 and version 2 has different format
    {
        unsigned char *pVendId;
        unsigned char *pHwId;
        unsigned char GroupId[2];
        
        pVendId = SysCtlGetVendorId();
        
        if(pVendId != NULL)
        {
            pData[numBytes++] = pVendId[0];
            pData[numBytes++] = pVendId[1];
            pData[numBytes++] = pVendId[2];
        }
        else
        {
            pData[numBytes++] = 0;
            pData[numBytes++] = 0;
            pData[numBytes++] = 0;
        }

        pHwId = SysCtlGetHWVerId();
        
        if(pHwId != NULL)
        {
            pData[numBytes++] = pHwId[0];
            pData[numBytes++] = pHwId[1];
            pData[numBytes++] = pHwId[2];
            pData[numBytes++] = pHwId[3];
        }
        else
        {
            pData[numBytes++] = 0;
            pData[numBytes++] = 0;
            pData[numBytes++] = 0;
            pData[numBytes++] = 0;
        }
        
        cdl_group_id(GroupId,2);
        pData[numBytes++] = GroupId[0];
        pData[numBytes++] = GroupId[1];
    }
    *pSize = numBytes;
    return (unsigned char)status;

}



int diagSndApdu(USHORT sesnum,uint32 tag, uint32 dataLen, uint8 *data)
{
    uint16 alen;
    uint8 papdu[MAX_APDU_HEADER_BYTES + dataLen];

    //MDEBUG ( DPM_APPINFO, "ai:Called, tag = 0x%x, len = %d \n", tag, dataLen);

    if (buildApdu(papdu, &alen, tag, dataLen, data ) == NULL)
    {
        //MDEBUG (DPM_ERROR, "ERROR:ai: Unable to build apdu.\n");
        return EXIT_FAILURE;
    }

    AM_APDU_FROM_HOST(sesnum, papdu, alen);

    return EXIT_SUCCESS;
}
/*
 *    APDU in: 0x9F, 0xDF, 0x01, "Diagnostic_cnf"
 */
int  APDU_DiagCnf (USHORT sesnum, UCHAR Numdiags,UCHAR* ptr, USHORT Length)
{
    unsigned char tagvalue[3] = {0x9f, 0xDF, 0x01};

     diagSndApdu(sesnum,0x9fDF01, (uint32)Length,ptr);
    return 1;
}


/********************************************************
 *    APDU Functions to handle GENERIC DIAGNOSTIC SUPPORT
 *******************************************************/

/*
 *    APDU in: 0x9F, 0xDF, 0x00, "Diagnostic_req"
 */

 #if 1
int  APDU_DiagReq (USHORT sesnum, UCHAR * pApdu, USHORT len)
{
    uint8 *ptr;
    //uint32 tag = 0;
    uint16 len_field = 0;
    uint8  numdiags,ltsid;
    uint16 ii;
    UCHAR *payload,*pMem;
    unsigned long size;
    unsigned char tagvalue[3] = {0x9f, 0xDF, 0x00};

//    MDEBUG ( DPM_APPINFO, "ai:Called \n");
    if (pApdu == NULL)
    {
        //MDEBUG (DPM_ERROR, "ERROR:diag: No data passed to AppInfoCnf.\n");
        return 1;
    }
	rmf_osal_memAllocP(RMF_OSAL_MEM_POD, 0x1000,(void **)&payload);
	pMem = payload;
    if(payload == NULL)
        return -1;

    ptr = (uint8 *)pApdu;
    // Get / check the tag to make sure we got a good packet
    if (memcmp(ptr, tagvalue, 3) != 0)
    {
        //MDEBUG (DPM_ERROR, "ERROR:diag: Non-matching tag in AppInfoCnf.\n");
        rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pMem);
        return 1;
    }
    ptr += 3;

    if( *ptr == 0x80 )
    {
        uint8 lenbytes;
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," The Length is bigger than 127 \n");
        lenbytes = *ptr & 0x7F;
        if(lenbytes > 2)
        {
        //Error in length.
            rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pMem);
            return -1;
        }
        ptr++;

        if(lenbytes == 2)
        {
            len_field = *((uint16*)ptr);
        }
        else if(lenbytes == 1)
        {
            len_field = *ptr;
        }
        ptr += lenbytes;

    }
    else
    {
        len_field = *ptr;
        ptr++;
    }
    numdiags = *ptr++;
    for( ii = 0; ii < numdiags; ii++)
    {
        int Id;
        unsigned char ret;
        Id = *ptr++;
        ltsid = *ptr++;

        payload[0] = 1;//number of diags
        payload[1] = Id;//diag Id
        payload[2] = ltsid;//ltsid
        payload[3] = 1;//status field needs to be updated

        size = 0;
        switch(Id)
        {
        case DIAG_HOST_MEM_ALLOC://0x00
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ####### DIAG_HOST_MEM_ALLOC #### \n");
            ret = (unsigned char )memory_report(&payload[4],&size);
            break;
        case DIAG_APP_VER_NUM://0x01
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ####### DIAG_APP_VER_NUM #### \n");
            ret = (unsigned char )software_ver_report(&payload[4],&size);
            break;
        case DIAG_FIRM_VER://0x02
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ####### DIAG_FIRM_VER #### \n");
            ret = (unsigned char )firmware_ver__report(&payload[4],&size);
            break;
        case DIAG_MAC_ADDR://0x03
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ####### DIAG_MAC_ADDR #### \n");
            ret = (unsigned char )MAC_address_report(&payload[4],&size);
            break;
        case DIAG_FAT_STATUS://0x04
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ####### DIAG_FAT_STATUS #### \n");
            ret = (unsigned char )FAT_status_report(&payload[4],&size,ltsid);
            break;
        case DIAG_FDC_STATUS://0x05
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ####### DIAG_FDC_STATUS #### \n");
            ret = (unsigned char )FDC_status_report(&payload[4],&size);
            break;
        case DIAG_CUR_CHNL_REP://0x06
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ####### DIAG_CUR_CHNL_REP #### \n");
            ret = (unsigned char )current_channel_report(&payload[4],&size);
            break;
        case DIAG_1394_PORT://0x07
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ####### DIAG_1394_PORT #### \n");
            ret = (unsigned char )i1394_port_report(&payload[4],&size);
            break;
        case DIAG_DVI_STARUS://0x08
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ####### DIAG_DVI_STARUS #### \n");
            ret = (unsigned char )DVI_status_report(&payload[4],&size);
            break;
        case DIAG_ECM://0x09
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ####### DIAG_ECM #### \n");
            ret = (unsigned char )eCM_status_report(&payload[4],&size);
            break;
        case DIAG_HDMI_PORT_STATUS://0x0a
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ####### DIAG_HDMI_PORT_STATUS #### \n");
            ret = (unsigned char )HDMI_port_status_report(&payload[4],&size);
            break;
        case DIAG_RDC_STATUS://0x0b
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ####### DIAG_RDC_STATUS #### \n");
            ret = (unsigned char )RDC_status_report(&payload[4],&size);
            break;
        case DIAG_OCHD2_NET_STATUS://0x0c
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ####### DIAG_OCHD2_NET_STATUS #### \n");
            ret = (unsigned char )net_address_report(&payload[4],&size);
            break;
        case DIAG_HOME_NET_STATUS://0x0d
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ####### DIAG_HOME_NET_STATUS #### \n");
            ret = (unsigned char )home_network_report(&payload[4],&size);
            break;
        case DIAG_HOST_INFO://0x0e
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ####### DIAG_HOST_INFO #### \n");
            ret = (unsigned char )host_information_report(&payload[4],&size);
            break;
        default:
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD"," ### Error in Diag ID :%d \n",Id);
			rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pMem);
            return -1;
        }//switch

        payload[3] = ret;// status data returned

        //Send The CNF from here...
        {
            APDU_DiagCnf(sesnum,1,payload,4 + size);
        }

    }//for

    rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pMem);
    return 1;
}
#endif
