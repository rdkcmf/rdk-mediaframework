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


#ifndef __AVINTERFACE_IORM_HEADER
#define __AVINTERFACE_IORM_HEADER

#include "AvOutInterfaceDefs.h"

#define MAX_VIVIDLOGICERT_SIZE 2048
#define API_CHRMAX 256
#define API_MAX_IOD_LENGTH 128
#define MAC_MAX_SIZE 255
#define MAC_BYTE_SIZE 6
#define SNMP_STR_SIZE 128

#define SNMPSET_INTEGER         'i'
#define SNMPSET_UNSIGNED        'u'
#define SNMPSET_STRING          's'
#define SNMPSET_HEX STRING      'x'
#define SNMPSET_DECIMALSTRING   'd'
#define SNMPSET_SNMP_NULLOBJ    'n'
#define SNMPSET_OBJID           'o'
#define SNMPSET_TIMETICKS       't'
#define SNMPSET_IPADDRESS       'a'
#define SNMPSET_BITS            'b'
/* enums for scalar ocStbHostSoftwareApplicationInfoSigLastReadStatus */
#define SOFTWAREAPPLICATIONINFOSIGLASTREADSTATUS_UNKNOWN    0
#define SOFTWAREAPPLICATIONINFOSIGLASTREADSTATUS_OKAY        1
#define SOFTWAREAPPLICATIONINFOSIGLASTREADSTATUS_ERROR        2

/** MAX ROWS ALLOWD ARE 20 , if u want more than 20 rows increase this macrow size SNMP_MAX_ROWS*/
#define SNMP_MAX_ROWS 20
//comment the #define SNMP_DEBUGE_MESG if no debuge mesg are required
//#define SNMP_DEBUGE_MESG
#include <unistd.h>
#include "rdk_debug.h"
#define SNMP_DEBUGPRINT(a,args...) RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SNMP", "%s:%d:"#a"\n", __FUNCTION__,__LINE__, ##args)
#define SNMP_INFOPRINT(a,args...) RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "%s:%d:"#a"\n", __FUNCTION__,__LINE__, ##args)


typedef unsigned long apioid;

/** have to implement for all oid's */
 static apioid avTableId[25][15] = {
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 1, 1 }, // hrDeviceOther "ocStbHostOther"
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 1, 2 },// ocStbHostScte55FdcRx "out-of band tuner"
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 1, 3 },// ocStbHostScte55RdcTx "out-of-band upstreamtransmitter
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 1, 4 },  // ocStbHostScte40FatRx
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 1, 5 },//ocStbHostBbVideoIn
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 1, 6 }, //ocStbHostBbAudioIn
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 1, 7 },  //ocStbHostBbVideoOut
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 1, 8 }, //ocStbHostBbAudioOut
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 1, 9 },//ocStbHostRfOutCh
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 1, 10 },//ocStbHostSVideoIn
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 1, 11 },//ocStbHostSVideoOut
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 1, 12 },//ocStbHostComponentIn
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 1, 13 }, // ocStbHostHdComponentOut
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 1, 14 },//ocStbHostDviIn
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 1, 15 },//ocStbHostDviOut
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 1, 16 },//ocStbHostHdmiIn
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 1, 17 },//ocStbHostHdmiOut
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 1, 18 },//ocStbHostRcaSpdifIn
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 1, 19 },//ocStbHostRcaSpdifOut
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 1, 20 },//ocStbHostToslinkSpdifIn
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 1, 21 },//ocStbHostToslinkSpdifOut

        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 1, 22 },//ocStbHostDisplayOut
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 1, 23 },//ocStbHost1394In
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 1, 24 },//ocStbHost1394Out
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 1, 25 },//ocStbHostDRIInterface
};

static apioid Temp_mpeg2entry[18] ={1,3,6,1,4,1,4491,2,3,1,1,1,2,7,3,1,2,1};
 typedef enum {
    hrDeviceOther,
    ocStbHostScte55FdcRx,
    ocStbHostScte55RdcTx,
    ocStbHostScte40FatRx,
    ocStbHostBbVideoIn,
    ocStbHostBbAudioIn,
    ocStbHostBbVideoOut,
    ocStbHostBbAudioOut,
    ocStbHostRFOutCh,
    ocStbHostSVideoIn,
    ocStbHostSVideoOut,
    ocStbHostComponentIn,
    ocStbHostHdComponentOut,
    ocStbHostDviIn,
    ocStbHostDviOut,
    ocStbHostHdmiIn,
    ocStbHostHdmiOut,
    ocStbHostRcaSpdifIn,
    ocStbHostRcaSpdifOut,
    ocStbHostToslinkSpdifIn,
    ocStbHostToslinkSpdifOut,
        ocStbHostDisplayOut,
    ocStbHost1394In,
    ocStbHost1394Out,
    ocStbHostDRIInterface
}avType_t;

 static apioid avTableRowPointers[25][17] = {
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 2, 1, 2, 1 }, // hrDeviceOther "ocStbHostOther"
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 2, 1, 2, 2 },// ocStbHostScte55FdcRx "out-of band tuner"
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 2, 1, 2, 3 },// ocStbHostScte55RdcTx "out-of-band upstreamtransmitter
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 2, 1, 2, 4 },  // ocStbHostScte40FatRx
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 2, 1, 2, 5 },//ocStbHostBbVideoIn
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 2, 1, 2, 6 }, //ocStbHostBbAudioIn
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 2, 1, 2, 7 },  //ocStbHostBbVideoOut
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 2, 1, 2, 8 }, //ocStbHostBbAudioOut
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 2, 1, 2, 9 },//ocStbHostRfOutCh
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 2, 1, 2, 10 },//ocStbHostSVideoIn
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 2, 1, 2, 11 },//ocStbHostSVideoOut
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 2, 1, 2, 12 },//ocStbHostComponentIn
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 2, 1, 2, 13 }, // ocStbHostHdComponentOut
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 2, 1, 2, 14 },//ocStbHostDviIn
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 2, 1, 2, 15 },//ocStbHostDviOut
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 2, 1, 2, 16 },//ocStbHostHdmiIn
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 2, 1, 2, 17 },//ocStbHostHdmiOut
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 2, 1, 2, 18 },//ocStbHostRcaSpdifIn
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 2, 1, 2, 19 },//ocStbHostRcaSpdifOut
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 2, 1, 2, 20 },//ocStbHostToslinkSpdifIn
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 2, 1, 2, 21 },//ocStbHostToslinkSpdifOut
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 2, 1, 2, 22 },//ocStbHostDisplayOut
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 2, 1, 2, 23 },//ocStbHost1394In
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 2, 1, 2, 24 },//ocStbHost1394Out
    { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 2, 1, 2, 25 },//ocStbHostDRIInterface
};

/*typedef enum {
TunerDes = "Digital Capable Tuner",
RFchannelOut = "RF ChannelOut"

}TypeDescp_t;*/
/*
 * enums for column ocStbHostAVInterfaceStatus
 */


#if 0
/** used in haldisplay.h and haldisplay.cpp*/
typedef enum
{
    AV_INTERFACE_TYPE_COMPOSITE,
    AV_INTERFACE_TYPE_SVIDEO,
    AV_INTERFACE_TYPE_RF,
    AV_INTERFACE_TYPE_COMPONENT,
    AV_INTERFACE_TYPE_HDMI,
    AV_INTERFACE_TYPE_L_R,
    AV_INTERFACE_TYPE_SPDIF,

    AV_INTERFACE_TYPE_MAX
} AV_INTERFACE_TYPE;

typedef struct s_AVOutInterfaceInfo
{
    unsigned long   deviceHandle;
    unsigned long    numAvOutPorts;
    AV_INTERFACE_TYPE avOutTypes[AV_INTERFACE_TYPE_MAX];

}AVOutInterfaceInfo;
/** END */
#endif


typedef enum {
    OCSTBHOSTAVINTERFACESTATUS_ENABLED = 1,
    OCSTBHOSTAVINTERFACESTATUS_DISABLED =2
}avStatus_t;

typedef struct __DisplayRelatedResInfo {
    unsigned long    hDisplay; //Display handle received from haldisplay
    AV_INTERFACE_TYPE    DisplayPortType; //Port type received from haldisplay
} DisplayRelatedResInfo;

typedef union __ResourceIdentityInfo {
    unsigned long             tunerIndex; //corresponds to tuner resource
        unsigned long             Ieee1394port;// for 1394 port status
        DisplayRelatedResInfo    dispResData;
} ResourceIdentityInfo;

typedef struct {
        //unsigned long vl_ocStbHostAVInterfaceIndex;
        ResourceIdentityInfo resourceIdInfo;
         unsigned long     tunerPhysicalIndex;
        avType_t vl_ocStbHostAVInterfaceType ; /*[API_MAX_IOD_LENGTH]; "avtype"  */
                char vl_ocStbHostAVInterfaceDesc[API_CHRMAX];   //String :: SnmpAdminstring
        avStatus_t vl_ocStbHostAVInterfaceStatus;         //enum:: vl_AVInterfaceStatus
} SNMPocStbHostAVInterfaceTable_t;


/**************  TUNER ************/

/*
 * enums for column ocStbHostInBandTunerModulationMode
 */

typedef enum {
    OCSTBHOSTINBANDTUNERMODULATIONMODE_OTHER = 1,
    OCSTBHOSTINBANDTUNERMODULATIONMODE_ANALOG = 2,
    OCSTBHOSTINBANDTUNERMODULATIONMODE_QAM64 = 3,
     OCSTBHOSTINBANDTUNERMODULATIONMODE_QAM256 = 4
} tunerModMode_t;

/*
 * enums for column ocStbHostInBandTunerInterleaver
 */

typedef enum {
    INTERLEAVER_UNKNOWN    =    1,
    INTERLEAVER_OTHER    =    2,
    INTERLEAVER_TAPS64INCREMENT2    =       3,
    INTERLEAVER_TAPS128INCREMENT1    =    4,
    INTERLEAVER_TAPS128INCREMENT2    =    5,
    INTERLEAVER_TAPS128INCREMENT3    =    6,
    INTERLEAVER_TAPS128INCREMENT4    =    7,
    INTERLEAVER_TAPS32INCREMENT4    =    8,
    INTERLEAVER_TAPS16INCREMENT8    =    9,
    INTERLEAVER_TAPS8INCREMENT16    =    10
} tunerInleaver_t;

