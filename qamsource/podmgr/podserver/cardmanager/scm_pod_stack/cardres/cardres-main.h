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
#ifndef _CARDRES_MAIN_H_
#define _CARDRES_MAIN_H_
// CARDRES Req/Confirm Tags
#define CARDRES_TAG_STRM_PRF        0x9FA010
#define CARDRES_TAG_STRM_PRF_CNF    0x9FA011
#define CARDRES_TAG_PRG_PRF        0x9FA012
#define CARDRES_TAG_PRG_PRF_CNF        0x9FA013
#define CARDRES_TAG_ES_PRF        0x9FA014
#define CARDRES_TAG_ES_PRF_CNF        0x9FA015
#define CARDRES_TAG_REQ_PIDS        0x9FA016
#define CARDRES_TAG_PIDS_CNF        0x9FA017

#define CARDRES_HOST_TRAN_STRMS_SUPPORT  4

extern "C" unsigned char vlCardResGetMaxTSSupportedByCard();
extern "C" unsigned char vlCardResGetMaxProgramsSupportedByCard();
extern "C" unsigned char vlCardResGetMaxElmStreamsSupportedByCard();

#endif //_CA_MAIN_H_
