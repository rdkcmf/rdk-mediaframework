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
 * @file This file contains functions which are used to get info like channel number,
 * Service, encryptions, closed caption, front panel led status etc.
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
//#include "ipcclient_impl.h"
#include "stbHostUtils_GetData.h"
//#include "vlHalSnmpTunerInterface.h"
#ifdef USE_XFS_REALTIME
#include <xfs/xfs.h>
#endif
#include <sys/statvfs.h>


#define SICACHE_PATH "/persistent/si/SICache"
#define RECORDING_SEGMENT_PATH "/opt/data/OCAP_MSV/0/0/DEFAULT_RECORDING_VOLUME/dvr/"
#define DUMMY "DUMMY"
#define TRUE "TRUE"
#define FALSE "FALSE"
#define SUCCESS 1
#define FAILURE (-1)

/**
 * @brief This function is used to get the channel number.
 *
 * @param[in] nChars Size of Channel num received.
 * @param[out] ChannelNumber String Pointer to contain channel number.
 *
 * @retval SUCCESS Return success by default.
 */
unsigned int Sget_stbHostUtilsChannelNumber(char* ChannelNumber, int nChars)
{
	
	strncpy(ChannelNumber, "0", sizeof("0"));
	nChars = strlen("0")+1;
	return SUCCESS;
}

/**
 * @brief This function is used to get the type of Current Service.
 * Currently it is hardcoded to string "0".
 *
 * @param[in] nChars Size of received string of current service type.
 * @param[out] CurrentServiceType String Pointer which stores the type of current service .
 *
 * @retval SUCCESS Return success by default.
 */
unsigned int Sget_stbHostUtilsCurrentServiceType(char* CurrentServiceType, int nChars)
{
	//  hardcoded
	strncpy(CurrentServiceType, "0", sizeof("0"));
	nChars = strlen("0")+1;
	return SUCCESS;
}

/**
 * @brief This function is used to get the status for the encryption of current channel.
 * Currently its hardcoded to "YES".
 *
 * @param[in] nChars Size of received string of encryption status.
 * @param[out] CurrentChEncryptionStatus String Pointer which stores the status of
 * encrption of current channel.
 *
 * @retval SUCCESS Return success by default.
 */
unsigned int Sget_stbHostUtilsCurrentChEncryptionStatus(char* CurrentChEncryptionStatus, int nChars)
{
	//  hardcoded
	strncpy(CurrentChEncryptionStatus, "YES", sizeof("YES"));
	nChars = strlen("YES")+1;
	return SUCCESS;
}

/**
 * @brief This function is used to get the user defined (friendly) name of the device. 
 * Currently its hardcoded.
 *
 * @param[in] nChars Size of received friendly name of the device.
 * @param[out] DeviceFriendlyName String Pointer which stores name of the device.
 *
 * @retval SUCCESS Return success by default.
 */
unsigned int Sget_stbHostUtilsDeviceFriendlyName(char* DeviceFriendlyName, int nChars)
{
	//  hardcoded
	strncpy(DeviceFriendlyName, "DELIA-XG1", sizeof("DELIA-XG1"));
	nChars = strlen("DELIA-XG1")+1;
	return SUCCESS;
}

/**
 * @brief This function is used to get the current Zoom option like Wide Fit, FULL etc.
 * Currently its hardcoded to "FULL".
 *
 * @param[in] nChars Size of received string for Zoom Option.
 * @param[out] CurrentZoomOption String pointer which contain the current zoom option.
 *
 * @retval SUCCESS Return success by default.
 */
unsigned int Sget_stbHostUtilsCurrentZoomOption(char* CurrentZoomOption, int nChars)
{
	//  hardcoded 
	strncpy(CurrentZoomOption, "FULL", sizeof("FULL"));
	nChars = strlen("FULL")+1;
	return SUCCESS;
}

/**
 * @brief This function is used to check Lock Status of the current channel.
 * Currently it has been set to "TRUE".
 *
 * @param[in] nChars Size of received string for Lock Status.
 * @param[out] ChannelLockStatus String pointer which contain the current lock
 * status of channel.
 *
 * @retval SUCCESS Return success by default.
 */
unsigned int Sget_stbHostUtilsChannelLockStatus(char* ChannelLockStatus, int nChars)
{
	//  Dummy Code 
	strncpy(ChannelLockStatus, "TRUE", sizeof("TRUE"));
	nChars = strlen("TRUE")+1;
	return SUCCESS;
}

/**
 * @brief This function is used to get the status of parental control, whether
 * it is applied or not. Currently it is set to "FALSE".
 *
 * @param[in] nChars Size of received string for Parental Control Status.
 * @param[out] ParentalControlStatus String pointer to store Parental Control Status.
 *
 * @retval SUCCESS Return success by default.
 */