/*
 * enums for column ocStbHostInBandTunerState
 */
typedef enum {
    TUNERSTATE_READY    =    1,
    TUNERSTATE_WAITINGSYNC    =    2,
    TUNERSTATE_WAITINGQAM    =    3,
    TUNERSTATE_FOUNDSYNC    =    4,
    TUNERSTATE_FOUNDQAM    =    5,
    TUNERSTATE_UNKNOWN    =    6,
    TUNERSTATE_STANDBY    =    7
} tunerState_t;
/*
 * enums for column ocStbHostInBandTunerBER
 */
typedef enum {
    BER_BERGREATERTHAN10E2            =    1,
    BER_BERRANGE10E2TOGREATERTHAN10E4    =    2,
    BER_BERRANGE10E4TOGREATERTHAN10E6    =    3,
    BER_BERRANGE10E6TOGREATERTHAN10E8    =    4,
    BER_BERRANGE10E8TOGREATERTHAN10E12    =    5,
    BER_BEREQUALTOORLESSTHAN10E12          =    6,
    BER_BERNOTAPPLICABLE                 =    7
} tunerBER_t;
typedef enum {
    other_bandwith = 1,
    mHz864 = 2,
    mHz1002 = 3
}TunerBandwidth_t;
typedef struct {

        unsigned long          vl_TunerResourceIndex;               //index
        tunerModMode_t  vl_ocStbHostInBandTunerModulationMode;
        unsigned long          vl_ocStbHostInBandTunerFrequency;
        tunerInleaver_t vl_ocStbHostInBandTunerInterleaver;
        long            vl_ocStbHostInBandTunerPower; //TenthdBmV
        long            vl_old_ocStbHostInBandTunerPower;
        unsigned long          vl_ocStbHostInBandTunerAGCValue;//TenthdB
        long            vl_ocStbHostInBandTunerSNRValue;
        unsigned long          vl_ocStbHostInBandTunerUnerroreds;
        unsigned long          vl_ocStbHostInBandTunerCorrecteds;
        unsigned long          vl_ocStbHostInBandTunerUncorrectables;
        unsigned long          vl_ocStbHostInBandTunerCarrierLockLost;
        unsigned long          vl_ocStbHostInBandTunerPCRErrors;
        unsigned long          vl_ocStbHostInBandTunerPTSErrors;
                tunerState_t           vl_ocStbHostInBandTunerState;
                tunerBER_t               vl_ocStbHostInBandTunerBER;
                unsigned long          vl_ocStbHostInBandTunerSecsSinceLock;
            long                      vl_ocStbHostInBandTunerEqGain;
            long                   vl_ocStbHostInBandTunerMainTapCoeff;
        unsigned long vl_ocStbHostInBandTunerTotalTuneCount;
                unsigned long vl_ocStbHostInBandTunerTuneFailureCount;
                unsigned long vl_ocStbHostInBandTunerTuneFailFreq;
        TunerBandwidth_t       vl_ocStbHostInBandTunerBandwidth;
	 unsigned long vl_ocStbHostInBandTunerChannelMapId;
	  unsigned long vl_ocStbHostInBandTunerDacId;
} SNMPocStbHostInBandTunerTable_t;



    /*  DVIHDMI   */

/*
 * enums for all column Status "TruthValue" value
 */

 typedef enum {
     STATUS_TRUE =  1,
      STATUS_FALSE = 2
 } Status_t;


/*
 * enums for column ocStbHostDVIHDMIOutputType
 */
typedef enum vl_DVIHDMIOutputType{
    TYPE_DVI = 1,
        TYPE_HDMI= 2
}DVIHDMIOutput_t;
/*
 * enums for column ocStbHostDVIHDMIOutputFormat
 */
typedef enum {
    OUTPUTFORMAT_FORMAT480I=1,
    OUTPUTFORMAT_FORMAT480P=2,
    OUTPUTFORMAT_FORMAT720P=3,
    OUTPUTFORMAT_FORMAT1080I=4,
    OUTPUTFORMAT_FORMAT1080p=5,
    OUTPUTFORMAT_FORMAT2160p=6
} DVIHDMIOutputFormat_t;
/*
 * enums for column ocStbHostDVIHDMIAspectRatio
 */
typedef enum {
    ASPECTRATIO_OTHER=1,
       ASPECTRATIO_FOURBYTHREE=2,
    ASPECTRATIO_SIXTEENBYNINE=3
} DVIHDMIAspectRatio_t;

/*
 * enums for column ocStbHostDVIHDMIAudioFormat
 */

typedef enum {
    OCSTBHOSTDVIHDMIAUDIOFORMAT_OTHER=1,
    OCSTBHOSTDVIHDMIAUDIOFORMAT_LPCM=2,
    OCSTBHOSTDVIHDMIAUDIOFORMAT_AC3=3,
    OCSTBHOSTDVIHDMIAUDIOFORMAT_EAC3=4,
    OCSTBHOSTDVIHDMIAUDIOFORMAT_MPEG1L1L2=5,
    OCSTBHOSTDVIHDMIAUDIOFORMAT_MPEG1L3=6,
    OCSTBHOSTDVIHDMIAUDIOFORMAT_MPEG2=7,
    OCSTBHOSTDVIHDMIAUDIOFORMAT_MPEG4=8,
    OCSTBHOSTDVIHDMIAUDIOFORMAT_DTS=9,
    OCSTBHOSTDVIHDMIAUDIOFORMAT_ATRAC=10
} DVIHDMIAudioFormat_t;
/*
 * enums for column ocStbHostDVIHDMIAudioSampleRate
 */
typedef enum {
    OCSTBHOSTDVIHDMIAUDIOSAMPLERATE_OTHER=1,
    OCSTBHOSTDVIHDMIAUDIOSAMPLERATE_SAMPLERATE32KHZ=2,
    OCSTBHOSTDVIHDMIAUDIOSAMPLERATE_SAMPLERATE44KHZ=3,
    OCSTBHOSTDVIHDMIAUDIOSAMPLERATE_SAMPLERATE48KHZ=4,
    OCSTBHOSTDVIHDMIAUDIOSAMPLERATE_SAMPLERATE88KHZ=5,
    OCSTBHOSTDVIHDMIAUDIOSAMPLERATE_SAMPLERATE96KHZ=6,
    OCSTBHOSTDVIHDMIAUDIOSAMPLERATE_SAMPLERATE176KHZ=7,
    OCSTBHOSTDVIHDMIAUDIOSAMPLERATE_SAMPLERATE192KHZ=8
} DVIHDMIAudioSampleRate_t;
/*
 * enums for column ocStbHostDVIHDMIHostDeviceHDCPStatus
 */
typedef enum{
    HDCPSTATUS_NONHDCPDEVICE    =    1,
    HDCPSTATUS_COMPLIANTHDCPDEVICE    =    2,
    HDCPSTATUS_REVOKEDHDCPDEVICE    =    3
} DVIHDMIHostDeviceHDCPStatus;


typedef enum{
                notValid = 0,
                sample16Bit= 1,
                sample20Bit= 2,
                sample24Bit= 3
} DVIHDMIAudioSampleSize;

typedef enum{
                rgb= 0,
                ycc422= 1,
                ycc444= 2
} DVIHDMIColorSpace;
typedef enum{
             frameRateCode1= 1,
             frameRateCode2= 2,
             frameRateCode3= 3,
             frameRateCode4= 4,
             frameRateCode5= 5,
             frameRateCode6= 6
}DVIHDMIFrameRate;

typedef enum{
    tv = 0,
    recordingDevice = 1,
    tuner = 3,
    playbackDevice = 4,
    audioSystem = 5
}DviHdimiAttDeviceType_t;

typedef enum{
    oneTouchPlay = 0,
    systemStandby = 1,
    oneTouchRecord = 2,
    timerProgramming = 3,
    deckControl = 4,
    tunerControl = 5,
    deviceMenuControl = 6,
    remoteControlPassThrough = 7,
    systemAudioControl = 8,
    deviceOsdNameTransfer = 9,
    devicePowerStatus = 10,
    osdDisplay = 11,
    routingControl = 12,
    systemInformation = 13,
    vendorSpecificCommands = 14,
    audioRateControl = 15
}DVIHDMICecFeatures_t;

typedef enum{
    DVIHDMI3DCompatibilityControl_other = 0,
    DVIHDMI3DCompatibilityControl_passthru3D = 1,
    DVIHDMI3DCompatibilityControl_block3D = 2,
}DVIHDMI3DCompatibilityControlType_t;

