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


#include <stdio.h>
#include "snmpIntrfGetTunerData.h"
#include "snmpIntrfGetAvOutData.h"
#include "vlHalSnmpTunerInterface.h"
#include "ipcclient_impl.h"

//#include "vlMpeosCommon.h"
//#include "mpeos_si.h"
//#include "mpeos_media.h"



/**
 * @brief Constructor for snmp tuner interface.
 * Currently it is empty default constructor.
 */
snmpTunerInterface::snmpTunerInterface()
{
    
}


/**
 * @brief Destructor for snmp tuner interface.
 * Currently it is empty default destructor.
 */
snmpTunerInterface::~snmpTunerInterface()
{
    
}


/**
 * @brief This function is used to get the details about in-band tuners like frequency, bandwidth,
 * SNR Value, BER, channel map/dac id etc and call function to fill these data to the tuner table.
 *
 * @param[in] tunerIndex Index number of the available tuner.
 * @param[out] vl_InBandTunerTable Table where details about in-band tuner is stored
 *
 * @return retStatus Return status hal snmp api.
 */
int snmpTunerInterface::Snmp_getTunerdetails(SNMPocStbHostInBandTunerTable_t* vl_InBandTunerTable, unsigned long tunerIndex)
{
    unsigned int retStatus;

    SNMP_INTF_TUNER_INFO_t snmpHALTunersInfo;

    retStatus = HAL_SNMP_Get_Tuner_Info((void*)&snmpHALTunersInfo, tunerIndex);
    IPC_CLIENT_OobGetChannelMapId(( uint16_t *)&snmpHALTunersInfo.channelMapId);
    IPC_CLIENT_OobGetDacId(( uint16_t *)&snmpHALTunersInfo.dacId);

    fillUp_InBandTunerTable(vl_InBandTunerTable, (void*)&snmpHALTunersInfo, tunerIndex);

    return retStatus;

}


/**
 * @brief This function is used to get tuner details for QPSK mode.
 * Currently not implemented.
 *
 * @return None
 */
void snmpTunerInterface::Snmp_getQPSKTunerdetails(/*QpskObj*/)
{
}

/**
 * @brief This function is used to fill the details about in-band tuners like tunerIndex, frequency,
 *  mode, lock status, bandwidth, SNR Value, BER, channel map/dac id etc to the tuner table.
 *
 * @param[in] tunerIndex Index number of the available tuner.
 * @param[in] ptSnmpHALTunersInfo Pointer where tuner details are present.
 * @param[out] vl_InBandTunerTable Table where details about in-band tuner is stored
 *
 * @return None
 */
