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
#ifndef STBHOSTUTILS_H
#define STBHOSTUTILS_H

/* function declarations */
void init_stbHostUtils(void);
Netsnmp_Node_Handler handle_currentChannelNumber;
Netsnmp_Node_Handler handle_currentServiceType;
Netsnmp_Node_Handler handle_currentChEncryptionStatus;
Netsnmp_Node_Handler handle_deviceFriendlyName;
Netsnmp_Node_Handler handle_currentZoomOption;
Netsnmp_Node_Handler handle_channelLockStatus;
Netsnmp_Node_Handler handle_parentalControlStatus;
Netsnmp_Node_Handler handle_closedCaptionState;
Netsnmp_Node_Handler handle_closedCaptionColor;
Netsnmp_Node_Handler handle_closedCaptionFont;
Netsnmp_Node_Handler handle_closedCaptionOpacity;
Netsnmp_Node_Handler handle_easState;
Netsnmp_Node_Handler handle_fpRecordLedStatus;
Netsnmp_Node_Handler handle_fpPowerLedStatus;

#endif /* STBHOSTUTILS_H */
