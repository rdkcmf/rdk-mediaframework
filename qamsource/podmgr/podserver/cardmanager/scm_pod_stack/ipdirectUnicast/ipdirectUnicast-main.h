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



#ifndef _IPDIRECT_UNICAST_MAIN_H_
#define _IPDIRECT_UNICAST_MAIN_H_

#define APDU_IPDIRECT_UNICAST_FAILURE 0
#define APDU_IPDIRECT_UNICAST_SUCCESS 1

#include "ipdirect_unicast_types.h"
#include "rmf_osal_event.h"

#ifdef __cplusplus
extern "C" {
#endif

int vlIpDirectUnicastSndApdu2Pod(unsigned short sesnum, unsigned long tag, unsigned short dataLen, unsigned char *data);
int vlIpDirectUnicastHandleEcmMessage(VL_DSG_MESSAGE_TYPE eMessage);
int vlIpDirectUnicastNotifyHostIpAcquired();
int vlIpDirectUnicastNotifySocketFailure( IPDIRECT_UNICAST_LOST_FLOW_REASON reason );
void vlIpDirectUnicastDumpIpDirectUnicastStats();
void  vlSetIpDirectUnicastFlowId(unsigned long nFlowId);
void  SetSocketFlowLocalPortNumber( int PortNumber);

/* Used to start and stop the IPDU Download Progress Timer.                */
/* control=true will start the timer, control=false will cancel the timer. */
void vlIpduFileTransferProgressTimer(bool control, long int event);
/* Utility function to extract Url from APDU parameters. The basic url is the primary curl interface control. */
/* The url is the return value. */
char* AssembleHttpGetUrl(IPDIRECT_UNICAST_HTTP_GET_PARAMS* pParams);
int IsSystemTimeAvailable(void);
int IsNetworkAvailable(void);
/* This function encapsulates the steps to confirm an Http Get response and send downloaded file to the Cable Card. */
void ServiceHttpGetResponse(char* cUrl, IPDIRECT_UNICAST_HTTP_GET_PARAMS* pRequestParams);
/* This function encapsulates the steps to confirm an Http Post response. */
void ServiceHttpPostResponse(char* cUrl, IPDIRECT_UNICAST_HTTP_GET_PARAMS* pRequestParams);

rmf_osal_eventqueue_handle_t GetIpDirectUnicastMsgQueue(void);
#ifdef __cplusplus
}
#endif

#endif //_IPDIRECT_UNICAST_MAIN_H_