void snmpTunerInterface::fillUp_InBandTunerTable(SNMPocStbHostInBandTunerTable_t* vl_InBandTunerTable, void* ptSnmpHALTunersInfo, unsigned int tunerIndex)
{
    unsigned int tunerIter;
    SNMP_INTF_TUNER_INFO_t* vl_ptSnmpHALTunersInfo;
    vl_ptSnmpHALTunersInfo = (SNMP_INTF_TUNER_INFO_t*)ptSnmpHALTunersInfo;
    
    if((vl_InBandTunerTable != NULL) && (vl_ptSnmpHALTunersInfo != NULL))
    {
        vl_InBandTunerTable->vl_TunerResourceIndex = vl_ptSnmpHALTunersInfo->tunerIndex;
        
        vl_InBandTunerTable->vl_ocStbHostInBandTunerFrequency = vl_ptSnmpHALTunersInfo->ulFrequency;

        if(vl_ptSnmpHALTunersInfo->enTunerMode == TUNER_QAM256)
            vl_InBandTunerTable->vl_ocStbHostInBandTunerModulationMode = OCSTBHOSTINBANDTUNERMODULATIONMODE_QAM256;
        else if(vl_ptSnmpHALTunersInfo->enTunerMode == TUNER_QAM64)
            vl_InBandTunerTable->vl_ocStbHostInBandTunerModulationMode = OCSTBHOSTINBANDTUNERMODULATIONMODE_QAM64;
        else if(vl_ptSnmpHALTunersInfo->enTunerMode == TUNER_ANALOG)
            vl_InBandTunerTable->vl_ocStbHostInBandTunerModulationMode = OCSTBHOSTINBANDTUNERMODULATIONMODE_ANALOG;
        else
            vl_InBandTunerTable->vl_ocStbHostInBandTunerModulationMode = OCSTBHOSTINBANDTUNERMODULATIONMODE_OTHER;

        
        if(vl_InBandTunerTable->vl_ocStbHostInBandTunerModulationMode  == OCSTBHOSTINBANDTUNERMODULATIONMODE_ANALOG)
        {
            if(vl_ptSnmpHALTunersInfo->enLockStatus == TUNER_DEVICE_LOCKED)
                vl_InBandTunerTable->vl_ocStbHostInBandTunerState = TUNERSTATE_FOUNDSYNC;
            else if(vl_ptSnmpHALTunersInfo->enLockStatus == TUNER_DEVICE_UNLOCKED)
                vl_InBandTunerTable->vl_ocStbHostInBandTunerState = TUNERSTATE_WAITINGSYNC;
            else
                vl_InBandTunerTable->vl_ocStbHostInBandTunerState = TUNERSTATE_UNKNOWN;
        }
        else
        {
            if(vl_ptSnmpHALTunersInfo->enLockStatus == TUNER_DEVICE_LOCKED)
                vl_InBandTunerTable->vl_ocStbHostInBandTunerState = TUNERSTATE_FOUNDQAM;
            else if(vl_ptSnmpHALTunersInfo->enLockStatus == TUNER_DEVICE_UNLOCKED)
                vl_InBandTunerTable->vl_ocStbHostInBandTunerState = TUNERSTATE_WAITINGQAM;
            else
                //DEVICE_TIMEOUT;
                vl_InBandTunerTable->vl_ocStbHostInBandTunerState = TUNERSTATE_UNKNOWN;

            vl_InBandTunerTable->vl_ocStbHostInBandTunerUnerroreds     = vl_ptSnmpHALTunersInfo->stFEC_Status.ulUnerroreds;//Counter
            vl_InBandTunerTable->vl_ocStbHostInBandTunerCorrecteds     = vl_ptSnmpHALTunersInfo->stFEC_Status.ulCorrBytes; //Counter
            vl_InBandTunerTable->vl_ocStbHostInBandTunerUncorrectables = vl_ptSnmpHALTunersInfo->stFEC_Status.ulUnCorrBlocks; //Counter
            vl_InBandTunerTable->vl_ocStbHostInBandTunerPCRErrors      = vl_ptSnmpHALTunersInfo->ulPCRErrors;          //Counter
            vl_InBandTunerTable->vl_ocStbHostInBandTunerPTSErrors      = vl_ptSnmpHALTunersInfo->ulPTSErrors;
        }

        vl_InBandTunerTable->vl_ocStbHostInBandTunerPower          = vl_ptSnmpHALTunersInfo->ulPower;
        vl_InBandTunerTable->vl_ocStbHostInBandTunerBandwidth      = (TunerBandwidth_t)vl_ptSnmpHALTunersInfo->bandwidthCode;
        vl_InBandTunerTable->vl_ocStbHostInBandTunerAGCValue       = vl_ptSnmpHALTunersInfo->ulAGC;       //TenthdB
        vl_InBandTunerTable->vl_ocStbHostInBandTunerSNRValue       = vl_ptSnmpHALTunersInfo->ulSNR;    /* in 1/10 dB */
        vl_InBandTunerTable->vl_ocStbHostInBandTunerInterleaver    = (tunerInleaver_t)vl_ptSnmpHALTunersInfo->ulInterleaver;
        vl_InBandTunerTable->vl_ocStbHostInBandTunerBER            = (tunerBER_t)vl_ptSnmpHALTunersInfo->ulBER;            //Counter
        vl_InBandTunerTable->vl_ocStbHostInBandTunerEqGain         = vl_ptSnmpHALTunersInfo->ulEqGain;
        vl_InBandTunerTable->vl_ocStbHostInBandTunerMainTapCoeff   = vl_ptSnmpHALTunersInfo->ulMainTapCoeff;
        
        
        //Rajasree
        vl_InBandTunerTable->vl_ocStbHostInBandTunerTotalTuneCount= vl_ptSnmpHALTunersInfo->totalTuneCount;
        vl_InBandTunerTable->vl_ocStbHostInBandTunerTuneFailureCount = vl_ptSnmpHALTunersInfo->tuneFailCount;
        vl_InBandTunerTable->vl_ocStbHostInBandTunerTuneFailFreq = vl_ptSnmpHALTunersInfo->lastTuneFailFreq;        
        vl_InBandTunerTable->vl_ocStbHostInBandTunerSecsSinceLock = vl_ptSnmpHALTunersInfo->lockedTimeInSecs;
        vl_InBandTunerTable->vl_ocStbHostInBandTunerCarrierLockLost = vl_ptSnmpHALTunersInfo->carrierLockLostCount;
        vl_InBandTunerTable->vl_ocStbHostInBandTunerChannelMapId = vl_ptSnmpHALTunersInfo->channelMapId;
		vl_InBandTunerTable->vl_ocStbHostInBandTunerDacId = vl_ptSnmpHALTunersInfo->dacId;
    }
    
}

