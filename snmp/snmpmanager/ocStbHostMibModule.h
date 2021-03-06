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


/****************************************************************************

    File: ocStbHostMibModule.h
    Description: Definition of the ocStbHostMibModuel modules SNMP resource.

    Revision History:
    $URL:
    $LastChangedRevision:
    $LastChangedDate: 2007-12-03
    $Author: $

****************************************************************************/

#ifndef OCSTBHOSTMIBMODULE_H
#define OCSTBHOSTMIBMODULE_H

/*
 * Note: this file originally auto-generated by mib2c using
 *  : mib2c.table_data.conf 15999 2007-03-25 22:32:02Z dts12 $
 */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>


#define CHRMAX 256
#define MAX_IOD_LENGTH 128

/** These driverplugeIndex will act as a AVindex */

#define DRIVERPLUGEINDEX_ONE 0x0
#define DRIVERPLUGEINDEX_TWO 0x1

//#include "ifTable.h"
//#include "ipNetToPhysicalTable.h"
//#include "system.h"
//#include "vividlogicmib.h"
//#include "vl_ocStbHost_GetData.h"

/*
 * function declarations
 */

void
init_ocStbHostMibModule(void); // contains all the table dec


/*scalar handle fun dec */
#if 1

Netsnmp_Node_Handler handle_ocStbHostSerialNumber;
Netsnmp_Node_Handler handle_ocStbHostHostID;
Netsnmp_Node_Handler handle_ocStbHostCapabilities;
Netsnmp_Node_Handler handle_ocStbHostAvcSupport;
Netsnmp_Node_Handler handle_ocStbHostQpskFDCFreq;
Netsnmp_Node_Handler handle_ocStbHostQpskRDCFreq;
Netsnmp_Node_Handler handle_ocStbHostQpskFDCBer;
Netsnmp_Node_Handler handle_ocStbHostQpskFDCStatus;
Netsnmp_Node_Handler handle_ocStbHostQpskFDCBytesRead;
Netsnmp_Node_Handler handle_ocStbHostQpskFDCPower;
Netsnmp_Node_Handler handle_ocStbHostQpskFDCLockedTime;
Netsnmp_Node_Handler handle_ocStbHostQpskFDCSNR;
Netsnmp_Node_Handler handle_ocStbHostQpskAGC;
Netsnmp_Node_Handler handle_ocStbHostQpskRDCPower;
Netsnmp_Node_Handler handle_ocStbHostQpskRDCDataRate;
Netsnmp_Node_Handler handle_ocStbEasMessageStateCode;
Netsnmp_Node_Handler handle_ocStbEasMessageCountyCode;
Netsnmp_Node_Handler handle_ocStbEasMessageCountySubdivisionCode;
Netsnmp_Node_Handler handle_ocStbHostSecurityIdentifier;
Netsnmp_Node_Handler handle_ocStbHostCASystemIdentifier;
Netsnmp_Node_Handler handle_ocStbHostCAType;
Netsnmp_Node_Handler handle_ocStbHostSoftwareFirmwareVersion;
Netsnmp_Node_Handler handle_ocStbHostSoftwareOCAPVersion;
Netsnmp_Node_Handler handle_ocStbHostSoftwareFirmwareReleaseDate;
Netsnmp_Node_Handler handle_ocStbHostFirmwareImageStatus;
Netsnmp_Node_Handler handle_ocStbHostFirmwareCodeDownloadStatus;
Netsnmp_Node_Handler handle_ocStbHostFirmwareCodeObjectName;
Netsnmp_Node_Handler handle_ocStbHostFirmwareDownloadFailedStatus;
Netsnmp_Node_Handler handle_ocStbHostFirmwareDownloadFailedCount;   
Netsnmp_Node_Handler handle_ocStbHostPowerStatus;
Netsnmp_Node_Handler handle_ocStbHostAcOutletStatus;
Netsnmp_Node_Handler handle_ocStbHostUserSettingsPreferedLanguage;
Netsnmp_Node_Handler handle_ocStbHostCardMacAddress;
Netsnmp_Node_Handler handle_ocStbHostCardIpAddressType;
Netsnmp_Node_Handler handle_ocStbHostCardIpAddress;
Netsnmp_Node_Handler handle_ocStbHostIpAddressType;
Netsnmp_Node_Handler handle_ocStbHostIpSubNetMask;
Netsnmp_Node_Handler handle_ocStbHostOobMessageMode;
Netsnmp_Node_Handler handle_ocStbHostBootStatus;
Netsnmp_Node_Handler handle_ocStbHostRebootType;
Netsnmp_Node_Handler handle_ocStbHostRebootReset;
Netsnmp_Node_Handler handle_ocStbHostRebootIpModeTlvChange;
Netsnmp_Node_Handler handle_ocStbHostLargestAvailableBlock;
Netsnmp_Node_Handler handle_ocStbHostTotalVideoMemory;
Netsnmp_Node_Handler handle_ocStbHostAvailableVideoMemory;
Netsnmp_Node_Handler handle_ocStbHostTotalSystemMemory;
Netsnmp_Node_Handler handle_ocStbHostAvailableSystemMemory;
Netsnmp_Node_Handler handle_ocStbHostJVMHeapSize;
Netsnmp_Node_Handler handle_ocStbHostJVMAvailHeap;
Netsnmp_Node_Handler handle_ocStbHostJVMLiveObjects;
Netsnmp_Node_Handler handle_ocStbHostJVMDeadObjects;
/** update OCSTB2.x list of scalar items*/
 /*ocStbHostCardBindingStatus module */