typedef struct {
    //unsigned long           DVIHDMIInterfaceIndex;      //index
    DVIHDMIOutput_t        vl_ocStbHostDVIHDMIOutputType;
    Status_t             vl_ocStbHostDVIHDMIConnectionStatus;
    Status_t             vl_ocStbHostDVIHDMIRepeaterStatus;
    Status_t             vl_ocStbHostDVIHDMIVideoXmissionStatus;
    Status_t        vl_ocStbHostDVIHDMIHDCPStatus;
    Status_t                vl_ocStbHostDVIHDMIVideoMuteStatus;
    DVIHDMIOutputFormat_t     vl_ocStbHostDVIHDMIOutputFormat;
    DVIHDMIAspectRatio_t     vl_ocStbHostDVIHDMIAspectRatio;
        DVIHDMIHostDeviceHDCPStatus  vl_ocStbHostDVIHDMIHostDeviceHDCPStatus;
    DVIHDMIAudioFormat_t     vl_ocStbHostDVIHDMIAudioFormat;
    DVIHDMIAudioSampleRate_t vl_ocStbHostDVIHDMIAudioSampleRate;
    unsigned long          vl_ocStbHostDVIHDMIAudioChannelCount;
    Status_t              vl_ocStbHostDVIHDMIAudioMuteStatus;
       DVIHDMIAudioSampleSize     vl_ocStbHostDVIHDMIAudioSampleSize;
       DVIHDMIColorSpace      vl_ocStbHostDVIHDMIColorSpace;
       DVIHDMIFrameRate      vl_ocStbHostDVIHDMIFrameRate;
    DviHdimiAttDeviceType_t       vl_ocStbHostDVIHDMIAttachedDeviceType;
    char            vl_ocStbHostDVIHDMIEdid[3*API_CHRMAX];
    int             vl_ocStbHostDVIHDMIEdid_len;
    long            vl_ocStbHostDVIHDMILipSyncDelay;
    char            vl_ocStbHostDVIHDMICecFeatures[API_CHRMAX]; //BITS
    int             vl_ocStbHostDVIHDMICecFeatures_len;
    char            vl_ocStbHostDVIHDMIFeatures[API_CHRMAX];
    int             vl_ocStbHostDVIHDMIFeatures_len;
    long            vl_ocStbHostDVIHDMIMaxDeviceCount;
    long         vl_ocStbHostDVIHDMIPreferredVideoFormat;
    char         vl_ocStbHostDVIHDMIEdidVersion[API_CHRMAX];
    size_t        vl_ocStbHostDVIHDMIEdidVersion_len;
    DVIHDMI3DCompatibilityControlType_t vl_ocStbHostDVIHDMI3DCompatibilityControl;
    Status_t      vl_ocStbHostDVIHDMI3DCompatibilityMsgDisplay;
} SNMPocStbHostDVIHDMITable_t;
/* ocStbHostDVIHDMIVideoFormatTable  */
typedef struct SNMPocStbHostDVIHDMIVideoFormatTable{
    /* Index values */
    long vl_ocStbHostDVIHDMIAvailableVideoFormatIndex;
    /* Column values */
    long vl_ocStbHostDVIHDMIAvailableVideoFormat;
    char vl_ocStbHostDVIHDMISupported3DStructures[API_CHRMAX];
    long vl_ocStbHostDVIHDMISupported3DStructures_len;
    long vl_ocStbHostDVIHDMIActive3DStructure;

}SNMPocStbHostDVIHDMIVideoFormatTable_t;

/*
 * enums for column ocStbHostComponentVideoConstrainedStatus
 */
// typedef enum{
//     OCSTBHOSTCOMPONENTVIDEOCONSTRAINEDSTATUS_TRUE    =    1,
//     OCSTBHOSTCOMPONENTVIDEOCONSTRAINEDSTATUS_FALSE    =    2
// }ComponentVideoConstrainedStatus_t;

/*
 * enums for column ocStbHostComponentOutputFormat
 */
typedef enum {
    COMPONENTOUTPUTFORMAT_FORMAT480I = 1,
    COMPONENTOUTPUTFORMAT_FORMAT480P = 2,
    COMPONENTOUTPUTFORMAT_FORMAT720P = 3,
    COMPONENTOUTPUTFORMAT_FORMAT1080I = 4,
    COMPONENTOUTPUTFORMAT_FORMAT1080P = 5,
    COMPONENTOUTPUTFORMAT_FORMAT2160P = 6
} ComponentOutputFormat_t;
/*
 * enums for column ocStbHostComponentAspectRatio
 */
typedef enum {
    OCSTBHOSTCOMPONENTASPECTRATIO_OTHER  =    1,
     OCSTBHOSTCOMPONENTASPECTRATIO_FOURBYTHREE =     2,
     OCSTBHOSTCOMPONENTASPECTRATIO_SIXTEENBYNINE = 3
} ComponentAspectRatio_t;


typedef struct {
    //unsigned long             CompnentVedioInterfaceIndex;
    Status_t                vl_ocStbHostComponentVideoConstrainedStatus;
    ComponentOutputFormat_t   vl_ocStbHostComponentOutputFormat;
    ComponentAspectRatio_t    vl_ocStbHostComponentAspectRatio;
    Status_t                vl_ocStbHostComponentVideoMuteStatus;
} SNMPocStbHostComponentVideoTable_t;



  typedef struct {
    //unsigned long   RFChannelOutAvInterfaceIx;          //index
    unsigned long   vl_ocStbHostRFChannelOut;                 // Integer32
    Status_t        vl_ocStbHostRFChannelOutAudioMuteStatus; //TruthValue      ::enum value
    Status_t        vl_ocStbHostRFChannelOutVideoMuteStatus; //TruthValue      ::enum value
} SNMPocStbHostRFChannelOutTable_t;

/*
 * enums for column ocStbHostAnalogVideoProtectionStatus
 */
typedef enum {
    STATUS_COPYPROTECTIONOFF =    0,
    STATUS_SPLITBURSTOFF     =    1,
    STATUS_TWOLINESPLITBURST =    2,
    STATUS_FOURLINESPLITBURST =    3
} AnalogVideoProtectionStatus_t;

   typedef struct {
    //unsigned long                         AnalogVideoIndex;
    AnalogVideoProtectionStatus_t         vl_ocStbHostAnalogVideoProtectionStatus;
} SNMPocStbHostAnalogVideoTable_t;

/* SNMPocStbHostProgramStatusTable */
typedef struct {
    unsigned long         ProgramStatusIndex;
    avType_t              vl_ocStbHostProgramAVSource;//[API_MAX_IOD_LENGTH];
    avType_t              vl_ocStbHostProgramAVDestination;//[API_MAX_IOD_LENGTH];
    apioid              vl_ocStbHostProgramContentSource[API_MAX_IOD_LENGTH];//[API_MAX_IOD_LENGTH];
    apioid              vl_ocStbHostProgramContentDestination[API_MAX_IOD_LENGTH];//[API_MAX_IOD_LENGTH];
    int vl_ocStbHostProgramContentSource_len ;
    int vl_ocStbHostProgramContentDestination_len;

} SNMPocStbHostProgramStatusTable_t;


typedef enum{
        SnmpMediaEngineMPVideo,  // Playback of MPEG video +/- audio .. Streaming video
        SnmpMediaEngineAVPlayBack,
        SnmpMediaEngineEthernet,
        SnmpMediaEngineBD,
        SnmpMediaEngineDVD,
        SnmpMediaEngineMemory,
    SnmpMediaEngineTuner,
    SnmpMediaEngineAnalogInput,
    SnmpMediaEngineDVR
}SnmpMediaType_t;

#define PORT_ONE_INFO 0
#define PORT_TWO_INFO 1
typedef struct{

    bool IEEE1394Stream;
    int  port; //they are only two ports port 0 and port 1
}Ieee1394PgInfo_t;
typedef struct{
    unsigned long physicalSrcId;
    int pcrPid;
    int videoPid;
    int audioPid;
    int pgstatus;
    int activepgnumber;
    /*Decoder type*/
    SnmpMediaType_t mediaType;//disc or 1394 or dvr
    bool snmpVDecoder;   //tuner flow to vedio ports
    bool snmpADecoder;   //tuner flow to audio ports
    Ieee1394PgInfo_t Ieee1394DestPgInfo;

} programInfo;


/*
 * enums for column ocStbHostMpeg2ContentCCIValue
 */
typedef enum {
    OCSTBHOSTMPEG2CONTENTCCIVALUE_COPYFREELY    =    0,
    OCSTBHOSTMPEG2CONTENTCCIVALUE_COPYNOMORE    =    1,
    OCSTBHOSTMPEG2CONTENTCCIVALUE_COPYONEGENERATION =    2,
    OCSTBHOSTMPEG2CONTENTCCIVALUE_COPYNEVER        =    3,
    OCSTBHOSTMPEG2CONTENTCCIVALUE_UNDEFINED        =    4
} Mpeg2ContentCCIValue_t;

/*
 * enums for column ocStbHostMpeg2ContentAPSValue
 */
typedef enum {
    OCSTBHOSTMPEG2CONTENTAPSVALUE_TYPE1        =    1,
    OCSTBHOSTMPEG2CONTENTAPSVALUE_TYPE2        =    2,
    OCSTBHOSTMPEG2CONTENTAPSVALUE_TYPE3        =    3,
    OCSTBHOSTMPEG2CONTENTAPSVALUE_NOMACROVISION    =    4,
    OCSTBHOSTMPEG2CONTENTAPSVALUE_NOTDEFINED    =    5
} Mpeg2ContentAPSValue_t;

/*
 * enums for column ocStbHostMpeg2ContainerFormat
 */
typedef enum {
    OCSTBHOSTMPEG2CONTAINERFORMAT_OTHER        =    1,
    OCSTBHOSTMPEG2CONTAINERFORMAT_MPEG2        =    2,
    OCSTBHOSTMPEG2CONTAINERFORMAT_MPEG4        =    3,
} OCSTBHOSTMPEG2CONTAINERFORMAT_t;

/*
 * enums for column ocStbHostMpeg2ContentPCRLockStatus
 */
typedef enum {
    MPEG2_PCRLOCKSTATUS_NOTLOCKED     =    1,
    MPEG2_PCRLOCKSTATUS_LOCKED   =  2
} Mpeg2ContentPCRLockStatus_t;

 typedef struct {
    unsigned long           Mpeg2ContentIndex;                /*Range: 1..20*/
    unsigned long           vl_ocStbHostMpeg2ContentProgramNumber;
    unsigned long           vl_ocStbHostMpeg2ContentTransportStreamID;
    unsigned long           vl_ocStbHostMpeg2ContentTotalStreams;
    long                    vl_ocStbHostMpeg2ContentSelectedVideoPID;
    long                    vl_ocStbHostMpeg2ContentSelectedAudioPID;/* Range: -1 | 1..8191*/
    Status_t                vl_ocStbHostMpeg2ContentOtherAudioPIDs;             //TruthValue
    Mpeg2ContentCCIValue_t    vl_ocStbHostMpeg2ContentCCIValue;
    Mpeg2ContentAPSValue_t    vl_ocStbHostMpeg2ContentAPSValue;
    Status_t                vl_ocStbHostMpeg2ContentCITStatus;                //TruthValue
    Status_t                vl_ocStbHostMpeg2ContentBroadcastFlagStatus;
    Status_t                vl_ocStbHostMpeg2ContentEPNStatus;
    long                    vl_ocStbHostMpeg2ContentPCRPID;
    Mpeg2ContentPCRLockStatus_t vl_ocStbHostMpeg2ContentPCRLockStatus;
    unsigned long           vl_ocStbHostMpeg2ContentDecoderPTS;
    unsigned long           vl_ocStbHostMpeg2ContentDiscontinuities;
    unsigned long           vl_ocStbHostMpeg2ContentPktErrors;
    unsigned long           vl_ocStbHostMpeg2ContentPipelineErrors;
    unsigned long           vl_ocStbHostMpeg2ContentDecoderRestarts;
    OCSTBHOSTMPEG2CONTAINERFORMAT_t vl_ocStbHostMpeg2ContainerFormat;

} SNMPocStbHostMpeg2ContentTable_t;

