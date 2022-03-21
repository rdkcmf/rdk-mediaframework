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

#ifndef XCALIBURCLIENT_H
#define XCALIBURCLIENT_H

/* function declarations */
void init_xcaliburClient(void);
Netsnmp_Node_Handler handle_t2wpVersion;
Netsnmp_Node_Handler handle_chMapId;
Netsnmp_Node_Handler handle_controllerID;
Netsnmp_Node_Handler handle_vodServerId;
Netsnmp_Node_Handler handle_plantId;
Netsnmp_Node_Handler handle_noTuners;
Netsnmp_Node_Handler handle_hostPowerOn;
Netsnmp_Node_Handler handle_dvrCapable;
Netsnmp_Node_Handler handle_dvrEnabled;
Netsnmp_Node_Handler handle_vodEnabled;
Netsnmp_Node_Handler handle_currentChannel;
Netsnmp_Node_Handler handle_ccStatus;
Netsnmp_Node_Handler handle_easStatus;
Netsnmp_Node_Handler handle_avgChannelTuneTime;
Netsnmp_Node_Handler handle_cmacVersion;
Netsnmp_Node_Handler handle_mocaEnabled;
Netsnmp_Node_Handler handle_mocaInUse;
Netsnmp_Node_Handler handle_ethernetEnabled;
Netsnmp_Node_Handler handle_ethernetInUse;
Netsnmp_Node_Handler handle_sysTime;
Netsnmp_Node_Handler handle_sysTimezone;
Netsnmp_Node_Handler handle_sysDSTOffset;
Netsnmp_Node_Handler handle_siCacheSize;
Netsnmp_Node_Handler handle_siLoadComplete;
Netsnmp_Node_Handler handle_noDvrTuners;
Netsnmp_Node_Handler handle_freeDiskSpace;
Netsnmp_Node_Handler handle_cableCardMacAddr;
Netsnmp_Node_Handler handle_hdCapable;
Netsnmp_Node_Handler handle_hdAuthorized;
Netsnmp_Node_Handler handle_mdvrAuthorized;
Netsnmp_Node_Handler handle_dvrReady;
Netsnmp_Node_Handler handle_dvrProvisioned;
Netsnmp_Node_Handler handle_dvrNotBlocked;
Netsnmp_Node_Handler handle_networkType;
Netsnmp_Node_Handler handle_networkVersion;
Netsnmp_Node_Handler handle_specVersion;
Netsnmp_Node_Handler handle_dvrRecording;
Netsnmp_Node_Handler handle_dlnadvrAuthorized;
Netsnmp_Node_Handler handle_dlnalinearAuthorized;
Netsnmp_Node_Handler handle_imageVersion;
Netsnmp_Node_Handler handle_imageTimestamp;
Netsnmp_Node_Handler handle_eidDisconnect;
Netsnmp_Node_Handler handle_hnIp;
Netsnmp_Node_Handler handle_hnPort;
Netsnmp_Node_Handler handle_hnReady;
Netsnmp_Node_Handler handle_dvrProtocolVersion;

#endif /* XCALIBURCLIENT_H */