Netsnmp_Node_Handler handle_ocStbHostCardBindingStatus;
/* ocStbHostCard modules */
Netsnmp_Node_Handler handle_ocStbHostCardId;
Netsnmp_Node_Handler handle_ocStbHostCardMfgId;
Netsnmp_Node_Handler handle_ocStbHostCardRootOid;
Netsnmp_Node_Handler handle_ocStbHostCardSerialNumber;
Netsnmp_Node_Handler handle_ocStbHostCardSnmpAccessControl;
Netsnmp_Node_Handler handle_ocStbHostCardVersion;
Netsnmp_Node_Handler handle_ocStbHostCardBindingStatus;
Netsnmp_Node_Handler handle_ocStbHostCardOpenedGenericResource;
Netsnmp_Node_Handler handle_ocStbHostCardTimeZoneOffset;
Netsnmp_Node_Handler handle_ocStbHostCardDaylightSavingsTimeDelta;
Netsnmp_Node_Handler handle_ocStbHostCardDaylightSavingsTimeEntry;
Netsnmp_Node_Handler handle_ocStbHostCardDaylightSavingsTimeExit;
Netsnmp_Node_Handler handle_ocStbHostCardEaLocationCode;
Netsnmp_Node_Handler handle_ocStbHostCardVctId;
Netsnmp_Node_Handler handle_ocStbHostCardCpAuthKeyStatus;
Netsnmp_Node_Handler handle_ocStbHostCardCpCertificateCheck;
Netsnmp_Node_Handler handle_ocStbHostCardCpCciChallengeCount;
Netsnmp_Node_Handler handle_ocStbHostCardCpKeyGenerationReqCount;
Netsnmp_Node_Handler handle_ocStbHostCardCpIdList;
/* ocStbHostOobMessageMode module */
Netsnmp_Node_Handler handle_ocStbHostOobMessageMode;
Netsnmp_Node_Handler handle_ocStbHostSoftwareApplicationInfoSigLastReceivedTime;
Netsnmp_Node_Handler handle_ocStbHostSoftwareApplicationInfoSigLastReadStatus;

//To contorl the ECM MIB access for HostRootOID
void EstbContorlAcess_HostcardrootID(int status);
#endif //if 0 //scalar





void initialize_table_ocStbHostAVInterfaceTable(void); //not required
Netsnmp_Node_Handler ocStbHostAVInterfaceTable_handler;
NetsnmpCacheLoad ocStbHostAVInterfaceTable_load;
NetsnmpCacheFree ocStbHostAVInterfaceTable_free;
#define OCSTBHOSTAVINTERFACETABLE_TIMEOUT  2

/*
 * column number definitions for table ocStbHostAVInterfaceTable
 */
#define COLUMN_OCSTBHOSTAVINTERFACEINDEX        1
#define COLUMN_OCSTBHOSTAVINTERFACETYPE        2
#define COLUMN_OCSTBHOSTAVINTERFACEDESC        3
#define COLUMN_OCSTBHOSTAVINTERFACESTATUS        4


/*
     * Typical data structure for a row entry
     */
typedef struct ocStbHostAVInterfaceTable_entry {
    /*
     * Index values
     */
    u_long          ocStbHostAVInterfaceIndex;
    /*
     * Column values
     */
    oid             ocStbHostAVInterfaceType[MAX_IOD_LENGTH];
    size_t          ocStbHostAVInterfaceType_len;
    char            ocStbHostAVInterfaceDesc[CHRMAX];
    size_t          ocStbHostAVInterfaceDesc_len;
    long            ocStbHostAVInterfaceStatus;


}ocStbHostAVInterfaceTable_S;



/*#ifndef OCSTBHOSTIEEE1394TABLE_H
#define OCSTBHOSTIEEE1394TABLE_H*/
/*
 * function declarations
 */
//void            init_ocStbHostIEEE1394Table(void);
void             initialize_table_ocStbHostIEEE1394Table(void);
Netsnmp_Node_Handler ocStbHostIEEE1394Table_handler;
NetsnmpCacheLoad ocStbHostIEEE1394Table_load;
NetsnmpCacheFree ocStbHostIEEE1394Table_free;
#define OCSTBHOSTIEEE1394TABLE_TIMEOUT  2

/*
 * column number definitions for table ocStbHostIEEE1394Table
 */
#define COLUMN_OCSTBHOSTIEEE1394ACTIVENODES        1
#define COLUMN_OCSTBHOSTIEEE1394DATAXMISSION        2
#define COLUMN_OCSTBHOSTIEEE1394DTCPSTATUS        3
#define COLUMN_OCSTBHOSTIEEE1394LOOPSTATUS        4
#define COLUMN_OCSTBHOSTIEEE1394ROOTSTATUS        5
#define COLUMN_OCSTBHOSTIEEE1394CYCLEISMASTER        6
#define COLUMN_OCSTBHOSTIEEE1394IRMSTATUS        7
#define COLUMN_OCSTBHOSTIEEE1394AUDIOMUTESTATUS        8
#define COLUMN_OCSTBHOSTIEEE1394VIDEOMUTESTATUS        9

 /*
     * Typical data structure for a row entry
     */
struct ocStbHostIEEE1394Table_entry {
    /*
     * Index values
     */
    u_long          ocStbHostAVInterfaceIndex;

    /*
     * Column values
     */
    long            ocStbHostIEEE1394ActiveNodes;
    long            ocStbHostIEEE1394DataXMission;
    long            ocStbHostIEEE1394DTCPStatus;
    long            ocStbHostIEEE1394LoopStatus;
    long            ocStbHostIEEE1394RootStatus;
    long            ocStbHostIEEE1394CycleIsMaster;
    long            ocStbHostIEEE1394IRMStatus;
    long            ocStbHostIEEE1394AudioMuteStatus;
    long            ocStbHostIEEE1394VideoMuteStatus;

    int             valid;
};

//#endif                          /* OCSTBHOSTIEEE1394TABLE_H */


// #ifndef OCSTBHOSTIEEE1394CONNECTEDDEVICESTABLE_H
// #define OCSTBHOSTIEEE1394CONNECTEDDEVICESTABLE_H

/*
 * function declarations
 */
//void            init_ocStbHostIEEE1394ConnectedDevicesTable(void);
void
initialize_table_ocStbHostIEEE1394ConnectedDevicesTable(void);
Netsnmp_Node_Handler ocStbHostIEEE1394ConnectedDevicesTable_handler;
NetsnmpCacheLoad ocStbHostIEEE1394ConnectedDevicesTable_load;
NetsnmpCacheFree ocStbHostIEEE1394ConnectedDevicesTable_free;
#define OCSTBHOSTIEEE1394CONNECTEDDEVICESTABLE_TIMEOUT  2

/*
 * column number definitions for table ocStbHostIEEE1394ConnectedDevicesTable
 */
#define COLUMN_OCSTBHOSTIEEE1394CONNECTEDDEVICESINDEX        1
#define COLUMN_OCSTBHOSTIEEE1394CONNECTEDDEVICESAVINTERFACEINDEX        2
#define COLUMN_OCSTBHOSTIEEE1394CONNECTEDDEVICESSUBUNITTYPE        3
#define COLUMN_OCSTBHOSTIEEE1394CONNECTEDDEVICESEUI64        4
#define COLUMN_OCSTBHOSTIEEE1394CONNECTEDDEVICESADSOURCESELECTSUPPORT        5

 /*
     * Typical data structure for a row entry
     */
