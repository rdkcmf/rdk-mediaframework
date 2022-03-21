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


 
//----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif
#define APDU_HAEDEND_COMM_FAILURE -1
#define APDU_HAEDEND_COMM_SUCCESS 0
USHORT lookupheadEndCommSessionValue(void);




int headEndCommReq(unsigned char *pSnmpMess, unsigned long size);
int headEndCommReply(unsigned char  *apdu, unsigned short apduLen);
#ifdef __cplusplus
}
#endif