/*
 * enums for column ocStbHostMpeg4ContentCCIValue
 */
typedef enum {
    OCSTBHOSTMPEG4CONTENTCCIVALUE_COPYFREELY    =    0,
    OCSTBHOSTMPEG4CONTENTCCIVALUE_COPYNOMORE    =    1,
    OCSTBHOSTMPEG4CONTENTCCIVALUE_COPYONEGENERATION =    2,
    OCSTBHOSTMPEG4CONTENTCCIVALUE_COPYNEVER        =    3,
    OCSTBHOSTMPEG4CONTENTCCIVALUE_UNDEFINED        =    4
} Mpeg4ContentCCIValue_t;

/*
 * enums for column ocStbHostMpeg4ContentAPSValue
 */
typedef enum {
    OCSTBHOSTMPEG4CONTENTAPSVALUE_TYPE1        =    1,
    OCSTBHOSTMPEG4CONTENTAPSVALUE_TYPE2        =    2,
    OCSTBHOSTMPEG4CONTENTAPSVALUE_TYPE3        =    3,
    OCSTBHOSTMPEG4CONTENTAPSVALUE_NOMACROVISION    =    4,
    OCSTBHOSTMPEG4CONTENTAPSVALUE_NOTDEFINED    =    5
} Mpeg4ContentAPSValue_t;

/*
 * enums for column ocStbHostMpeg4ContainerFormat
 */
typedef enum {
    OCSTBHOSTMPEG4CONTAINERFORMAT_OTHER        =    1,
    OCSTBHOSTMPEG4CONTAINERFORMAT_MPEG2        =    2,
    OCSTBHOSTMPEG4CONTAINERFORMAT_MPEG4        =    3,
} OCSTBHOSTMPEG4CONTAINERFORMAT_t;

/*
 * enums for column ocStbHostMpeg4ContentPCRLockStatus
 */
typedef enum {
    MPEG4_PCRLOCKSTATUS_NOTLOCKED     =    1,
    MPEG4_PCRLOCKSTATUS_LOCKED   =  2
} Mpeg4ContentPCRLockStatus_t;

 typedef struct {
    unsigned long           Mpeg4ContentIndex;                /*Range: 1..20*/
    unsigned long           vl_ocStbHostMpeg4ContentProgramNumber;
    unsigned long           vl_ocStbHostMpeg4ContentTransportStreamID;
    unsigned long           vl_ocStbHostMpeg4ContentTotalStreams;
    long                    vl_ocStbHostMpeg4ContentSelectedVideoPID;
    long                    vl_ocStbHostMpeg4ContentSelectedAudioPID;/* Range: -1 | 1..8191*/
    Status_t                vl_ocStbHostMpeg4ContentOtherAudioPIDs;             //TruthValue
    Mpeg4ContentCCIValue_t    vl_ocStbHostMpeg4ContentCCIValue;
    Mpeg4ContentAPSValue_t    vl_ocStbHostMpeg4ContentAPSValue;
    Status_t                vl_ocStbHostMpeg4ContentCITStatus;                //TruthValue
    Status_t                vl_ocStbHostMpeg4ContentBroadcastFlagStatus;
    Status_t                vl_ocStbHostMpeg4ContentEPNStatus;
    long                    vl_ocStbHostMpeg4ContentPCRPID;
    Mpeg4ContentPCRLockStatus_t vl_ocStbHostMpeg4ContentPCRLockStatus;
    unsigned long           vl_ocStbHostMpeg4ContentDecoderPTS;
    unsigned long           vl_ocStbHostMpeg4ContentDiscontinuities;
    unsigned long           vl_ocStbHostMpeg4ContentPktErrors;
    unsigned long           vl_ocStbHostMpeg4ContentPipelineErrors;
    unsigned long           vl_ocStbHostMpeg4ContentDecoderRestarts;
    OCSTBHOSTMPEG4CONTAINERFORMAT_t vl_ocStbHostMpeg4ContainerFormat;

} SNMPocStbHostMpeg4ContentTable_t;

/*
 * enums for column ocStbHostVc1ContainerFormat
 */
typedef enum {
    OCSTBHOSTVC1CONTAINERFORMAT_OTHER        =    1,
    OCSTBHOSTVC1CONTAINERFORMAT_MPEG2        =    2,
    OCSTBHOSTVC1CONTAINERFORMAT_MPEG4        =    3,
} OCSTBHOSTVC1CONTAINERFORMAT_t;

 typedef struct {
    unsigned long           Vc1ContentIndex;                /*Range: 1..20*/
    unsigned long           vl_ocStbHostVc1ContentProgramNumber;
    OCSTBHOSTVC1CONTAINERFORMAT_t vl_ocStbHostVc1ContainerFormat;

} SNMPocStbHostVc1ContentTable_t;

/*
 * enums for column ocStbHostSPDIfAudioFormat
 */
typedef enum {
    OCSTBHOSTSPDIFAUDIOFORMAT_OTHER        =    1,
    OCSTBHOSTSPDIFAUDIOFORMAT_LPCM      =    2,
    OCSTBHOSTSPDIFAUDIOFORMAT_AC3        =    3,
    OCSTBHOSTSPDIFAUDIOFORMAT_EAC3        =    4,
    OCSTBHOSTSPDIFAUDIOFORMAT_MPEG1L1L2 =    5,
    OCSTBHOSTSPDIFAUDIOFORMAT_MPEG1L3   =    6,
    OCSTBHOSTSPDIFAUDIOFORMAT_MPEG2        =    7,
    OCSTBHOSTSPDIFAUDIOFORMAT_MPEG4        =    8,
    OCSTBHOSTSPDIFAUDIOFORMAT_DTS        =    9,
    OCSTBHOSTSPDIFAUDIOFORMAT_ATRAC        =    10
} SPDIfAudioFormat_t;

typedef struct {
    unsigned long           SPDIfInterfaceIndex;          // Av index index
    SPDIfAudioFormat_t    vl_ocStbHostSPDIfAudioFormat;      // AudioOutputFormat ::enum value
    Status_t         vl_ocStbHostSPDIfAudioMuteStatus;  // TruthValue        ::enum value
} SNMPocStbHostSPDIfTable_t;



typedef struct {

    //unsigned long       vl_AvIEEE1394Index;
    long                vl_ocStbHostIEEE1394ActiveNodes;
//    long             vl_ActiveNodeID;
    Status_t            vl_ocStbHostIEEE1394DataXMission;
    Status_t            vl_ocStbHostIEEE1394DTCPStatus;
    Status_t            vl_ocStbHostIEEE1394LoopStatus;
    Status_t            vl_ocStbHostIEEE1394RootStatus;
    Status_t            vl_ocStbHostIEEE1394CycleIsMaster;
    Status_t            vl_ocStbHostIEEE1394IRMStatus;
    Status_t            vl_ocStbHostIEEE1394AudioMuteStatus;
    Status_t            vl_ocStbHostIEEE1394VideoMuteStatus;

} IEEE1394Table_t;

/*
 * enums for column ocStbHostIEEE1394ConnectedDevicesSubUnitType
 */
typedef enum {
        SUBUNITTYPE_MONITOR    =    0,
        SUBUNITTYPE_AUDIO    =    1,
    SUBUNITTYPE_PRINTER    =    2,
    SUBUNITTYPE_DISC    =    3,
    SUBUNITTYPE_TAPE    =    4,
    SUBUNITTYPE_TUNER    =    5,
    SUBUNITTYPE_CA        =    6,
    SUBUNITTYPE_CAMERA    =    7,
    SUBUNITTYPE_RESERVED    =    8,
    SUBUNITTYPE_PANEL    =    9,
    SUBUNITTYPE_OTHER    =    10
} ConnectedDevicesSubUnitType_t;
/*
 * enums for column ocStbHostIEEE1394ConnectedDevicesADSourceSelectSupport
 */
typedef enum{
    ADSOURCESELECTSUPPORT_TRUE    =    1,
    ADSOURCESELECTSUPPORT_FALSE    =    2
} ADSourceSelectSupport_t;

typedef struct  {

    //unsigned long    vl_ocStbHostIEEE1394ConnectedDevicesIndex; It will be matained vl_ocStbHost_GetData.cpp
   // unsigned long    vl_ocStbHostIEEE1394ConnectedDevicesAVInterfaceIndex;
    ConnectedDevicesSubUnitType_t                                                            vl_ocStbHostIEEE1394ConnectedDevicesSubUnitType;
    char             vl_ocStbHostIEEE1394ConnectedDevicesEui64[API_CHRMAX];
    ADSourceSelectSupport_t                                                        vl_ocStbHostIEEE1394ConnectedDevicesADSourceSelectSupport;
} IEEE1394ConnectedDevices_t;