struct ocStbHostIEEE1394ConnectedDevicesTable_entry {
    /*
     * Index values
     */
    u_long          ocStbHostIEEE1394ConnectedDevicesIndex;

    /*
     * Column values
     */
    u_long          ocStbHostIEEE1394ConnectedDevicesAVInterfaceIndex;
    long            ocStbHostIEEE1394ConnectedDevicesSubUnitType;
    char            ocStbHostIEEE1394ConnectedDevicesEui64[CHRMAX];
    size_t          ocStbHostIEEE1394ConnectedDevicesEui64_len;
    long            ocStbHostIEEE1394ConnectedDevicesADSourceSelectSupport;

    int             valid;
};
//#endif                          /* OCSTBHOSTIEEE1394CONNECTEDDEVICESTABLE_H */





//#ifndef OCSTBHOSTINBANDTUNERTABLE_H
//#define OCSTBHOSTINBANDTUNERTABLE_H

/**
 * OCSTBHOSTINBANDTUNERTABLE_H  function declarations
 */
//void            init_ocStbHostInBandTunerTable(void);
void            initialize_table_ocStbHostInBandTunerTable(void);
Netsnmp_Node_Handler ocStbHostInBandTunerTable_handler;
NetsnmpCacheLoad ocStbHostInBandTunerTable_load;
NetsnmpCacheFree ocStbHostInBandTunerTable_free;
#define OCSTBHOSTINBANDTUNERTABLE_TIMEOUT  2
/*
 * column number definitions for table ocStbHostInBandTunerTable
 */
#define COLUMN_OCSTBHOSTINBANDTUNERMODULATIONMODE        1
#define COLUMN_OCSTBHOSTINBANDTUNERFREQUENCY        2
#define COLUMN_OCSTBHOSTINBANDTUNERINTERLEAVER        3
#define COLUMN_OCSTBHOSTINBANDTUNERPOWER        4
#define COLUMN_OCSTBHOSTINBANDTUNERAGCVALUE        5
#define COLUMN_OCSTBHOSTINBANDTUNERSNRVALUE        6
#define COLUMN_OCSTBHOSTINBANDTUNERUNERROREDS        7
#define COLUMN_OCSTBHOSTINBANDTUNERCORRECTEDS        8
#define COLUMN_OCSTBHOSTINBANDTUNERUNCORRECTABLES        9
#define COLUMN_OCSTBHOSTINBANDTUNERCARRIERLOCKLOST        10
#define COLUMN_OCSTBHOSTINBANDTUNERPCRERRORS        11
#define COLUMN_OCSTBHOSTINBANDTUNERPTSERRORS        12
#define COLUMN_OCSTBHOSTINBANDTUNERSTATE        13
#define COLUMN_OCSTBHOSTINBANDTUNERBER        14
#define COLUMN_OCSTBHOSTINBANDTUNERSECSSINCELOCK        15
#define COLUMN_OCSTBHOSTINBANDTUNEREQGAIN        16
#define COLUMN_OCSTBHOSTINBANDTUNERMAINTAPCOEFF        17
#define COLUMN_OCSTBHOSTINBANDTUNERTOTALTUNECOUNT        18
#define COLUMN_OCSTBHOSTINBANDTUNERTUNEFAILURECOUNT        19
#define COLUMN_OCSTBHOSTINBANDTUNERTUNEFAILFREQ        20
#define COLUMN_OCSTBHOSTINBANDTUNERBANDWIDTH        21
#define COLUMN_OCSTBHOSTINBANDTUNERCHANNELMAPID        22
#define COLUMN_OCSTBHOSTINBANDTUNERDACID        23
    /*
     * Typical data structure for a row entry
     */
struct ocStbHostInBandTunerTable_entry {
    /*
     * Index values
     */
    u_long          ocStbHostAVInterfaceIndex;

    /*
     * Column values
     */
    long            ocStbHostInBandTunerModulationMode;
    u_long          ocStbHostInBandTunerFrequency;
    long            ocStbHostInBandTunerInterleaver;
    long            ocStbHostInBandTunerPower;
    long            old_ocStbHostInBandTunerPower;
    u_long          ocStbHostInBandTunerAGCValue;
    long            ocStbHostInBandTunerSNRValue;
    u_long          ocStbHostInBandTunerUnerroreds;
    u_long          ocStbHostInBandTunerCorrecteds;
    u_long          ocStbHostInBandTunerUncorrectables;
    u_long          ocStbHostInBandTunerCarrierLockLost;
    u_long          ocStbHostInBandTunerPCRErrors;
    u_long          ocStbHostInBandTunerPTSErrors;
    long            ocStbHostInBandTunerState;
    long            ocStbHostInBandTunerBER;
    u_long          ocStbHostInBandTunerSecsSinceLock;
    long            ocStbHostInBandTunerEqGain;
    long            ocStbHostInBandTunerMainTapCoeff;
    u_long          ocStbHostInBandTunerTotalTuneCount;
    u_long          ocStbHostInBandTunerTuneFailureCount;
    u_long          ocStbHostInBandTunerTuneFailFreq;
    long            ocStbHostInBandTunerBandwidth;
    u_long 	    ocStbHostInBandTunerChannelMapId;
    u_long 	    ocStbHostInBandTunerDacId;
    

    int             valid;
};
//#endif                          /* OCSTBHOSTINBANDTUNERTABLE_H */

//#ifndef OCSTBHOSTDVIHDMITABLE_H
//#define OCSTBHOSTDVIHDMITABLE_H
/*
 * function declarations
 */
//void            init_ocStbHostDVIHDMITable(void);
void            initialize_table_ocStbHostDVIHDMITable(void);
Netsnmp_Node_Handler ocStbHostDVIHDMITable_handler;
NetsnmpCacheLoad ocStbHostDVIHDMITable_load;
NetsnmpCacheFree ocStbHostDVIHDMITable_free;
#define OCSTBHOSTDVIHDMITABLE_TIMEOUT  2

/*
 * column number definitions for table ocStbHostDVIHDMITable
 */
