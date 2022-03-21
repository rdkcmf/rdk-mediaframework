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



#ifndef _DSG_MAIN_H_
#define _DSG_MAIN_H_

#define APDU_DSG_FAILURE 0
#define APDU_DSG_SUCCESS 1

#include "dsg_types.h"

#ifdef __cplusplus
extern "C" {
#endif
    
int  APDU_DSG2POD_InquireDsgMode (unsigned short sesnum, unsigned long tag);
int  APDU_DSG2POD_DsgMessage     (unsigned short sesnum, unsigned long tag, VL_DSG_MESSAGE_TYPE eMessageType, unsigned char bytMsgData);
int  APDU_DSG2POD_SendDcdInfo    (unsigned short sesnum, unsigned long tag, unsigned char * pInfo, unsigned short len);

int  APDU_POD2DSG_SetDsgMode     (unsigned short sesnum, unsigned char * apkt, unsigned short len);
int  APDU_POD2DSG_DsgError       (unsigned short sesnum, unsigned char * apkt, unsigned short len);

int  APDU_POD2DSG_DsgDirectory   (unsigned short sesnum, unsigned char * apkt, unsigned short len);
int  APDU_POD2DSG_ConfigAdvDsg   (unsigned short sesnum, unsigned char * apkt, unsigned short len);

int vlDsgSndApdu2Pod(unsigned short sesnum, unsigned long tag, unsigned short dataLen, unsigned char *data);

int vlIsIpPacketFragmented(VL_DSG_IP_HEADER * pIpPacket);
int vlDsgIsValidTcpChecksum(VL_DSG_IP_HEADER * pIpHeader, VL_DSG_TCP_HEADER * pTcpHeader,
                            unsigned char * pPayload, unsigned long nPayLoadLength);
int vlDsgIsValidUdpChecksum(VL_DSG_IP_HEADER * pIpHeader, VL_DSG_UDP_HEADER * pUdpHeader,
                            unsigned char * pPayload, unsigned long nPayLoadLength);

#ifdef __cplusplus
}
#endif
    
#endif //_DSG_MAIN_H_