/**  ocStbHostSystemTempTable  */
/* optional to implement this table according to spec*/
typedef struct{
   // long            hrDeviceIndex;
    //unsigned long          vl_ocStbHostSystemTempIndex;
    char            vl_ocStbHostSystemTempDescr[API_CHRMAX];
    int          vl_ocStbHostSystemTempDescr_len;
    long            vl_ocStbHostSystemTempValue;
    unsigned long          vl_ocStbHostSystemTempLastUpdate;
    long           vl_ocStbHostSystemTempMaxValue;

} SNMPocStbHostSystemTempTable_t;
/**  ocStbHostSystemHomeNetworkTable  */
typedef struct {
    /* Index values  */
   // u_long          vl_ocStbHostSystemHomeNetworkIndex;
   /* Column values  */
    long     vl_ocStbHostSystemHomeNetworkMaxClients;
    long     vl_ocStbHostSystemHomeNetworkHostDRMStatus;
    long     vl_ocStbHostSystemHomeNetworkConnectedClients;
    char         vl_ocStbHostSystemHomeNetworkClientMacAddress[API_CHRMAX];
    int       vl_ocStbHostSystemHomeNetworkClientMacAddress_len;
    char             vl_ocStbHostSystemHomeNetworkClientIpAddress[API_CHRMAX];
    int       vl_ocStbHostSystemHomeNetworkClientIpAddress_len;
    long       vl_ocStbHostSystemHomeNetworkClientDRMStatus;

} SNMPocStbHostSystemHomeNetworkTable_t;

/**  ocStbHostSystemMemoryReportTable  */
        /*
 * enums for column ocStbHostSystemMemoryReportMemoryType
 */

typedef enum {
                SystemMemorytype_rom = 1,
                SystemMemorytype_dram= 2,
                SystemMemorytype_sram= 3,
                SystemMemorytype_flash= 4,
                SystemMemorytype_nvm = 5,
                SystemMemorytype_videomemory= 7,
                SystemMemorytype_othermemory= 8,
                SystemMemorytype_reserved = 9,
                SystemMemorytype_internalHardDrive = 10,
                SystemMemorytype_externalHardDrive = 11,
                SystemMemorytype_opticalMedia = 12
}SystemMemorytype;

typedef struct {
    /* Index values */
   // unsigned long          vl_ocStbHostSystemMemoryReportIndex;
    /* Column values */
    SystemMemorytype            vl_ocStbHostSystemMemoryReportMemoryType;
    long            vl_ocStbHostSystemMemoryReportMemorySize;

} SNMPocStbHostSystemMemoryReportTable_t;
/** ocStbHostCCAppInfoTable  */
typedef struct {
    /* Index values  */
  //  unsigned long          vl_ocStbHostCCAppInfoIndex;
    /* Column values */
    unsigned long          vl_ocStbHostCCApplicationType;
    char            vl_ocStbHostCCApplicationName[API_CHRMAX];
    int          vl_ocStbHostCCApplicationName_len;
    unsigned long          vl_ocStbHostCCApplicationVersion;
    char            vl_ocStbHostCCAppInfoPage[API_CHRMAX*4];
    int          vl_ocStbHostCCAppInfoPage_len;
    int         vl_ocStbHostCCAppInfoPage_nSubPages;

} SNMPocStbHostCCAppInfoTable_t;


typedef enum {
      loaded = 4,
    notLoaded = 5,
    paused = 6,
    running = 7,
    destroyed = 8
}SoftwareAPP_status;

typedef enum {
    SoftwareAPP_SigStatus_other = 0,
    SoftwareAPP_SigStatus_okay = 1,
    SoftwareAPP_SigStatus_error = 2
}SoftwareAPP_SigStatus;

/** ocStbHostSoftwareApplicationInfoTable */
#define SORG_ID 4
#define SAID 2
typedef struct  {
    /* Index values  */
    unsigned long           vl_ocStbHostSoftwareApplicationInfoIndex;
    /* Column values */
    char                    vl_ocStbHostSoftwareAppNameString[API_CHRMAX];
    int                     vl_ocStbHostSoftwareAppNameString_len;
    char                    vl_ocStbHostSoftwareAppVersionNumber[API_CHRMAX];
    int                     vl_ocStbHostSoftwareAppVersionNumber_len;
    SoftwareAPP_status      vl_ocStbHostSoftwareStatus;
    char                    vl_ocStbHostSoftwareOrganizationId[SORG_ID]; //4-BYTES
    char                    vl_ocStbHostSoftwareApplicationId[SAID];  //2-BYTES
    SoftwareAPP_SigStatus   vl_ocStbHostSoftwareApplicationSigStatus;
    char                vl_ocStbHostSoftwareApplicationInfoSigLastReceivedTime[API_CHRMAX];//last time an XAIT was received
    size_t                  vl_ocStbHostSoftwareApplicationInfoSigLastReadStatus;//status of the last attempted read of the  XAIT


} SNMPocStbHostSoftwareApplicationInfoTable_t;

typedef struct  {
    /* Index values  */
    unsigned long           vl_ocStbHostSoftwareApplicationInfoIndex;
    /* Column values */
    char                    vl_ocStbHostSoftwareAppNameString[API_CHRMAX];
    int                     vl_ocStbHostSoftwareAppNameString_len;
    char                    vl_ocStbHostSoftwareAppVersionNumber[API_CHRMAX];
    int                     vl_ocStbHostSoftwareAppVersionNumber_len;
    SoftwareAPP_status      vl_ocStbHostSoftwareStatus;
    int                     vl_ocStbHostSoftwareOrganizationId; //4-BYTES
    int                     vl_ocStbHostSoftwareApplicationId;  //2-BYTES
    char                    vl_ocStbHostSoftwareApplicationSigStatus[API_CHRMAX];
} SNMPHOSTSoftwareApplicationInfoTable_t;
/*
 * enums for column ifType
 */