#define COLUMN_OCSTBHOSTDVIHDMIOUTPUTTYPE        2
#define COLUMN_OCSTBHOSTDVIHDMICONNECTIONSTATUS        3
#define COLUMN_OCSTBHOSTDVIHDMIREPEATERSTATUS        4
#define COLUMN_OCSTBHOSTDVIHDMIVIDEOXMISSIONSTATUS        5
#define COLUMN_OCSTBHOSTDVIHDMIHDCPSTATUS        6
#define COLUMN_OCSTBHOSTDVIHDMIVIDEOMUTESTATUS        7
#define COLUMN_OCSTBHOSTDVIHDMIOUTPUTFORMAT        8
#define COLUMN_OCSTBHOSTDVIHDMIASPECTRATIO        9
#define COLUMN_OCSTBHOSTDVIHDMIHOSTDEVICEHDCPSTATUS        10
#define COLUMN_OCSTBHOSTDVIHDMIAUDIOFORMAT        11
#define COLUMN_OCSTBHOSTDVIHDMIAUDIOSAMPLERATE        12
#define COLUMN_OCSTBHOSTDVIHDMIAUDIOCHANNELCOUNT        13
#define COLUMN_OCSTBHOSTDVIHDMIAUDIOMUTESTATUS        14
#define COLUMN_OCSTBHOSTDVIHDMIAUDIOSAMPLESIZE        15
#define COLUMN_OCSTBHOSTDVIHDMICOLORSPACE        16
#define COLUMN_OCSTBHOSTDVIHDMIFRAMERATE        17
#define COLUMN_OCSTBHOSTDVIHDMIATTACHEDDEVICETYPE       18
#define COLUMN_OCSTBHOSTDVIHDMIEDID     19
#define COLUMN_OCSTBHOSTDVIHDMILIPSYNCDELAY     20
#define COLUMN_OCSTBHOSTDVIHDMICECFEATURES      21
#define COLUMN_OCSTBHOSTDVIHDMIFEATURES     22
#define COLUMN_OCSTBHOSTDVIHDMIMAXDEVICECOUNT       23
#define COLUMN_OCSTBHOSTDVIHDMIPREFERREDVIDEOFORMAT    24
#define COLUMN_OCSTBHOSTDVIHDMIEDIDVERSION        25
#define COLUMN_OCSTBHOSTDVIHDMI3DCOMPATIBILITYCONTROL        26
#define COLUMN_OCSTBHOSTDVIHDMI3DCOMPATIBILITYMSGDISPLAY        27
//#endif                          /* OCSTBHOSTDVIHDMITABLE_H */

 /*
     * Typical data structure for a row entry
     */
struct ocStbHostDVIHDMITable_entry {
    /*
     * Index values
     */
    u_long          ocStbHostAVInterfaceIndex;

    /*
     * Column values
     */
    long            ocStbHostDVIHDMIOutputType;
    long            ocStbHostDVIHDMIConnectionStatus;
    long            ocStbHostDVIHDMIRepeaterStatus;
    long            ocStbHostDVIHDMIVideoXmissionStatus;
    long            ocStbHostDVIHDMIHDCPStatus;
    long            ocStbHostDVIHDMIVideoMuteStatus;
    long            ocStbHostDVIHDMIOutputFormat;
    long            ocStbHostDVIHDMIAspectRatio;
    long            ocStbHostDVIHDMIHostDeviceHDCPStatus;
    long            ocStbHostDVIHDMIAudioFormat;
    long            ocStbHostDVIHDMIAudioSampleRate;
    u_long          ocStbHostDVIHDMIAudioChannelCount;
    long            ocStbHostDVIHDMIAudioMuteStatus;
    long            ocStbHostDVIHDMIAudioSampleSize;
    long            ocStbHostDVIHDMIColorSpace;
    long            ocStbHostDVIHDMIFrameRate;
    long            ocStbHostDVIHDMIAttachedDeviceType;
    char            ocStbHostDVIHDMIEdid[3*CHRMAX];
    size_t          ocStbHostDVIHDMIEdid_len;
    long            ocStbHostDVIHDMILipSyncDelay;
    char            ocStbHostDVIHDMICecFeatures[CHRMAX];
    size_t          ocStbHostDVIHDMICecFeatures_len;
    char            ocStbHostDVIHDMIFeatures[CHRMAX];
    size_t          ocStbHostDVIHDMIFeatures_len;
    long            ocStbHostDVIHDMIMaxDeviceCount;
    long         ocStbHostDVIHDMIPreferredVideoFormat;
    char         ocStbHostDVIHDMIEdidVersion[CHRMAX];
    size_t        ocStbHostDVIHDMIEdidVersion_len;
    long          ocStbHostDVIHDMI3DCompatibilityControl;
    long          old_ocStbHostDVIHDMI3DCompatibilityControl;
    long          ocStbHostDVIHDMI3DCompatibilityMsgDisplay;
    long          old_ocStbHostDVIHDMI3DCompatibilityMsgDisplay;

    int             valid;
};

/** Initializes the ocStbHostDVIHDMIAvailableVideoFormatTable module */
void initialize_table_ocStbHostDVIHDMIAvailableVideoFormatTable(void);
Netsnmp_Node_Handler ocStbHostDVIHDMIAvailableVideoFormatTable_handler;
NetsnmpCacheLoad ocStbHostDVIHDMIAvailableVideoFormatTable_load;
NetsnmpCacheFree ocStbHostDVIHDMIAvailableVideoFormatTable_free;
#define OCSTBHOSTDVIHDMIAVAILABLE_TIMEOUT  2
/* column number definitions for table ocStbHostDVIHDMIAvailableVideoFormatTable */
#define COLUMN_OCSTBHOSTDVIHDMIAVAILABLEVIDEOFORMATINDEX        1
#define COLUMN_OCSTBHOSTDVIHDMIAVAILABLEVIDEOFORMAT        2
#define COLUMN_OCSTBHOSTDVIHDMISUPPORTED3DSTRUCTURES        3
#define COLUMN_OCSTBHOSTDVIHDMIACTIVE3DSTRUCTURE        4
/* Typical data structure for a row entry */
struct ocStbHostDVIHDMIAvailableVideoFormatTable_entry {
    /* Index values */
    long ocStbHostDVIHDMIAvailableVideoFormatIndex;
    /* Column values */
    long ocStbHostDVIHDMIAvailableVideoFormat;
    char ocStbHostDVIHDMISupported3DStructures[CHRMAX];
    long ocStbHostDVIHDMISupported3DStructures_len;
    long ocStbHostDVIHDMIActive3DStructure;

    int   valid;
};


/*
 * function declarations
 */
//void            init_ocStbHostComponentVideoTable(void);
void            initialize_table_ocStbHostComponentVideoTable(void);
Netsnmp_Node_Handler ocStbHostComponentVideoTable_handler;
NetsnmpCacheLoad ocStbHostComponentVideoTable_load;
NetsnmpCacheFree ocStbHostComponentVideoTable_free;
#define OCSTBHOSTCOMPONENTVIDEOTABLE_TIMEOUT  2