#define VL_CASE_QAM_MODE_STRING(mode)    case MPE_SI_MODULATION_##mode: pszMode = #mode; break

#if 0
const char * vlGetQamModeString(int eQamMode)
{
    static char szMode[32];
    const char * pszMode = "UNKNOWN";
    
    switch(eQamMode)
    {
        VL_CASE_QAM_MODE_STRING(UNKNOWN);
        VL_CASE_QAM_MODE_STRING(QPSK);
        VL_CASE_QAM_MODE_STRING(BPSK);
        VL_CASE_QAM_MODE_STRING(OQPSK);
        VL_CASE_QAM_MODE_STRING(VSB8);
        VL_CASE_QAM_MODE_STRING(VSB16);
        VL_CASE_QAM_MODE_STRING(QAM16);
        VL_CASE_QAM_MODE_STRING(QAM32);
        VL_CASE_QAM_MODE_STRING(QAM64);
        VL_CASE_QAM_MODE_STRING(QAM80);
        VL_CASE_QAM_MODE_STRING(QAM96);
        VL_CASE_QAM_MODE_STRING(QAM112);
        VL_CASE_QAM_MODE_STRING(QAM128);
        VL_CASE_QAM_MODE_STRING(QAM160);
        VL_CASE_QAM_MODE_STRING(QAM192);
        VL_CASE_QAM_MODE_STRING(QAM224);
        VL_CASE_QAM_MODE_STRING(QAM256);
        VL_CASE_QAM_MODE_STRING(QAM320);
        VL_CASE_QAM_MODE_STRING(QAM384);
        VL_CASE_QAM_MODE_STRING(QAM448);
        VL_CASE_QAM_MODE_STRING(QAM512);
        VL_CASE_QAM_MODE_STRING(QAM640);
        VL_CASE_QAM_MODE_STRING(QAM768);
        VL_CASE_QAM_MODE_STRING(QAM896);
        VL_CASE_QAM_MODE_STRING(QAM1024);
        VL_CASE_QAM_MODE_STRING(QAM_NTSC);
    }
    
    snprintf(szMode, sizeof(szMode), "%s (%d)", pszMode, eQamMode);
    
    return szMode;
}
#endif
#if 0
void HAL_SNMP_diag_dump_tuner_stats(unsigned int tunerIndex)
{
    const mpe_LogModule mod = "LOG.RDK.MEDIA";
    const mpe_LogLevel  lvl = RDK_LOG_INFO;
    if(vlMpeosWantLog(mod, lvl))
    {
        SNMP_INTF_TUNER_INFO_t snmpHALTunersInfo;
        usleep(10000); // 10 ms
        unsigned int retStatus = HAL_SNMP_Get_Tuner_Info((void*)&snmpHALTunersInfo, tunerIndex);
        RDK_LOG(lvl, mod,"==== MPEOS TUNE REPORT =============================================================================\n");
        RDK_LOG(lvl, mod,"%16s = %3d, %4s = %12s, %6s = %8s, %9s = %10d\n", "Index", snmpHALTunersInfo.tunerIndex, "Mode", vlGetQamModeString(snmpHALTunersInfo.enTunerMode), "Status", ((snmpHALTunersInfo.enLockStatus == TUNER_DEVICE_LOCKED) ? "Locked" : "Unlocked"), "Frequency", snmpHALTunersInfo.ulFrequency);
        RDK_LOG(lvl, mod,"%16s = %10d, %16s = %10d, %16s = %10d\n", "Correcteds", snmpHALTunersInfo.stFEC_Status.ulCorrBytes, "UnCorrecteds", snmpHALTunersInfo.stFEC_Status.ulUnCorrBlocks, "UnErroreds", snmpHALTunersInfo.stFEC_Status.ulUnerroreds);
        RDK_LOG(lvl, mod,"%16s = %3d, %16s = %3d dB, %13s = %3d, %16s = %2d.%1d dBmV\n", "AGC", snmpHALTunersInfo.ulAGC, "SNR", snmpHALTunersInfo.ulSNR, "BER", snmpHALTunersInfo.ulBER, "Power", ((int)(snmpHALTunersInfo.ulPower))/10, vlAbs((int)(snmpHALTunersInfo.ulPower))%10);
        RDK_LOG(lvl, mod,"%16s = %3d, %16s = %3d, %16s = %3d, %16s = %3d\n", "Interleaver", snmpHALTunersInfo.ulInterleaver, "Eq Gain", snmpHALTunersInfo.ulEqGain, "Main Tap Coeff", snmpHALTunersInfo.ulMainTapCoeff, "PCR Errors", snmpHALTunersInfo.ulPCRErrors);
        RDK_LOG(lvl, mod,"%16s = %3d, %16s = %3d, %16s = %10d\n", "Tune Count", snmpHALTunersInfo.totalTuneCount, "Fail Count", snmpHALTunersInfo.tuneFailCount, "Last Fail Freq", snmpHALTunersInfo.lastTuneFailFreq);
        RDK_LOG(lvl, mod,"====================================================================================================\n");
    }
}
#endif

