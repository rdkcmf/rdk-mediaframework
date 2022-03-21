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
 * @defgroup HOST_UTILS Host Utils Data
 * @ingroup SNMP_MGR
 * @ingroup HOST_UTILS
 * @{
 */

#ifndef STBHOSTUTILS_GETDATA_H
#define STBHOSTUTILS_GETDATA_H
unsigned int Sget_stbHostUtilsChannelNumber(char* ChannelNumber, int nChars);
unsigned int Sget_stbHostUtilsCurrentServiceType(char* CurrentServiceType, int nChars);
unsigned int Sget_stbHostUtilsCurrentChEncryptionStatus(char* CurrentChEncryptionStatus, int nChars);
unsigned int Sget_stbHostUtilsDeviceFriendlyName(char* DeviceFriendlyName, int nChars);
unsigned int Sget_stbHostUtilsCurrentZoomOption(char* CurrentZoomOption, int nChars);

unsigned int Sget_stbHostUtilsChannelLockStatus(char* ChannelLockStatus, int nChars);
unsigned int Sget_stbHostUtilsParentalControlStatus(char* ParentalControlStatus, int nChars);
unsigned int Sget_stbHostUtilsClosedCaptionState(char* ClosedCaptionState, int nChars);
unsigned int Sget_stbHostUtilsClosedCaptionColor(char* ClosedCaptionColor, int nChars);
unsigned int Sget_stbHostUtilsClosedCaptionFont(char* ClosedCaptionFont, int nChars);
unsigned int Sget_stbHostUtilsClosedCaptionOpacity(char* ClosedCaptionOpacity, int nChars);
unsigned int Sget_stbHostUtilsEASState(char* EASState, int nChars);
unsigned int Sget_stbHostUtilsFpRecordLedStatus(char* FpRecordLedStatus, int nChars);
unsigned int Sget_stbHostUtilsFpPowerLedStatus(char* FpPowerLedStatus, int nChars);

#endif /*STBHOSTUTILS_GETDATA_H */
/**
 * @}
 */