/*
 * column number definitions for table ocStbHostComponentVideoTable
 */
#define COLUMN_OCSTBHOSTCOMPONENTVIDEOCONSTRAINEDSTATUS        1
#define COLUMN_OCSTBHOSTCOMPONENTOUTPUTFORMAT        2
#define COLUMN_OCSTBHOSTCOMPONENTASPECTRATIO        3
#define COLUMN_OCSTBHOSTCOMPONENTVIDEOMUTESTATUS        4
//#endif                          /* OCSTBHOSTCOMPONENTVIDEOTABLE_H */


  /*
     * Typical data structure for a row entry
     */
struct ocStbHostComponentVideoTable_entry {
    /*
     * Index values
     */
    u_long          ocStbHostAVInterfaceIndex;

    /*
     * Column values
     */
    long            ocStbHostComponentVideoConstrainedStatus;
    long            ocStbHostComponentOutputFormat;
    long            ocStbHostComponentAspectRatio;
    long            ocStbHostComponentVideoMuteStatus;

    int             valid;
};



/*
 * function declarations
 */
//void            init_ocStbHostRFChannelOutTable(void);
void            initialize_table_ocStbHostRFChannelOutTable(void);
Netsnmp_Node_Handler ocStbHostRFChannelOutTable_handler;
NetsnmpCacheLoad ocStbHostRFChannelOutTable_load;
NetsnmpCacheFree ocStbHostRFChannelOutTable_free;
#define OCSTBHOSTRFCHANNELOUTTABLE_TIMEOUT  2

/*
 * column number definitions for table ocStbHostRFChannelOutTable
 */
#define COLUMN_OCSTBHOSTRFCHANNELOUT        2
#define COLUMN_OCSTBHOSTRFCHANNELOUTAUDIOMUTESTATUS        3
#define COLUMN_OCSTBHOSTRFCHANNELOUTVIDEOMUTESTATUS        4
//#endif                          /* OCSTBHOSTRFCHANNELOUTTABLE_H */

 /*
     * Typical data structure for a row entry
     */
struct ocStbHostRFChannelOutTable_entry {
    /*
     * Index values
     */
    u_long          ocStbHostAVInterfaceIndex;

    /*
     * Column values
     */
    u_long          ocStbHostRFChannelOut;
    long            ocStbHostRFChannelOutAudioMuteStatus;
    long            ocStbHostRFChannelOutVideoMuteStatus;

    int             valid;
};




/*
 * function declarations
 */
//void            init_ocStbHostAnalogVideoTable(void);
void            initialize_table_ocStbHostAnalogVideoTable(void);
Netsnmp_Node_Handler ocStbHostAnalogVideoTable_handler;
NetsnmpCacheLoad ocStbHostAnalogVideoTable_load;
NetsnmpCacheFree ocStbHostAnalogVideoTable_free;
#define OCSTBHOSTANALOGVIDEOTABLE_TIMEOUT  2

/*
 * column number definitions for table ocStbHostAnalogVideoTable
 */
#define COLUMN_OCSTBHOSTANALOGVIDEOPROTECTIONSTATUS        1


    /*
     * Typical data structure for a row entry
     */
struct ocStbHostAnalogVideoTable_entry {
    /*
     * Index values
     */
    u_long          ocStbHostAVInterfaceIndex;

    /*
     * Column values
     */
    long            ocStbHostAnalogVideoProtectionStatus;

    int             valid;
};


/*
 * function declarations
 */
//void            init_ocStbHostProgramStatusTable(void);
void            initialize_table_ocStbHostProgramStatusTable(void);
Netsnmp_Node_Handler ocStbHostProgramStatusTable_handler;
NetsnmpCacheLoad ocStbHostProgramStatusTable_load;
NetsnmpCacheFree ocStbHostProgramStatusTable_free;
#define OCSTBHOSTPROGRAMSTATUSTABLE_TIMEOUT  2

/*
 * column number definitions for table ocStbHostProgramStatusTable
 */
#define COLUMN_OCSTBHOSTPROGRAMINDEX        1
#define COLUMN_OCSTBHOSTPROGRAMAVSOURCE        2
#define COLUMN_OCSTBHOSTPROGRAMAVDESTINATION        3
#define COLUMN_OCSTBHOSTPROGRAMCONTENTSOURCE        4
#define COLUMN_OCSTBHOSTPROGRAMCONTENTDESTINATION        5


    /*
     * Typical data structure for a row entry
     */
struct ocStbHostProgramStatusTable_entry {
    /*
     * Index values
     */
    u_long          ocStbHostProgramIndex;

    /*
     * Column values
     */
    oid             ocStbHostProgramAVSource[MAX_IOD_LENGTH];
    size_t          ocStbHostProgramAVSource_len;
    oid             ocStbHostProgramAVDestination[MAX_IOD_LENGTH];
    size_t          ocStbHostProgramAVDestination_len;
    oid             ocStbHostProgramContentSource[MAX_IOD_LENGTH];
    size_t          ocStbHostProgramContentSource_len;
    oid             ocStbHostProgramContentDestination[MAX_IOD_LENGTH];
    size_t          ocStbHostProgramContentDestination_len;

    int             valid;
};


/*
 * function declarations
 */
//void            init_ocStbHostMpeg2ContentTable(void);
void            initialize_table_ocStbHostMpeg2ContentTable(void);
Netsnmp_Node_Handler ocStbHostMpeg2ContentTable_handler;
NetsnmpCacheLoad ocStbHostMpeg2ContentTable_load;
NetsnmpCacheFree ocStbHostMpeg2ContentTable_free;
#define OCSTBHOSTMPEG2CONTENTTABLE_TIMEOUT  2

/*
 * column number definitions for table ocStbHostMpeg2ContentTable
 */