typedef enum{

 IFTYPE_OTHER     =    1,
 IFTYPE_REGULAR1822    =    2,
 IFTYPE_HDH1822    =    3,
 IFTYPE_DDNX25    =    4,
 IFTYPE_RFC877X25    =    5,
 IFTYPE_ETHERNETCSMACD    =    6,
 IFTYPE_ISO88023CSMACD    =    7,
 IFTYPE_ISO88024TOKENBUS    =    8,
 IFTYPE_ISO88025TOKENRING    =    9,
 IFTYPE_ISO88026MAN        = 10,
 IFTYPE_STARLAN        =11,
 IFTYPE_PROTEON10MBIT    =    12,
 IFTYPE_PROTEON80MBIT    =    13,
 IFTYPE_HYPERCHANNEL    =    14,
 IFTYPE_FDDI        = 15,
 IFTYPE_LAPB        =16,
 IFTYPE_SDLC        =17,
 IFTYPE_DS1        =18,
 IFTYPE_E1        =19,
 IFTYPE_BASICISDN    =    20,
 IFTYPE_PRIMARYISDN    =    21,
 IFTYPE_PROPPOINTTOPOINTSERIAL    =    22,
 IFTYPE_PPP    =    23,
 IFTYPE_SOFTWARELOOPBACK     =    24,
 IFTYPE_EON    =    25,
 IFTYPE_ETHERNET3MBIT    =    26,
 IFTYPE_NSIP    =    27,
 IFTYPE_SLIP    =    28,
 IFTYPE_ULTRA    =    29,
 IFTYPE_DS3    =    30,
 IFTYPE_SIP    =    31,
 IFTYPE_FRAMERELAY    =    32,
 IFTYPE_RS232    =    33,
 IFTYPE_PARA    =    34,
 IFTYPE_ARCNET    =    35,
 IFTYPE_ARCNETPLUS    =    36,
 IFTYPE_ATM    =    37,
 IFTYPE_MIOX25    =    38,
 IFTYPE_SONET    =    39,
 IFTYPE_X25PLE    =    40,
 IFTYPE_ISO88022LLC    =    41,
 IFTYPE_LOCALTALK    =    42,
 IFTYPE_SMDSDXI    =    43,
 IFTYPE_FRAMERELAYSERVICE    =    44,
 IFTYPE_V35    =    45,
 IFTYPE_HSSI    =    46,
 IFTYPE_HIPPI    =    47,
 IFTYPE_MODEM    =    48,
 IFTYPE_AAL5    =    49,
 IFTYPE_SONETPATH    =    50,
 IFTYPE_SONETVT        =51,
 IFTYPE_SMDSICIP        =52,
 IFTYPE_PROPVIRTUAL        =53,
 IFTYPE_PROPMULTIPLEXOR        =54,
 IFTYPE_IEEE80212        =55,
 IFTYPE_FIBRECHANNEL        =56,
 IFTYPE_HIPPIINTERFACE        =57,
 IFTYPE_FRAMERELAYINTERCONNECT        =58,
 IFTYPE_AFLANE8023        =59,
 IFTYPE_AFLANE8025        =60,
 IFTYPE_CCTEMUL        =61,
 IFTYPE_FASTETHER        =62,
 IFTYPE_ISDN        =63,
 IFTYPE_V11        =64,
 IFTYPE_V36        =65,
 IFTYPE_G703AT64K        =66,
 IFTYPE_G703AT2MB        =67,
 IFTYPE_QLLC        =68,
 IFTYPE_FASTETHERFX        =69,
 IFTYPE_CHANNEL        =70,
 IFTYPE_IEEE80211        =71,
 IFTYPE_IBM370PARCHAN        =72,
 IFTYPE_ESCON        =73,
 IFTYPE_DLSW        =74,
 IFTYPE_ISDNS        =75,
 IFTYPE_ISDNU        =76,
 IFTYPE_LAPD        =77,
 IFTYPE_IPSWITCH        =78,
 IFTYPE_RSRB        =79,
 IFTYPE_ATMLOGICAL        =80,
 IFTYPE_DS0        =81,
 IFTYPE_DS0BUNDLE        =82,
 IFTYPE_BSC        =83,
 IFTYPE_ASYNC        =84,
 IFTYPE_CNR        =85,
 IFTYPE_ISO88025DTR        =86,
 IFTYPE_EPLRS        =87,
 IFTYPE_ARAP        =88,
 IFTYPE_PROPCNLS        =89,
 IFTYPE_HOSTPAD        =90,
 IFTYPE_TERMPAD        =91,
 IFTYPE_FRAMERELAYMPI        =92,
 IFTYPE_X213        =93,
 IFTYPE_ADSL        =94,
 IFTYPE_RADSL        =95,
 IFTYPE_SDSL        =96,
 IFTYPE_VDSL        =97,
 IFTYPE_ISO88025CRFPINT        =98,
 IFTYPE_MYRINET        =99,
 IFTYPE_VOICEEM        =100,
 IFTYPE_VOICEFXO        =101,
 IFTYPE_VOICEFXS        =102,
 IFTYPE_VOICEENCAP        =103,
 IFTYPE_VOICEOVERIP        =104,
 IFTYPE_ATMDXI        =105,
 IFTYPE_ATMFUNI        =106,
 IFTYPE_ATMIMA        =107,
 IFTYPE_PPPMULTILINKBUNDLE        =108,
 IFTYPE_IPOVERCDLC        =109,
 IFTYPE_IPOVERCLAW        =110,
 IFTYPE_STACKTOSTACK        =111,
 IFTYPE_VIRTUALIPADDRESS        =112,
 IFTYPE_MPC        =113,
 IFTYPE_IPOVERATM        =114,
 IFTYPE_ISO88025FIBER        =115,
 IFTYPE_TDLC        =116,
 IFTYPE_GIGABITETHERNET        =117,
 IFTYPE_HDLC        =118,
 IFTYPE_LAPF        =119,
 IFTYPE_V37        =120,
 IFTYPE_X25MLP        =121,
 IFTYPE_X25HUNTGROUP        =122,
 IFTYPE_TRASNPHDLC        =123,
 IFTYPE_INTERLEAVE        =124,
 IFTYPE_FAST        =125,
 IFTYPE_IP        =126,
 IFTYPE_DOCSCABLEMACLAYER        =127,
 IFTYPE_DOCSCABLEDOWNSTREAM    =    128,
 IFTYPE_DOCSCABLEUPSTREAM    =    129,
 IFTYPE_A12MPPSWITCH        =130,
 IFTYPE_TUNNEL        =131,
 IFTYPE_COFFEE        =132,
 IFTYPE_CES        =133,
 IFTYPE_ATMSUBINTERFACE        =134,
 IFTYPE_L2VLAN        =135,
 IFTYPE_L3IPVLAN        =136,
 IFTYPE_L3IPXVLAN    =    137,
 IFTYPE_DIGITALPOWERLINE        =138,
 IFTYPE_MEDIAMAILOVERIP        =139,
 IFTYPE_DTM        =140,
 IFTYPE_DCN        =141,
 IFTYPE_IPFORWARD        =142,
 IFTYPE_MSDSL        =143,
 IFTYPE_IEEE1394        =144,
 IFTYPE_IF_GSN        =145,
 IFTYPE_DVBRCCMACLAYER        =146,
 IFTYPE_DVBRCCDOWNSTREAM        =147,
 IFTYPE_DVBRCCUPSTREAM        =148,
 IFTYPE_ATMVIRTUAL        =149,
 IFTYPE_MPLSTUNNEL        =150,
 IFTYPE_SRP        =151,
 IFTYPE_VOICEOVERATM        =152,
 IFTYPE_VOICEOVERFRAMERELAY        =153,
 IFTYPE_IDSL        =154,
 IFTYPE_COMPOSITELINK        =155,
 IFTYPE_SS7SIGLINK        =156,
 IFTYPE_PROPWIRELESSP2P        =157,
 IFTYPE_FRFORWARD        =158,
 IFTYPE_RFC1483        =159,
 IFTYPE_USB        =160,
 IFTYPE_IEEE8023ADLAG        =161,
 IFTYPE_BGPPOLICYACCOUNTING        =162,
 IFTYPE_FRF16MFRBUNDLE        =163,
 IFTYPE_H323GATEKEEPER        =164,
 IFTYPE_H323PROXY        =165,
 IFTYPE_MPLS        =166,
 IFTYPE_MFSIGLINK        =167,
 IFTYPE_HDSL2        =168,
 IFTYPE_SHDSL        =169,
 IFTYPE_DS1FDL        =170,
 IFTYPE_POS        =171,
 IFTYPE_DVBASIIN        =172,
 IFTYPE_DVBASIOUT        =173,
 IFTYPE_PLC        =174,
 IFTYPE_NFAS        =175,
 IFTYPE_TR008        =176,
 IFTYPE_GR303RDT        =177,
 IFTYPE_GR303IDT        =178,
 IFTYPE_ISUP        =179,
 IFTYPE_PROPDOCSWIRELESSMACLAYER        =180,
 IFTYPE_PROPDOCSWIRELESSDOWNSTREAM        =181,
 IFTYPE_PROPDOCSWIRELESSUPSTREAM        =182,
 IFTYPE_HIPERLAN2        =183,
 IFTYPE_PROPBWAP2MP        =184,
 IFTYPE_SONETOVERHEADCHANNEL        =185,
 IFTYPE_DIGITALWRAPPEROVERHEADCHANNEL        =186,
 IFTYPE_AAL2        =187,
 IFTYPE_RADIOMAC        =188,
 IFTYPE_ATMRADIO        =189,
 IFTYPE_IMT        =190,
 IFTYPE_MVL        =191,
 IFTYPE_REACHDSL        =192,
 IFTYPE_FRDLCIENDPT        =193,
 IFTYPE_ATMVCIENDPT        =194,
 IFTYPE_OPTICALCHANNEL        =195,
 IFTYPE_OPTICALTRANSPORT        =196,
 IFTYPE_PROPATM        =197,
 IFTYPE_VOICEOVERCABLE        =198,
 IFTYPE_INFINIBAND        =199,
 IFTYPE_TELINK        =200,
 IFTYPE_Q2931        =201,
 IFTYPE_VIRTUALTG        =202,
 IFTYPE_SIPTG        =203,
 IFTYPE_SIPSIG        =204,
 IFTYPE_DOCSCABLEUPSTREAMCHANNEL    =    205,
 IFTYPE_ECONET        =206,
 IFTYPE_PON155        =207,
 IFTYPE_PON622        =208,
 IFTYPE_BRIDGE        =209,
 IFTYPE_LINEGROUP        =210,
 IFTYPE_VOICEEMFGD        =211,
 IFTYPE_VOICEFGDEANA        =212,
 IFTYPE_VOICEDID        =213,
 IFTYPE_MPEGTRANSPORT        =214,
 IFTYPE_SIXTOFOUR        =215,
 IFTYPE_GTP        =216,
 IFTYPE_PDNETHERLOOP1        =217,
 IFTYPE_PDNETHERLOOP2        =218,
 IFTYPE_OPTICALCHANNELGROUP        =219,
 IFTYPE_HOMEPNA        =220,
 IFTYPE_GFP        =221,
 IFTYPE_CISCOISLVLAN        =222,
 IFTYPE_ACTELISMETALOOP        =223,
 IFTYPE_FCIPLINK        =224,
 IFTYPE_RPR        =225,
 IFTYPE_QAM        =226,
 IFTYPE_LMP        =227,
 IFTYPE_CBLVECTASTAR        =228,
 IFTYPE_DOCSCABLEMCMTSDOWNSTREAM    =    229,
 IFTYPE_ADSL2        =230,
 IFTYPE_MACSECCONTROLLEDIF = 231,
 IFTYPE_MACSECUNCONTROLLEDIF = 232,
 IFTYPE_AVICIOPTICALETHER = 233,
 IFTYPE_ATMBOND = 234,
 IFTYPE_VOICEFGDOS = 235,
 IFTYPE_MOCAVERSION1 = 236,
 IFTYPE_IEEE80216WMAN = 237,
 IFTYPE_ADSL2PLUS = 238,
 IFTYPE_DVBRCSMACLAYER = 239,
 IFTYPE_DVBTDM = 240,
 IFTYPE_DVBRCSTDMA = 241,
 IFTYPE_X86LAPS = 242,
 IFTYPE_WWANPP = 243,
 IFTYPE_WWANPP2 = 244,
 IFTYPE_VOICEEBS = 245,
 IFTYPE_IFPWTYPE = 246,
 IFTYPE_ILAN = 247,
 IFTYPE_PIP = 248,
 IFTYPE_ALUELP = 249,
 IFTYPE_GPON = 250,
 IFTYPE_VDSL2 = 251,
 IFTYPE_CAPWAPDOT11PROFILE = 252,
 IFTYPE_CAPWAPDOT11BSS = 253,
 IFTYPE_CAPWAPWTPVIRTUALRADIO = 254,
 IFTYPE_BITS = 255,
 IFTYPE_DOCSCABLEUPSTREAMRFPORT = 256,
 IFTYPE_CABLEDOWNSTREAMRFPORT = 257,
 IFTYPE_VMWAREVIRTUALNIC = 258,
 IFTYPE_IEEE802154 = 259,
 IFTYPE_OTNODU = 260,
 IFTYPE_OTNOTU = 261,
 IFTYPE_IFVFITYPE = 262,
 IFTYPE_G9981 = 263,
 IFTYPE_G9982 = 264,
 IFTYPE_G9983 = 265,
 IFTYPE_ALUEPON = 266,
 IFTYPE_ALUEPONONU = 267,
 IFTYPE_ALUEPONPHYSICALUNI = 268,
 IFTYPE_ALUEPONLOGICALLINK = 269,
 IFTYPE_ALUGPONONU = 270,
 IFTYPE_ALUGPONPHYSICALUNI = 271,
 IFTYPE_VMWARENICTEAM = 272,
 IFTYPE_DOCSOFDMDOWNSTREAM = 277,
 IFTYPE_DOCSOFDMAUPSTREAM = 278,
 IFTYPE_GFAST = 279,
 IFTYPE_SDCI = 280,
} IFtype_t;
/*
 * enums for column ifAdminStatus
 */
typedef enum {
 IFADMINSTATUS_UP       =    1,
 IFADMINSTATUS_DOWN    =    2,
 IFADMINSTATUS_TESTING    =    3
}IFadminstatus_t;
/*
 * enums for column ifOperStatus
 */
