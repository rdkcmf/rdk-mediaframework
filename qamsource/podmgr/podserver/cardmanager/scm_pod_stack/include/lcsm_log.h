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



#ifndef _LCSM_LOG_H_
#define _LCSM_LOG_H_


// diagnostic levels (per Randall Gay)
#define LOG_FATAL       ( 1 )
#define LOG_NON_FATAL   ( 2 )
#define LOG_WARNING     ( 3 )
#define LOG_DEBUG       ( 4 )
#define LOG_INFO        ( 5 )

typedef struct  _LogInfo{
  int diagLevel;        // use the #define's above
  const char *diagMsg;
} LogInfo;

#ifdef __KERNEL__

//Kernel-space logging interface
int lcsm_send_log(LogInfo *logEvent);

#else

#ifdef __cplusplus
extern "C" {
#endif
//User-space logging interface
int lcsm_send_log(LCSM_DEVICE_HANDLE handle, LogInfo *logEvent);

#ifdef __cplusplus
}
#endif

#endif /*__KERNEL__*/

#endif /*_LCSM_LOG_H_*/