#define COLUMN_OCSTBHOSTMPEG2CONTENTINDEX        1
#define COLUMN_OCSTBHOSTMPEG2CONTENTPROGRAMNUMBER        2
#define COLUMN_OCSTBHOSTMPEG2CONTENTTRANSPORTSTREAMID        3
#define COLUMN_OCSTBHOSTMPEG2CONTENTTOTALSTREAMS        4
#define COLUMN_OCSTBHOSTMPEG2CONTENTSELECTEDVIDEOPID        5
#define COLUMN_OCSTBHOSTMPEG2CONTENTSELECTEDAUDIOPID        6
#define COLUMN_OCSTBHOSTMPEG2CONTENTOTHERAUDIOPIDS        7
#define COLUMN_OCSTBHOSTMPEG2CONTENTCCIVALUE        8
#define COLUMN_OCSTBHOSTMPEG2CONTENTAPSVALUE        9
#define COLUMN_OCSTBHOSTMPEG2CONTENTCITSTATUS        10
#define COLUMN_OCSTBHOSTMPEG2CONTENTBROADCASTFLAGSTATUS        11
#define COLUMN_OCSTBHOSTMPEG2CONTENTEPNSTATUS        12
#define COLUMN_OCSTBHOSTMPEG2CONTENTPCRPID        13
#define COLUMN_OCSTBHOSTMPEG2CONTENTPCRLOCKSTATUS        14
#define COLUMN_OCSTBHOSTMPEG2CONTENTDECODERPTS        15
#define COLUMN_OCSTBHOSTMPEG2CONTENTDISCONTINUITIES        16
#define COLUMN_OCSTBHOSTMPEG2CONTENTPKTERRORS        17
#define COLUMN_OCSTBHOSTMPEG2CONTENTPIPELINEERRORS        18
#define COLUMN_OCSTBHOSTMPEG2CONTENTDECODERRESTARTS        19
#define COLUMN_OCSTBHOSTMPEG2CONTAINERFORMAT        20
  /*
     * Typical data structure for a row entry
     */
struct ocStbHostMpeg2ContentTable_entry {
    /*
     * Index values
     */
    u_long          ocStbHostMpeg2ContentIndex;

    /*
     * Column values
     */
    u_long          ocStbHostMpeg2ContentProgramNumber;
    u_long          ocStbHostMpeg2ContentTransportStreamID;
    u_long          ocStbHostMpeg2ContentTotalStreams;
    long            ocStbHostMpeg2ContentSelectedVideoPID;
    long            ocStbHostMpeg2ContentSelectedAudioPID;
    long            ocStbHostMpeg2ContentOtherAudioPIDs;
    long            ocStbHostMpeg2ContentCCIValue;
    long            ocStbHostMpeg2ContentAPSValue;
    long            ocStbHostMpeg2ContentCITStatus;
    long            ocStbHostMpeg2ContentBroadcastFlagStatus;
    long            ocStbHostMpeg2ContentEPNStatus;
    long            ocStbHostMpeg2ContentPCRPID;
    long            ocStbHostMpeg2ContentPCRLockStatus;
    u_long          ocStbHostMpeg2ContentDecoderPTS;
    u_long          ocStbHostMpeg2ContentDiscontinuities;
    u_long          ocStbHostMpeg2ContentPktErrors;
    u_long          ocStbHostMpeg2ContentPipelineErrors;
    u_long          ocStbHostMpeg2ContentDecoderRestarts;
    long            ocStbHostMpeg2ContainerFormat;

    int             valid;
};

void            initialize_table_ocStbHostMpeg4ContentTable(void);
Netsnmp_Node_Handler ocStbHostMpeg4ContentTable_handler;
NetsnmpCacheLoad ocStbHostMpeg4ContentTable_load;
NetsnmpCacheFree ocStbHostMpeg4ContentTable_free;
#define OCSTBHOSTMPEG4CONTENTTABLE_TIMEOUT  2

/*
 * column number definitions for table ocStbHostMpeg4ContentTable
 */
#define COLUMN_OCSTBHOSTMPEG4CONTENTINDEX        1
#define COLUMN_OCSTBHOSTMPEG4CONTENTPROGRAMNUMBER        2
#define COLUMN_OCSTBHOSTMPEG4CONTENTTRANSPORTSTREAMID        3
#define COLUMN_OCSTBHOSTMPEG4CONTENTTOTALSTREAMS        4
#define COLUMN_OCSTBHOSTMPEG4CONTENTSELECTEDVIDEOPID        5
#define COLUMN_OCSTBHOSTMPEG4CONTENTSELECTEDAUDIOPID        6
#define COLUMN_OCSTBHOSTMPEG4CONTENTOTHERAUDIOPIDS        7
#define COLUMN_OCSTBHOSTMPEG4CONTENTCCIVALUE        8
#define COLUMN_OCSTBHOSTMPEG4CONTENTAPSVALUE        9
#define COLUMN_OCSTBHOSTMPEG4CONTENTCITSTATUS        10
#define COLUMN_OCSTBHOSTMPEG4CONTENTBROADCASTFLAGSTATUS        11
#define COLUMN_OCSTBHOSTMPEG4CONTENTEPNSTATUS        12
#define COLUMN_OCSTBHOSTMPEG4CONTENTPCRPID        13
#define COLUMN_OCSTBHOSTMPEG4CONTENTPCRLOCKSTATUS        14
#define COLUMN_OCSTBHOSTMPEG4CONTENTDECODERPTS        15
#define COLUMN_OCSTBHOSTMPEG4CONTENTDISCONTINUITIES        16
#define COLUMN_OCSTBHOSTMPEG4CONTENTPKTERRORS        17
#define COLUMN_OCSTBHOSTMPEG4CONTENTPIPELINEERRORS        18
#define COLUMN_OCSTBHOSTMPEG4CONTENTDECODERRESTARTS        19
#define COLUMN_OCSTBHOSTMPEG4CONTAINERFORMAT        20
  /*
     * Typical data structure for a row entry
     */
struct ocStbHostMpeg4ContentTable_entry {
    /*
     * Index values
     */
    u_long          ocStbHostMpeg4ContentIndex;

    /*
     * Column values
     */
    u_long          ocStbHostMpeg4ContentProgramNumber;
    u_long          ocStbHostMpeg4ContentTransportStreamID;
    u_long          ocStbHostMpeg4ContentTotalStreams;
    long            ocStbHostMpeg4ContentSelectedVideoPID;
    long            ocStbHostMpeg4ContentSelectedAudioPID;
    long            ocStbHostMpeg4ContentOtherAudioPIDs;
    long            ocStbHostMpeg4ContentCCIValue;
    long            ocStbHostMpeg4ContentAPSValue;
    long            ocStbHostMpeg4ContentCITStatus;
    long            ocStbHostMpeg4ContentBroadcastFlagStatus;
    long            ocStbHostMpeg4ContentEPNStatus;
    long            ocStbHostMpeg4ContentPCRPID;
    long            ocStbHostMpeg4ContentPCRLockStatus;
    u_long          ocStbHostMpeg4ContentDecoderPTS;
    u_long          ocStbHostMpeg4ContentDiscontinuities;
    u_long          ocStbHostMpeg4ContentPktErrors;
    u_long          ocStbHostMpeg4ContentPipelineErrors;
    u_long          ocStbHostMpeg4ContentDecoderRestarts;
    long            ocStbHostMpeg4ContainerFormat;

