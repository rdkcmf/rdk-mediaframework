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

#ifndef PROXYMIBGETDATA_H
#define PROXYMIBGETDATA_H

unsigned int Sget_xcaliburClientDvrProtocolVersion(char* DvrProtocolVersion, int nChars);
unsigned int Sget_xcaliburClientHNReady(char* HNReady, int nChars);
unsigned int Sget_xcaliburClientHNPort(char* HNPort, int nChars);
unsigned int Sget_xcaliburClientHNIp(char* HNIp, int nChars);
unsigned int Sget_xcaliburClientEIDDisconnect(char* EIDDisconnect, int nChars);
/*
unsigned int Sget_xcaliburClientImageTimeStamp(char* ImageTimeStamp, int nChars);
unsigned int Sget_xcaliburClientImageVersion(char* ImageVersion, int nChars);
*/
unsigned int Sget_xcaliburClientDLNALinearAuth(char* DLNALinearAuth, int nChars);
unsigned int Sget_xcaliburClientDLNADVRAuth(char* DLNADVRAuth, int nChars);
unsigned int Sget_xcaliburClientDVRRec(char* DVRRec, int nChars);
unsigned int Sget_xcaliburClientSpecVersion(char* SpecVersion, int nChars);
unsigned int Sget_xcaliburClientNetworkVersion(char* NetworkVersion, int nChars);
unsigned int Sget_xcaliburClientNetworkType(char* NetworkType, int nChars);
unsigned int Sget_xcaliburClientDVRNotBlocked(char* DVRNotBlocked, int nChars);
unsigned int Sget_xcaliburClientDVRProvisioned(char* DVRProvisioned, int nChars);
unsigned int Sget_xcaliburClientDVRReady(char* DVRReady, int nChars);
unsigned int Sget_xcaliburClientmDVRAuth(char* DVRAuth, int nChars);
unsigned int Sget_xcaliburClientHDAuth(char* HDAuth, int nChars);
unsigned int Sget_xcaliburClientHDCapable(char* HDCapable, int nChars);
//unsigned int Sget_xcaliburClientCableCardMacAddr(char* CableCardMacAddr, int nChars);
unsigned int Sget_xcaliburClientFreeDiskSpace(char* FreeDiskSpace, int nChars);
unsigned int Sget_xcaliburClientNumberOfDVRTuners(char* NumberOfDVRTuners, int nChars);
unsigned int Sget_xcaliburClientSILoadCompleted(char* SILoadCompleted, int nChars);
unsigned int Sget_xcaliburClientSICacheSize(char* SICacheSize, int nChars);
unsigned int Sget_xcaliburClientSysDSTOffset(char* SysDSTOffset, int nChars);
//unsigned int Sget_xcaliburClientSysTimeZone(char* SysTimeZone, int nChars);
unsigned int Sget_xcaliburClientSysTime(char** ppSysTime, int nChars);
unsigned int Sget_xcaliburClientEthernetInUse(char* EthernetInUse, int nChars);
unsigned int Sget_xcaliburClientEthernetEnabled(char* EthernetEnabled, int nChars);
unsigned int Sget_xcaliburClientMoCAInUse(char* MoCAInUse, int nChars);
unsigned int Sget_xcaliburClientMoCAEnabled(char* MoCAEnabled, int nChars);
unsigned int Sget_xcaliburClientCMACVersion(char* CMACVersion, int nChars);
unsigned int Sget_xcaliburClientAvgChannelTuneTime(char* AvgChannelTuneTime, int nChars);
unsigned int Sget_xcaliburClientEASStatus(char* EASStatus, int nChars);
unsigned int Sget_xcaliburClientCCStatus(char* CCStatus, int nChars);
unsigned int Sget_xcaliburClientCurrentChannel(char* CurrentChannel, int nChars);
unsigned int Sget_xcaliburClientVodEabled(char* VodEabled, int nChars);
unsigned int Sget_xcaliburClientDVREnabled(char* DVREnabled, int nChars);
unsigned int Sget_xcaliburClientDVRCapable(char* DVRCapable, int nChars);
//unsigned int Sget_xcaliburClientHostPower(char* HostPower, int nChars);
unsigned int Sget_xcaliburClientNumberOfTuners(char* NumberOfTuners, int nChars);
unsigned int Sget_xcaliburClientPlantId(char* PlantId, int nChars);
unsigned int Sget_xcaliburClientVodServerId(char* VodServerId, int nChars);
unsigned int Sget_xcaliburClientControllerID(char* ControllerID, int nChars);
unsigned int Sget_xcaliburClientChannelMapId(char* ChannelMapId, int nChars);
unsigned int Sget_xcaliburClientT2pVersion(char* T2pVersion, int nChars);

#endif /*PROXYMIBGETDATA_H */

