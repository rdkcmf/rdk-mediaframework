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


#ifndef RESOURCETABLE_H
#define RESOURCETABLE_H
#ifdef __cplusplus
extern "C" {
#endif 

extern int rsmgr_start(LCSM_DEVICE_HANDLE);
extern int rsmgr_stop();

extern int appinfo_start(LCSM_DEVICE_HANDLE);
extern int appinfo_stop();

extern int mmi_start(LCSM_DEVICE_HANDLE);
extern int mmi_stop();

extern int host_start(LCSM_DEVICE_HANDLE);
extern int host_stop();

extern int ca_start(LCSM_DEVICE_HANDLE);
extern int ca_stop();

extern int cardres_start(LCSM_DEVICE_HANDLE);
extern int cardres_stop();

extern int cardMibAcc_start(LCSM_DEVICE_HANDLE);
extern int cardMibAcc_stop();

extern int hostAddPrt_start(LCSM_DEVICE_HANDLE);
extern int hostAddPrt_stop();

extern int headEndComm_start(LCSM_DEVICE_HANDLE);
extern int headEndComm_stop();

extern int dsg_start(LCSM_DEVICE_HANDLE);
extern int dsg_stop();

extern int ipdirect_unicast_start(LCSM_DEVICE_HANDLE);
extern int ipdirect_unicast_stop();

extern int homing_start(LCSM_DEVICE_HANDLE);
extern int homing_stop();

extern int xchan_start(LCSM_DEVICE_HANDLE);
extern int xchan_stop();

extern int systime_start(LCSM_DEVICE_HANDLE);
extern int systime_stop();

extern int cprot_start(LCSM_DEVICE_HANDLE);
extern int cprot_stop();

extern int sas_start(LCSM_DEVICE_HANDLE);
extern int sas_stop();


extern int system_start(LCSM_DEVICE_HANDLE);
extern int system_stop();

extern int feature_start(LCSM_DEVICE_HANDLE);
extern int feature_stop();

extern int diag_start(LCSM_DEVICE_HANDLE);
extern int diag_stop();

//int podStartResources(LCSM_DEVICE_HANDLE h);
//int podStopResources();

#ifdef __cplusplus
}
#endif 

#endif