    int             valid;
};

void            initialize_table_ocStbHostVc1ContentTable(void);
Netsnmp_Node_Handler ocStbHostVc1ContentTable_handler;
NetsnmpCacheLoad ocStbHostVc1ContentTable_load;
NetsnmpCacheFree ocStbHostVc1ContentTable_free;
#define OCSTBHOSTVC1CONTENTTABLE_TIMEOUT  2

/*
 * column number definitions for table ocStbHostVc1ContentTable
 */
#define COLUMN_OCSTBHOSTVC1CONTENTINDEX        1
#define COLUMN_OCSTBHOSTVC1CONTENTPROGRAMNUMBER        2
#define COLUMN_OCSTBHOSTVC1CONTAINERFORMAT        20
  /*
     * Typical data structure for a row entry
     */
struct ocStbHostVc1ContentTable_entry {
    /*
     * Index values
     */
    u_long          ocStbHostVc1ContentIndex;

    /*
     * Column values
     */
    u_long          ocStbHostVc1ContentProgramNumber;
    long            ocStbHostVc1ContainerFormat;

    int             valid;
};

// #ifndef OCSTBHOSTSPDIFTABLE_H
// #define OCSTBHOSTSPDIFTABLE_H

/*
 * function declarations
 */
//void            init_ocStbHostSPDIfTable(void);
void            initialize_table_ocStbHostSPDIfTable(void);
Netsnmp_Node_Handler ocStbHostSPDIfTable_handler;
NetsnmpCacheLoad ocStbHostSPDIfTable_load;
NetsnmpCacheFree ocStbHostSPDIfTable_free;
#define OCSTBHOSTSPDIFTABLE_TIMEOUT  2

/*
 * column number definitions for table ocStbHostSPDIfTable
 */
#define COLUMN_OCSTBHOSTSPDIFAUDIOFORMAT        1
#define COLUMN_OCSTBHOSTSPDIFAUDIOMUTESTATUS        2

   /*
     * Typical data structure for a row entry
     */
struct ocStbHostSPDIfTable_entry {
    /*
     * Index values
     */
    u_long          ocStbHostAVInterfaceIndex;

    /*
     * Column values
     */
    long            ocStbHostSPDIfAudioFormat;
    long            ocStbHostSPDIfAudioMuteStatus;

    int             valid;
};
//#endif                          /* OCSTBHOSTSPDIFTABLE_H */


// #ifndef OCSTBHOSTCCAPPINFOTABLE_H
// #define OCSTBHOSTCCAPPINFOTABLE_H

/*
 * function declarations
 */
//void            init_ocStbHostCCAppInfoTable(void);
void            initialize_table_ocStbHostCCAppInfoTable(void);
Netsnmp_Node_Handler ocStbHostCCAppInfoTable_handler;
NetsnmpCacheLoad ocStbHostCCAppInfoTable_load;
NetsnmpCacheFree ocStbHostCCAppInfoTable_free;
#define OCSTBHOSTCCAPPINFOTABLE_TIMEOUT  5

/*
 * column number definitions for table ocStbHostCCAppInfoTable
 */
#define COLUMN_OCSTBHOSTCCAPPINFOINDEX        1
#define COLUMN_OCSTBHOSTCCAPPLICATIONTYPE        2
#define COLUMN_OCSTBHOSTCCAPPLICATIONNAME        3
#define COLUMN_OCSTBHOSTCCAPPLICATIONVERSION        4
#define COLUMN_OCSTBHOSTCCAPPINFOPAGE        5

    /*
     * Typical data structure for a row entry
     */
struct ocStbHostCCAppInfoTable_entry {
    /*
     * Index values
     */
    u_long          ocStbHostCCAppInfoIndex;

    /*
     * Column values
     */
    u_long          ocStbHostCCApplicationType;
    char            ocStbHostCCApplicationName[CHRMAX];
    size_t          ocStbHostCCApplicationName_len;
    u_long          ocStbHostCCApplicationVersion;
    char            ocStbHostCCAppInfoPage[CHRMAX*4];
    size_t          ocStbHostCCAppInfoPage_len;

    int             valid;
};
//#endif                          /* OCSTBHOSTCCAPPINFOTABLE_H */



/** Non used tables may be we will use in future */

// #ifndef OCSTBHOSTSOFTWAREAPPLICATIONINFOTABLE_H
// #define OCSTBHOSTSOFTWAREAPPLICATIONINFOTABLE_H

/*
 * function declarations
 */
//void            init_ocStbHostSoftwareApplicationInfoTable(void);
void
initialize_table_ocStbHostSoftwareApplicationInfoTable(void);
Netsnmp_Node_Handler ocStbHostSoftwareApplicationInfoTable_handler;
NetsnmpCacheLoad ocStbHostSoftwareApplicationInfoTable_load;
NetsnmpCacheFree ocStbHostSoftwareApplicationInfoTable_free;
#define OCSTBHOSTSOFTWAREAPPLICATIONINFOTABLE_TIMEOUT  2

/*
 * column number definitions for table ocStbHostSoftwareApplicationInfoTable
 */
#define COLUMN_OCSTBHOSTSOFTWAREAPPNAMESTRING        1
#define COLUMN_OCSTBHOSTSOFTWAREAPPVERSIONNUMBER        2
#define COLUMN_OCSTBHOSTSOFTWARESTATUS        3
#define COLUMN_OCSTBHOSTSOFTWAREAPPLICATIONINFOINDEX        4
#define COLUMN_OCSTBHOSTSOFTWAREORGANIZATIONID        5
#define COLUMN_OCSTBHOSTSOFTWAREAPPLICATIONID        6
#define COLUMN_OCSTBHOSTSOFTWAREAPPLICATIONSIGSTATUS        7


/*
 * Typical data structure for a row entry
*/
#define SAORG_ID 4
#define SA_ID 2
struct ocStbHostSoftwareApplicationInfoTable_entry {
    /*
     * Index values
     */
    u_long          ocStbHostSoftwareApplicationInfoIndex;