typedef enum{
 IFOPERSTATUS_UP    =    1,
 IFOPERSTATUS_DOWN    =    2,
 IFOPERSTATUS_TESTING    =    3,
 IFOPERSTATUS_UNKNOWN    =    4,
 IFOPERSTATUS_DORMANT    =    5,
 IFOPERSTATUS_NOTPRESENT    =    6,
 IFOPERSTATUS_LOWERLAYERDOWN    =    7
}IFoperstatus_t;
/*
 * enums for column ipNetToPhysicalNetAddressType
 */
typedef enum{
 IPNETTOPHYSICALNETADDRESSTYPE_UNKNOWN    =    0,
 IPNETTOPHYSICALNETADDRESSTYPE_IPV4    =    1,
 IPNETTOPHYSICALNETADDRESSTYPE_IPV6    =    2,
 IPNETTOPHYSICALNETADDRESSTYPE_IPV4Z    =    3,
 IPNETTOPHYSICALNETADDRESSTYPE_IPV6Z    =    4,
 IPNETTOPHYSICALNETADDRESSTYPE_DNS    =    16
}IPphyaddress_t;
/*
 * enums for column ipNetToPhysicalType
 */
typedef enum{
 IPNETTOPHYSICALTYPE_OTHER    =    1,
 IPNETTOPHYSICALTYPE_INVALID    =    2,
 IPNETTOPHYSICALTYPE_DYNAMIC    =    3,
 IPNETTOPHYSICALTYPE_STATIC    =    4,
 IPNETTOPHYSICALTYPE_LOCAL    =    5
}IPphytype_t;
/*
 * enums for column ipNetToPhysicalState
 */
typedef enum{
 IPNETTOPHYSICALSTATE_REACHABLE    =    1,
 IPNETTOPHYSICALSTATE_STALE    =    2,
 IPNETTOPHYSICALSTATE_DELAY    =    3,
 IPNETTOPHYSICALSTATE_PROBE    =    4,
 IPNETTOPHYSICALSTATE_INVALID    =    5,
 IPNETTOPHYSICALSTATE_UNKNOWN    =    6,
 IPNETTOPHYSICALSTATE_INCOMPLETE=       7
}IPphystate_t;
/*
 * enums for column ipNetToPhysicalRowStatus
 */
typedef enum{
 IPNETTOPHYSICALROWSTATUS_ACTIVE    =    1,
 IPNETTOPHYSICALROWSTATUS_NOTINSERVICE    =    2,
 IPNETTOPHYSICALROWSTATUS_NOTREADY    =    3,
 IPNETTOPHYSICALROWSTATUS_CREATEANDGO    =    4,
 IPNETTOPHYSICALROWSTATUS_CREATEANDWAIT    =    5,
 IPNETTOPHYSICALROWSTATUS_DESTROY    =    6
}IProwstatus_t;
typedef struct {

   unsigned long         vl_ifIndex;
    unsigned long         vl_ifindex;//for Iptable index
    char                 vl_ifDescr[API_CHRMAX];
    int                  vl_ifDescr_len;
    IFtype_t                 vl_ifType;
    long                  vl_ifMtu;
    unsigned long          vl_ifSpeed;
    char                vl_ifPhysAddress[API_CHRMAX];
    int                 vl_ifPhysAddress_len;
    IFadminstatus_t                vl_ifAdminStatus;
    IFadminstatus_t                vl_old_ifAdminStatus;
    IFoperstatus_t                vl_ifOperStatus;
    unsigned long          vl_ifLastChange;
    unsigned long          vl_ifInOctets;
    unsigned long          vl_ifInUcastPkts;
    unsigned long          vl_ifInNUcastPkts;
    unsigned long          vl_ifInDiscards;
    unsigned long          vl_ifInErrors;
    unsigned long          vl_ifInUnknownProtos;
    unsigned long          vl_ifOutOctets;
    unsigned long          vl_ifOutUcastPkts;
    unsigned long          vl_ifOutNUcastPkts;
    unsigned long          vl_ifOutDiscards;
    unsigned long          vl_ifOutErrors;
    unsigned long          vl_ifOutQLen;
    apioid                 vl_ifSpecific[API_MAX_IOD_LENGTH];
    int                    vl_ifSpecific_len;

}SNMPifTable_t;

typedef struct {

    long            vl_ipNetToPhysicalIfIndex;

    /*
     * Column values
     */
    IPphyaddress_t         vl_ipNetToPhysicalNetAddressType;
    char                   vl_ipNetToPhysicalNetAddress[API_CHRMAX];
    int                    vl_ipNetToPhysicalNetAddress_len;
    char                   vl_ipNetToPhysicalPhysAddress[API_CHRMAX];
    int                    vl_ipNetToPhysicalPhysAddress_len;
    unsigned long          vl_ipNetToPhysicalLastUpdated;
    IPphytype_t            vl_ipNetToPhysicalType;
    IPphystate_t           vl_ipNetToPhysicalState;
    IProwstatus_t          vl_ipNetToPhysicalRowStatus;

}SNMPipNetToPhysicalTable_t;


/**END OF TABLES*/

/**  SCALAR VAR DEC Starts */

/*
 * enums ocStbHostCapabilities
 */
typedef enum {
    S_CAPABILITIES_OTHER    =  1,
    S_CAPABILITIES_OCHD2    =  2,
        S_CAPABILITIES_EMBEDDED   =  3,
     S_CAPABILITIES_DCAS       =  4,
        S_CAPABILITIES_OCHD21     =  5,
    S_CAPABILITIES_BOCR       =  6,
        S_CAPABILITIES_OCHD21TC     =  7,
}HostCapabilities_snmpt;

/**     OCSTBHOST_EASOBJECTS     */
typedef struct {

     unsigned int EMSCodel;  /* unsigned32 (0..99) */
     unsigned int EMCCode;              /* unsigned32 (0..999) */
     unsigned  int EMCSCodel;  /* unsigned32 (0..9) */
}EasInfo_t;


/**     OCSTBHOST_DEVICESOFTWARE     */


typedef struct {
          char SFirmwareVersion[API_CHRMAX];
           char SFirmwareOCAPVersionl[API_CHRMAX];
          char ocStbHostSoftwareFirmwareReleaseDate[API_CHRMAX];//This object returns release date of this entire firmware image (the stack OS).
          int  ocStbHostSoftwareFirmwareReleaseDate_len;
}DeviceSInfo_t;


/**     OCSTBHOST_SOFTWAREIMAGESTATUS     */
/*
 * enums  ocStbHostSoftwareImageStatus
 */
typedef enum {
    S_IMAGEAUTHORIZED    =    1,
    S_IMAGECORRUPTED    =    2,
    S_IMAGECERTFAILURE    =    3,
    S_IMAGEMAXDOWNLOADRETRY =    4,
    S_IMAGEMAXREBOOTRETRY      =     5
}FImageStatus_t;

/*
 * enums  ocStbHostSoftwareCodeDownloadStatus
 */
typedef enum {
    S_DOWNLOADINGSTARTED    =    1,
    S_DOWNLOADINGCOMPLETE    =    2,
    S_DOWNLOADINGFAILED    =    3,
    S_DOWNLOADINGOTHER    =    4
}FCodeDownloadStatus_t;

typedef enum{

    cdlError1  = 1,
    cdlError2   =  2,
    cdlError3  =  3,
    cdlError4  =  4,
    cdlError5  =  5,
    cdlError6  =  6,
    cdlError7  =  7,
    cdlError8   =  8 ,
    cdlError9  =  9,
    cdlError10  = 10,
    cdlError11  = 11,
    cdlError12  = 12,
    cdlError13 = 13,
    cdlError14 = 14,
    cdlError15 =  15,
    cdlError16  = 16,
    cdlError17  = 17,
    cdlError18  = 18,
    cdlError19  = 19,
    cdlError20  = 20,
    cdlError21  = 21,
    cdlError22  = 22,
    cdlError23  = 23,
    cdlError24  = 24,
    cdlError25  = 25

}FCodeDownloadFailedStatus_t;


/**     OCSTBHOST_SOFTWAREDOWNLOADSTATUS     */
typedef struct {

/* FImageStatus_t */ int    vl_FirmwareImageStatus;
/*FCodeDownloadStatus_t*/int    vl_FirmwareCodeDownloadStatus;
         char             vl_FirmwareCodeObjectName[API_CHRMAX];
/*FCodeDownloadFailedStatus_t*/int  vl_FrimwareFailedStatus;
                               int  vl_FirmwareDownloadFailedCount;
} FDownloadStatus_t;

/**     OCSTBHOST_SECURITYSUBSYSTEM     */
/*
 * enums  ocStbHostCAType
 */
typedef enum {
    S_CATYPE_OTHER    =    1,
    S_CATYPE_EMBEDDED    =    2,
    S_CATYPE_CABLECARD    =    3,
    S_CATYPE_DCAS    =    4
}HostCAType_t;


typedef struct {

     unsigned char S_SecurityID[API_CHRMAX];
     char S_CASystemID[API_CHRMAX];
     HostCAType_t  S_HostCAtype;
}HostCATInfo_t;


/**     ocStbHostPowerStatus     */
/*
 * enums ocStbHostPowerStatus
 */
typedef enum {
    S_POWERON    =    1,
    S_STANDBY    =    2
}PStatus_t;
/*
 * enums ocStbHostAcOutletStatus
 */
typedef enum {
    S_UNSWITCHED    =    1,
    S_SWITCHEDON    =    2,
    S_SWITCHEDOFF    =    3,
    S_NOTINSTALLED    =    4
}ActStatus_t;


typedef struct {
            PStatus_t ocStbHostPowerStatus;
            ActStatus_t  ocStbHostAcOutletStatus;
}HostPowerInfo_t;


/**     OCSTBHOST_USERSETTINGS     */

/**     ocStbHostCardIpAddressType     */
/*
 * enums for scalar ocStbHostCardIpAddressType
 */