unsigned int Sget_stbHostUtilsParentalControlStatus(char* ParentalControlStatus, int nChars)
{
	//  Dummy Code
	strncpy(ParentalControlStatus, "FALSE", sizeof("FALSE"));
	nChars = strlen("FALSE")+1;
	return SUCCESS;
}

/**
 * @brief This function is used to check the status of Closed Caption so that subtitles
 * can be made on or off. Currently it is set to "TRUE".
 *
 * @param[in] nChars Size of received string for state of Closed Caption.
 * @param[out] ClosedCaptionState String pointer to store Closed Caption State.
 *
 * @retval SUCCESS Return success by default.
 */
unsigned int  Sget_stbHostUtilsClosedCaptionState(char* ClosedCaptionState, int nChars)
{
	// Dummy Code
	strncpy(ClosedCaptionState, "TRUE", sizeof("TRUE")); 
	nChars = strlen("TRUE")+1;
	return SUCCESS;
}

/**
 * @brief This function is used to get which color has been set for closed caption.
 * Currently it has been set to "RED".
 *
 * @param[in] nChars Size of received string for color of closed caption.
 * @param[out] ClosedCaptionColor String pointer which contain color name of caption.
 *
 * @retval SUCCESS Return success by default.
 */
unsigned int Sget_stbHostUtilsClosedCaptionColor(char* ClosedCaptionColor, int nChars)
{
	//  Dummy Code
	strncpy(ClosedCaptionColor, "RED", sizeof("RED"));
	nChars = strlen("RED")+1;
	return SUCCESS;
}

/**
 * @brief This function is used to check which font has been set for closed caption.
 * Currently it has been set to "SAN".
 *
 * @param[in] nChars Size of received string for font of closed caption.
 * @param[out] ClosedCaptionFont String pointer which contain font name of caption.
 *
 * @retval SUCCESS Return success by default.
 */
unsigned int Sget_stbHostUtilsClosedCaptionFont(char* ClosedCaptionFont, int nChars)
{
	//  Dummy Code
	strncpy(ClosedCaptionFont, "SAN", sizeof("SAN"));
	nChars = strlen("SAN")+1;
	return SUCCESS;
}

/**
 * @brief This function is used to find how much opacity has been set for the closed
 * caption subtitle. Currently a dummy value of "10" has been assigned to it.
 *
 * @param[in] nChars Size of received string for value of opacity of closed caption.
 * @param[out] ClosedCaptionOpacity String pointer which contain value of opacity of caption.
 *
 * @retval SUCCESS Return success by default.
 */
unsigned int  Sget_stbHostUtilsClosedCaptionOpacity(char* ClosedCaptionOpacity, int nChars)
{
	//  Dummy Code 
	strncpy(ClosedCaptionOpacity, "10", sizeof("10"));
	nChars = strlen("10")+1;
	return SUCCESS;
}

/**
 * @brief This function is used to check the status of EAS.
 * Currently it is set to "TRUE".
 *
 * @param[in] nChars Size of the string which contain the state of EAS.
 * @param[out] EASState String pointer which contain EAS state.
 *
 * @retval SUCCESS Return success by default.
 */
unsigned int Sget_stbHostUtilsEASState(char* EASState, int nChars)
{
	//  Dummy Code
	strncpy(EASState, "TRUE", sizeof("TRUE"));
	nChars = strlen("TRUE")+1;
	return SUCCESS;
}

/**
 * @brief This function is used to get the status of record LED present in front
 * panel to check current status of recording. Currently its value is set to "TRUE".
 *
 * @param[in] nChars Size of the string which contain the status of record led.
 * @param[out] FpRecordLedStatus String pointer to store the status of Front panel record led.
 *
 * @retval SUCCESS Return success by default.
 */
unsigned int Sget_stbHostUtilsFpRecordLedStatus(char* FpRecordLedStatus, int nChars)
{
	//  Dummy Code 
	strncpy(FpRecordLedStatus, "TRUE", sizeof("TRUE"));
	nChars = strlen("TRUE")+1;
	return SUCCESS;
}

/**
 * @brief This function is used to get the status of front panel power LED to know
 * set-top box is in which state. Currently its value is set to "FALSE".
 *
 * @param[in] nChars Size of the string which contain the status of power led.
 * @param[out] FpPowerLedStatus String pointer to store the status of Front panel Power led.
 *
 * @retval SUCCESS Return success by default.
 */
unsigned int Sget_stbHostUtilsFpPowerLedStatus(char* FpPowerLedStatus, int nChars)
{
	//  Dummy Code 
	strncpy(FpPowerLedStatus, "FALSE", sizeof("FALSE"));
	nChars = strlen("FALSE")+1;
	return SUCCESS;
}