#define VL_CASE_DECODE_MODE_STRING(val)    case MEDIA_DECODE_SESSION_##val: pszValue = #val; break
#if 0
const char * vlGetDecodeModeString(int eValue)
{
    static char szString[32];
    const char * pszValue = "UNKNOWN";
    
    switch(eValue)
    {
        VL_CASE_DECODE_MODE_STRING(UNDEFINED);
        VL_CASE_DECODE_MODE_STRING(BROADCAST);
        VL_CASE_DECODE_MODE_STRING(DRIPFEED);
        VL_CASE_DECODE_MODE_STRING(DVR);
        VL_CASE_DECODE_MODE_STRING(IP);
    }
    
    snprintf(szString, sizeof(szString), "%s (%d)", pszValue, eValue);
    
    return szString;
}
#endif
#define VL_CASE_STREAM_TYPE_STRING(val)    case MPE_SI_ELEM_##val: pszValue = #val; break
#if 0
const char * vlGetStreamTypeString(int eValue)
{
    static char szString[32];
    const char * pszValue = "UNKNOWN";
    
    switch(eValue)
    {
        VL_CASE_STREAM_TYPE_STRING(MPEG_1_VIDEO);
        VL_CASE_STREAM_TYPE_STRING(MPEG_2_VIDEO);
        VL_CASE_STREAM_TYPE_STRING(MPEG_1_AUDIO);
        VL_CASE_STREAM_TYPE_STRING(MPEG_2_AUDIO);
        VL_CASE_STREAM_TYPE_STRING(MPEG_PRIVATE_SECTION);
        VL_CASE_STREAM_TYPE_STRING(MPEG_PRIVATE_DATA);
        VL_CASE_STREAM_TYPE_STRING(MHEG);
        VL_CASE_STREAM_TYPE_STRING(DSM_CC);
        VL_CASE_STREAM_TYPE_STRING(H_222);
        VL_CASE_STREAM_TYPE_STRING(DSM_CC_MPE);
        VL_CASE_STREAM_TYPE_STRING(DSM_CC_UN);
        VL_CASE_STREAM_TYPE_STRING(DSM_CC_STREAM_DESCRIPTORS);
        VL_CASE_STREAM_TYPE_STRING(DSM_CC_SECTIONS);
        VL_CASE_STREAM_TYPE_STRING(AUXILIARY);
        VL_CASE_STREAM_TYPE_STRING(AAC_ADTS_AUDIO);
        VL_CASE_STREAM_TYPE_STRING(ISO_14496_VISUAL);
        VL_CASE_STREAM_TYPE_STRING(AAC_AUDIO_LATM);
        VL_CASE_STREAM_TYPE_STRING(FLEXMUX_PES);
        VL_CASE_STREAM_TYPE_STRING(FLEXMUX_SECTIONS);
        VL_CASE_STREAM_TYPE_STRING(SYNCHRONIZED_DOWNLOAD);
        VL_CASE_STREAM_TYPE_STRING(METADATA_PES);
        VL_CASE_STREAM_TYPE_STRING(METADATA_SECTIONS);
        VL_CASE_STREAM_TYPE_STRING(METADATA_DATA_CAROUSEL);
        VL_CASE_STREAM_TYPE_STRING(METADATA_OBJECT_CAROUSEL);
        VL_CASE_STREAM_TYPE_STRING(METADATA_SYNCH_DOWNLOAD);
        VL_CASE_STREAM_TYPE_STRING(MPEG_2_IPMP);
        VL_CASE_STREAM_TYPE_STRING(AVC_VIDEO);
        VL_CASE_STREAM_TYPE_STRING(VIDEO_DCII);
        VL_CASE_STREAM_TYPE_STRING(ATSC_AUDIO);
        VL_CASE_STREAM_TYPE_STRING(STD_SUBTITLE);
        VL_CASE_STREAM_TYPE_STRING(ISOCHRONOUS_DATA);
        VL_CASE_STREAM_TYPE_STRING(ASYNCHRONOUS_DATA);
        VL_CASE_STREAM_TYPE_STRING(ENHANCED_ATSC_AUDIO);
    }
    
    snprintf(szString, sizeof(szString), "%s (0x%02X)", pszValue, eValue);
    
    return szString;
}

