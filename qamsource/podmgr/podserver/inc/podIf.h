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



#ifndef __VLMPEOSPODIF_H__
#define __VLMPEOSPODIF_H__

#include <rmf_osal_types.h>
#include <podimpl.h>
#include <podmgr.h>

#ifdef __cplusplus
extern "C" {
#endif

/* name is same as CardManagerCCIData_t. Need to make this common */
typedef struct
{
	unsigned short prgNum;// Program Number
	unsigned char LTSID; // tunner ltsid
	unsigned char CCI; //cci information of the program
	unsigned char EMI;// EMI part of cci
	unsigned char APS;// APS part of cci
	unsigned char CIT;// CIT part of cci
	unsigned char RCT;//
	unsigned char CCIAuthStatus;// if this flag is set to 1 then CCI Auth success else if set to 0 then failure
}VlCardManagerCCIData_t;

rmf_Error vlMpeosPodInit(rmf_PODDatabase *podDB);

rmf_Error vlMpeosPodRegister(rmf_osal_eventqueue_handle_t queueId, void* act);
rmf_Error vlMpeosPodUnregister(void);

rmf_Error vlMpeosPodGetAppInfo(rmf_PODDatabase *podDB);
rmf_Error vlMpeosPodGetFeatures(rmf_PODDatabase *podDB);

rmf_Error vlMpeosPodGetFeatureParam(rmf_PODDatabase *podDBPtr, uint32_t featureId);

rmf_Error vlMpeosPodSetFeatureParam(uint32_t featureId, uint8_t *param, uint32_t size);

rmf_Error vlMpeosPostCCIChangeEvent(VlCardManagerCCIData_t *pInCci);

rmf_Error vlMpeosPodGetParam(rmf_PODMGRParamId paramId, uint16_t* paramValue);

rmf_Error vlMpeosPodReceiveAPDU(uint32_t *sessionId, uint8_t **apdu, int32_t *len);

rmf_Error vlMpeosPodSendCaPmtAPDU(uint32_t sessionId, uint32_t apduTag, uint32_t length, uint8_t *apdu);

#ifdef __cplusplus
}
#endif

#endif // __VLMPEOSPODIF_H__

