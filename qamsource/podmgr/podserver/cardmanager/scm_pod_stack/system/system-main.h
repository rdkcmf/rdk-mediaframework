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


#ifndef _SYSTEM_MAIN_H_
#define _SYSTEM_MAIN_H_

#define SYSCONT_TAG_HOST_INFO_REQ         0x9F9C00
#define SYSCONT_TAG_HOST_INFO_RESP        0x9F9C01
#define SYSCONT_TAG_CVT_T1V1              0x9F9C02
#define SYSCONT_TAG_CVT_T2V1              0x9F9C05
#define SYSCONT_TAG_CVT_Rply_T1V1_T2V1    0x9F9C03
#define SYSCONT_TAG_DL_CTL          0x9F9C04
int  SysCtlReadVendorData();
void vlSystemFreeMem(void *pData);
#endif//_SYSTEM_MAIN_H_