    /*
     * Column values
     */
    char            ocStbHostSoftwareAppNameString[CHRMAX];
    size_t          ocStbHostSoftwareAppNameString_len;
    char            ocStbHostSoftwareAppVersionNumber[CHRMAX];
    size_t          ocStbHostSoftwareAppVersionNumber_len;
    long            ocStbHostSoftwareStatus;
    char            ocStbHostSoftwareOrganizationId[SAORG_ID]; //4-BYTES
    size_t          ocStbHostSoftwareOrganizationId_len;
    char            ocStbHostSoftwareApplicationId[SA_ID]; //2-BYTES
    size_t          ocStbHostSoftwareApplicationId_len;
    long            ocStbHostSoftwareApplicationSigStatus;

    int             valid;
};
//#endif                          /* OCSTBHOSTSOFTWAREAPPLICATIONINFOTABLE_H */


// #ifndef OCSTBHOSTSYSTEMHOMENETWORKTABLE_H
// #define OCSTBHOSTSYSTEMHOMENETWORKTABLE_H

/*
 * function declarations
 */
//void            init_ocStbHostSystemHomeNetworkTable(void);
void            initialize_table_ocStbHostSystemHomeNetworkTable(void);
Netsnmp_Node_Handler ocStbHostSystemHomeNetworkTable_handler;
NetsnmpCacheLoad ocStbHostSystemHomeNetworkTable_load;
NetsnmpCacheFree ocStbHostSystemHomeNetworkTable_free;
#define OCSTBHOSTSYSTEMHOMENETWORKTABLE_TIMEOUT  2

/*
 * column number definitions for table ocStbHostSystemHomeNetworkTable
 */
#define COLUMN_OCSTBHOSTSYSTEMHOMENETWORKINDEX        1
#define COLUMN_OCSTBHOSTSYSTEMHOMENETWORKMAXCLIENTS        2
#define COLUMN_OCSTBHOSTSYSTEMHOMENETWORKHOSTDRMSTATUS        3
#define COLUMN_OCSTBHOSTSYSTEMHOMENETWORKCONNECTEDCLIENTS        4
#define COLUMN_OCSTBHOSTSYSTEMHOMENETWORKCLIENTMACADDRESS        5
#define COLUMN_OCSTBHOSTSYSTEMHOMENETWORKCLIENTIPADDRESS        6
#define COLUMN_OCSTBHOSTSYSTEMHOMENETWORKCLIENTDRMSTATUS        7


 /*
     * Typical data structure for a row entry
     */
struct ocStbHostSystemHomeNetworkTable_entry {
    /*
     * Index values
     */
    u_long          ocStbHostSystemHomeNetworkIndex;

    /*
     * Column values
     */
    long            ocStbHostSystemHomeNetworkMaxClients;
    long            ocStbHostSystemHomeNetworkHostDRMStatus;
    long            ocStbHostSystemHomeNetworkConnectedClients;
    char            ocStbHostSystemHomeNetworkClientMacAddress[CHRMAX];
    size_t          ocStbHostSystemHomeNetworkClientMacAddress_len;
    char            ocStbHostSystemHomeNetworkClientIpAddress[CHRMAX];
    size_t          ocStbHostSystemHomeNetworkClientIpAddress_len;
    long            ocStbHostSystemHomeNetworkClientDRMStatus;

    int             valid;
};
//#endif                          /* OCSTBHOSTSYSTEMHOMENETWORKTABLE_H */


// #ifndef OCSTBHOSTSYSTEMMEMORYREPORTTABLE_H
// #define OCSTBHOSTSYSTEMMEMORYREPORTTABLE_H

/*
 * function declarations
 */
//void            init_ocStbHostSystemMemoryReportTable(void);
void            initialize_table_ocStbHostSystemMemoryReportTable(void);
Netsnmp_Node_Handler ocStbHostSystemMemoryReportTable_handler;
NetsnmpCacheLoad ocStbHostSystemMemoryReportTable_load;
NetsnmpCacheFree ocStbHostSystemMemoryReportTable_free;
#define OCSTBHOSTSYSTEMMEMORYREPORTTABLE_TIMEOUT  2

/*
 * column number definitions for table ocStbHostSystemMemoryReportTable
 */
#define COLUMN_OCSTBHOSTSYSTEMMEMORYREPORTINDEX        1
#define COLUMN_OCSTBHOSTSYSTEMMEMORYREPORTMEMORYTYPE        2
#define COLUMN_OCSTBHOSTSYSTEMMEMORYREPORTMEMORYSIZE        3
   /*
     * Typical data structure for a row entry
     */
struct ocStbHostSystemMemoryReportTable_entry {
    /*
     * Index values
     */
    u_long          ocStbHostSystemMemoryReportIndex;

    /*
     * Column values
     */
    long            ocStbHostSystemMemoryReportMemoryType;
    long            ocStbHostSystemMemoryReportMemorySize;

    int             valid;
};
//#endif                          /* OCSTBHOSTSYSTEMMEMORYREPORTTABLE_H */


// #ifndef OCSTBHOSTSYSTEMTEMPTABLE_H
// #define OCSTBHOSTSYSTEMTEMPTABLE_H

/*
 * function declarations
 */
//void            init_ocStbHostSystemTempTable(void);
void            initialize_table_ocStbHostSystemTempTable(void);
Netsnmp_Node_Handler ocStbHostSystemTempTable_handler;
NetsnmpCacheLoad ocStbHostSystemTempTable_load;
NetsnmpCacheFree ocStbHostSystemTempTable_free;
#define OCSTBHOSTSYSTEMTEMPTABLE_TIMEOUT  2

/*
 * column number definitions for table ocStbHostSystemTempTable
 */
#define COLUMN_OCSTBHOSTSYSTEMTEMPINDEX        1
#define COLUMN_OCSTBHOSTSYSTEMTEMPDESCR        2
#define COLUMN_OCSTBHOSTSYSTEMTEMPVALUE        3
#define COLUMN_OCSTBHOSTSYSTEMTEMPLASTUPDATE        4
#define COLUMN_OCSTBHOSTSYSTEMTEMPMAXVALUE      5

 /*
     * Typical data structure for a row entry
     */
struct ocStbHostSystemTempTable_entry {
    /*
     * Index values
     */
    long            hrDeviceIndex;
    u_long          ocStbHostSystemTempIndex;

    /*
     * Column values
     */
    char            ocStbHostSystemTempDescr[CHRMAX];
    size_t          ocStbHostSystemTempDescr_len;
    long            ocStbHostSystemTempValue;
    u_long          ocStbHostSystemTempLastUpdate;
    long            ocStbHostSystemTempMaxValue;

    int             valid;
};

//#endif                          /* OCSTBHOSTSYSTEMTEMPTABLE_H */








#endif                          /* OCSTBHOSTMIBMODULE_H */



