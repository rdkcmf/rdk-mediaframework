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


//this file may need to more up to the include level dir.
#ifdef __cplusplus
extern "C" {
#endif
//int /* APDU_HC_SUCCESS */  RespOOBTxTuneCnf  (USHORT sesnum);
int /* APDU_HC_SUCCESS */  RespOOBTxTuneCnf  (uint32 status);
int /* APDU_HC_SUCCESS */  RespOOBRxTuneCnf  (uint32 rtndStatus);
int /* APDU_HC_SUCCESS */  RespInbandTuneCnf (uint32 rtndStatus, uint32 SeqNum);

#define APDU_HC_FAILURE 0
#define APDU_HC_SUCCESS 1

#ifdef __cplusplus
}
#endif