int HAL_SNMP_dump_decode_params(void * pvDecodeRequest, int SessionType, unsigned int iSession)
{
    const mpe_LogModule mod = "LOG.RDK.MEDIA";
    const mpe_LogLevel  lvl = RDK_LOG_DEBUG;
    if(vlMpeosWantLog(mod, lvl))
    {
        int i = 0;
        mpe_MediaDecodeRequestParams * decodeRequest = (mpe_MediaDecodeRequestParams*)pvDecodeRequest;
        RDK_LOG(lvl, mod,"==== MPEOS DECODE REPORT =====================================================================================\n");
//        RDK_LOG(lvl, mod,"%8s = %d, %11s = %s, %s = %d, %s = %s, %s = 0x%X, %s = %d\n", "iSession", iSession, "Session Type", vlGetDecodeModeString(SessionType), "Tuner Id", decodeRequest->tunerId, "Status", (decodeRequest->blocked ? "Blocked" : "Unblocked"),
                                                                                                             "PCR Pid", decodeRequest->pcrPid, "Num Pids", decodeRequest->numPids);
        for(i = 0; i < decodeRequest->numPids; i++)
        {
//                RDK_LOG(lvl, mod,"%5s = %p, %s  = %s\n", "PID", decodeRequest->pids[i].pid, "Stream Type", vlGetStreamTypeString(decodeRequest->pids[i].pidType));
        }
        RDK_LOG(lvl, mod,"==============================================================================================================\n");
    }
}

int HAL_SNMP_dump_presentation_params()
{
    const mpe_LogModule mod = "LOG.RDK.MEDIA";
    const mpe_LogLevel  lvl = RDK_LOG_DEBUG;
    if(vlMpeosWantLog(mod, lvl))
    {
        snmpAvOutInterface snmpAvOutIntrfObj;
        SNMPocStbHostMpeg2ContentTable_t vl_Mpeg2ContentTable;
        snmpAvOutIntrfObj.snmp_get_Mpeg2Content_info(&vl_Mpeg2ContentTable, 0, 0);
        RDK_LOG(lvl, mod,"==== MPEOS PRESENTATION REPORT ===============================================================================\n");
        RDK_LOG(lvl, mod,"%7s =   %4d, %10s = 0x%04X, %12s =   %4d, %8s = %d\n", "Prog #", vl_Mpeg2ContentTable.vl_ocStbHostMpeg2ContentProgramNumber, "TS ID", vl_Mpeg2ContentTable.vl_ocStbHostMpeg2ContentTransportStreamID, "Num Streams", vl_Mpeg2ContentTable.vl_ocStbHostMpeg2ContentTotalStreams, "CCI", vl_Mpeg2ContentTable.vl_ocStbHostMpeg2ContentCCIValue);
        RDK_LOG(lvl, mod,"%7s = 0x%04X, %10s = 0x%04X, %12s = 0x%04X, %8s = %s\n", "PCR Pid", vl_Mpeg2ContentTable.vl_ocStbHostMpeg2ContentPCRPID, "Video Pid", vl_Mpeg2ContentTable.vl_ocStbHostMpeg2ContentSelectedVideoPID, "Audio Pid", vl_Mpeg2ContentTable.vl_ocStbHostMpeg2ContentSelectedAudioPID, "Status", ((PCRLOCKSTATUS_LOCKED==vl_Mpeg2ContentTable.vl_ocStbHostMpeg2ContentPCRLockStatus) ? "Locked" : "Unlocked"));
        RDK_LOG(lvl, mod,"%7s =   %4d, %10s =   %4d, %12s =   %4d, %8s = %d\n", "Discont", vl_Mpeg2ContentTable.vl_ocStbHostMpeg2ContentDiscontinuities, "Pkt Errors", vl_Mpeg2ContentTable.vl_ocStbHostMpeg2ContentPktErrors, "Pipe Ln Errs", vl_Mpeg2ContentTable.vl_ocStbHostMpeg2ContentPipelineErrors, "Restarts", vl_Mpeg2ContentTable.vl_ocStbHostMpeg2ContentDecoderRestarts);
        RDK_LOG(lvl, mod,"==============================================================================================================\n");
    }
}
#endif