typedef enum {
    S_CARDIPADDRESSTYPE_UNKNOWN =    0,
    S_CARDIPADDRESSTYPE_IPV4    =    1,
    S_CARDIPADDRESSTYPE_IPV6    =    2,
    S_CARDIPADDRESSTYPE_IPV4Z    =  3,
    S_CARDIPADDRESSTYPE_IPV6Z    =  4,
    S_CARDIPADDRESSTYPE_DNS    =    16
}CardIpAddress_t;

typedef struct {

     unsigned char ocStbHostCardMACAddress[API_CHRMAX];
     CardIpAddress_t  ocStbHostCardIpAddressType;
     char ocStbHostCardIpAddress[API_CHRMAX];
}CardInfo_t;


/** OCATBHOST_CCMMI */

typedef struct {

      int CCApplicationType;
      char CCApplicationName[API_CHRMAX];
      int CCApplicationVersion;
      char CCAppInfoPage[API_CHRMAX];
}SCCMIAppinfo_t;

/** OCATBHOST_INFO */

typedef struct {

   CardIpAddress_t IPAddressType; //same as Card IP enum types
   char IPSubNetMask[API_CHRMAX];
}SOcstbHOSTInfo_t;

/** OCATBHOST_REBOOTINFO */
typedef enum
{
    S_unknown  = 0,
    S_davicDocsis = 1,
    S_user = 2,
    S_system = 3,
    S_trap =  4,
    S_silentWatchdog = 5,
    S_bootloader = 6 ,
    S_powerup =  7,
    S_hostUpgrade = 8,
    S_hardware = 9,
    S_cablecardError = 10

}SRebootType_t;
typedef enum{
    completedSuccessfully = 1,
    completeWithErrors = 2,
    inProgressWithCodeDownload = 3,
    inProgressNoCodeDownload = 4,
    inProgressAwaitingMonitorApp = 5,
    unknown_bootstatus = 6
}ocStbHostBootStatus_t;
typedef struct {

    SRebootType_t rebootinfo;
    Status_t      rebootResetinfo;
    ocStbHostBootStatus_t BootStatus;
}SOcstbHostRebootInfo_t;

/** OCATBHOST_MEMORYINFO */

typedef struct
{
  int AvilabelBlock;
  int videoMemory;
  int AvailableVideoMemory;
  int systemMemory;
  int AvailableSystemMemory;

}SMemoryInfo_t;

/** OCATBHOST_JVMINFO */
typedef struct
{
  int JVMHeapSize;
  int JVMAvailHeap;
  int JVMLiveObjects;
  int JVMDeadObjects;
}SJVMinfo_t;
/** ocStbHostCardBindingStatus */
typedef enum{
    unknown=1,
        invalidCertificate=2,
        otherAuthFailure=3,
        bound=4
}Bindstatis_t;
typedef enum{
        scte55=1,
        dsg=2,
        other=3
}OobMessageMode_t;
typedef enum{
        ready = 1,
        notReady=2
}keyStatus_t;
typedef enum{
    ok = 1,
    failed = 2
}CpCertificateCheck_t;
/** ocStbHostSnmpProxyInfo */
typedef struct{
   char ocStbHostCardMfgId[2+1];
   char ocStbHostCardVersion[256+1];
   unsigned long ocStbHostCardRootOid[128+1];
   int CardRootOidLen;
   char ocStbHostCardSerialNumber[256+1];
   Status_t ocStbHostCardSnmpAccessControl;
   char ocStbHostCardId[17+1];
   Bindstatis_t gProxyCardInfo;
   OobMessageMode_t ocStbHostOobMessageMode;
   char ocStbHostCardOpenedGenericResource[4+1];//4-BYTES
   int ocStbHostCardTimeZoneOffset; //-12 to 12
   char ocStbHostCardDaylightSavingsTimeDelta[1]; //1-BYTE Decimal value of the daylights savings delta
   int ocStbHostCardDaylightSavingsTimeEntry;
   int ocStbHostCardDaylightSavingsTimeExit;
   char ocStbHostCardEaLocationCode[3+1];//3-BYTES
   char ocStbHostCardVctId[2+1]; //2-BYTES
   keyStatus_t ocStbHostCardCpAuthKeyStatus;
   CpCertificateCheck_t ocStbHostCardCpCertificateCheck;
   int ocStbHostCardCpCciChallengeCount; //Display the Card CCI Challenge message count [CCCP] since the last boot or reset
   int ocStbHostCardCpKeyGenerationReqCount;
   char ocStbHostCardCpIdList[4+1];//4-Bytes Display the CP_system_id_bitmask field
}SocStbHostSnmpProxy_CardInfo_t;
/*
 * enums for scalar ocStbHostQpskFDCBer
 */
typedef enum
{
    QPSKFDCBER_BERGREATERTHAN10E2         =1,
    QPSKFDCBER_BERRANGE10E2TOGREATERTHAN10E4 =        2,
    QPSKFDCBER_BERRANGE10E4TOGREATERTHAN10E6 =        3,
    QPSKFDCBER_BERRANGE10E6TOGREATERTHAN10E8 =         4,
    QPSKFDCBER_BERRANGE10E8TOGREATERTHAN10E12 =        5,
    QPSKFDCBER_BEREQUALTOORLESSTHAN10E12     =    6,
    QPSKFDCBER_BERNOTAPPLICABLE       =    7
} QpskFDCBer_t;
/*
 * enums for scalar ocStbHostQpskFDCStatus
 */
typedef enum
{
    QPSKFDCSTATUS_NOTLOCKED     =    1,
    QPSKFDCSTATUS_LOCKED     =    2
} QpskFDCStatus_t;
/*
 * enums for scalar ocStbHostQpskRDCDataRate
 */
typedef enum
{
    QPSKRDCDATARATE_KBPS256        =       1,
    QPSKRDCDATARATE_KBPS1544    =    2,
    QPSKRDCDATARATE_KBPS3088    =    3
} QpskRDCDataRate_t;

typedef struct {


    unsigned long     vl_ocStbHostQpskFDCFreq;
    unsigned long     vl_ocStbHostQpskRDCFreq;
    QpskFDCBer_t      vl_ocStbHostQpskFDCBer;
    QpskFDCStatus_t   vl_ocStbHostQpskFDCStatus;
    unsigned long     vl_ocStbHostQpskFDCBytesRead;
    long              vl_ocStbHostQpskFDCPower;
    unsigned long     vl_ocStbHostQpskFDCLockedTime;
    long              vl_ocStbHostQpskFDCSNR;
    unsigned long     vl_ocStbHostQpskAGC;
    long              vl_ocStbHostQpskRDCPower;
    QpskRDCDataRate_t vl_ocStbHostQpskRDCDataRate;


} ocStbHostQpsk_t;


typedef struct {

    char            vl_VividlogicCertFullInfo[MAX_VIVIDLOGICERT_SIZE];
    size_t          vl_VividlogicCertFullInfo_len;
}Vividlogiccertification_t;

#define VL_SNMP_PREPARE_AND_CHECK_TABLE_GET_REQUEST(struct_type)    \
    vl_table_data =   netsnmp_tdata_extract_table(request);         \
    if(NULL == vl_table_data)                                       \
    {                                                               \
        netsnmp_set_request_error(reqinfo, request,                 \
                                SNMP_NOSUCHINSTANCE);               \
        continue;                                                   \
    }                                                               \
    table_info = netsnmp_extract_table_info(request);               \
    if(NULL == table_info)                                          \
    {                                                               \
        netsnmp_set_request_error(reqinfo, request,                 \
                                SNMP_NOSUCHINSTANCE);               \
        continue;                                                   \
    }                                                               \
    vl_row = netsnmp_tdata_row_get_byidx(vl_table_data, table_info->indexes);\
    if(NULL == vl_row)                                              \
    {                                                               \
        netsnmp_set_request_error(reqinfo, request,                 \
                                SNMP_NOSUCHINSTANCE);               \
        continue;                                                   \
    }                                                               \
    table_entry = (struct_type*)(vl_row->data);                     \
    if(NULL == table_entry)                                         \
    {                                                               \
        netsnmp_set_request_error(reqinfo, request,                 \
                                SNMP_NOSUCHINSTANCE);               \
        continue;                                                   \
    }


#if  0
/**-------------- VIVIDLOGIC Ieee1394certificationTable --------*/
typedef struct
{
    char vl_szDispString[SNMP_STR_SIZE];      //e.g. "xyz Certificate Info" e.g "1394 Certificate Info"
    char vl_szKeyInfoString[SNMP_STR_SIZE];   // usually empty
    char vl_szMfrName[SNMP_STR_SIZE];
    char vl_szModelName[SNMP_STR_SIZE];
    unsigned char vl_acModelId[4];
    unsigned char vl_acVendorId[3];
    unsigned char vl_acDeviceId[5];
    unsigned char vl_acGUIDorMACADDR[8];
    unsigned char vl_vl_bCertAvailable;
    unsigned char vl_bCertValid;
    unsigned char vl_bVerifiedWithChain;
    unsigned char vl_bIsProduction;

}vl_snmpIeeeCertificatateinfo;
#endif//if 0

/**-------------- VIVIDLOGIC McardcertificationTable --------*/

#define CARD_STR_SIZE 512
typedef struct vlMCardCertInfo
{
    char vl_szDispString[CARD_STR_SIZE];  // String" Displays "M-card Host Certificate Info"
    char vl_szDeviceCertSubjectName[CARD_STR_SIZE];// subject string
    char vl_szDeviceCertIssuerName[CARD_STR_SIZE];// Issuer string
    char vl_szManCertSubjectName[CARD_STR_SIZE];// subject string
    char vl_szManCertIssuerName[CARD_STR_SIZE];// Issuer string
    char vl_acHostId[CARD_STR_SIZE]; // Host ID of the device
    char vl_bCertAvailable[4];// 4 if certificates are not copied
    char vl_bCertValid[4]; // 4 if certificates are valid
    char vl_bVerifiedWithChain[4];// 4 if cert chain is verified successfully
    char vl_bIsProduction[4];// 4 for production keys
}vlMCardCertInfo_t;

#endif//__AVINTERFACE_IORM_HEADER







