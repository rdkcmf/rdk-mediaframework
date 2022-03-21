/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2014 RDK Management
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

#include <dsg_types.h>
#include "halpodclass.h"
#include <cablecarddriver.h>
#include <rdk/podmgr/snmp_types.h>
#include <vl_cdl_types.h>

#define VL_CDL_RESULT_SUCCESS 0
#define HAL_SUCCESS 0

#define UNUSED_VAR(v) ((void)v)
typedef unsigned char  UINT8;
typedef unsigned short UINT16;
typedef unsigned long  UINT32;
typedef signed char    INT8;
typedef signed short   INT16;
typedef signed long    INT32;
typedef unsigned long DEVICE_HANDLE_t;

typedef void          ( *VL_CDL_VOID_CBFUNC_t        )(void);

typedef enum tag_VL_OOB_COMMAND
{
    VL_OOB_COMMAND_GET_IP_INFO,
}VL_OOB_COMMAND;


extern "C" int
CHALCdl_init()
{
	return 0;
}

extern "C" int 
CHALCdl_notify_cdl_mgr_event(VL_CDL_MANAGER_EVENT_TYPE eEvent, unsigned long nEventData)
{
	UNUSED_VAR(eEvent);
	UNUSED_VAR(nEventData);
	return VL_CDL_RESULT_SUCCESS;
}

extern "C" int
CHALCdl_register_callback(VL_CDL_NOTIFY_TYPE_t type, VL_CDL_VOID_CBFUNC_t func_ptr, void * pulData)
{
    UNUSED_VAR(type);
	UNUSED_VAR(func_ptr);
	UNUSED_VAR(pulData);
	return HAL_SUCCESS;
}

extern "C" int 
CHALDsg_init()
{
    return HAL_SUCCESS;
}

extern "C" int 
CHALDsg_register_callback(int device_instance, VL_DSG_NOTIFY_TYPE_t type, void *func_ptr)
{
    UNUSED_VAR(type);
	UNUSED_VAR(func_ptr);
    return HAL_SUCCESS; 
}

extern "C" 
int CHALDsg_Send_ControlMessage(int device_instance, unsigned char * pData, unsigned long length)
{
    UNUSED_VAR(device_instance);
	UNUSED_VAR(pData);
	UNUSED_VAR(length);
    return HAL_SUCCESS;
}

extern "C" 
int CHALDsg_Set_Config(int device_instance, unsigned long ulTag, void* pvConfig)
{
    UNUSED_VAR(device_instance);
	UNUSED_VAR(ulTag);
	UNUSED_VAR(pvConfig);
    return HAL_SUCCESS;
}
extern "C" int CHALPod_init()
{
	return 0;
}

extern "C" int    CHALPod_oob_control(int device_instance, VL_OOB_COMMAND eCommand, void * pData)
{
	UNUSED_VAR(device_instance);
	UNUSED_VAR(eCommand);
	UNUSED_VAR(pData);
    return 0;
}

extern "C" 
VL_SNMP_API_RESULT CHalSys_snmp_request(VL_SNMP_REQUEST eRequest, void * _pvStruct)
{
	UNUSED_VAR(eRequest);
	UNUSED_VAR(_pvStruct);
	return VL_SNMP_API_RESULT_SUCCESS;
}

extern "C" 
void EventCableCard_RawAPDU_free_fn(void *ptr)
{
return;
}

extern "C"
INT32 HAL_POD_RESET_PCMCIA(DEVICE_HANDLE_t hPODHandle, UINT32  *pData)
{
	UNUSED_VAR(hPODHandle);
	UNUSED_VAR(pData);
	return HAL_SUCCESS;
}


INT32 HAL_Save_Icn_Pid (UINT16 *IcnPid)
{
	UNUSED_VAR(IcnPid);
	return HAL_SUCCESS;
}

extern "C"
void HAL_SYS_Reboot(const char * pszRequestor, const char * pszReason)
{
	UNUSED_VAR(pszRequestor);
	UNUSED_VAR(pszReason);
	return;
}

extern "C"
int podmgrConfigCipher(unsigned char ltsid,unsigned short PrgNum,UINT32 *decodePid,UINT32 numpids)
{
	UNUSED_VAR(ltsid);
	UNUSED_VAR(PrgNum);
	UNUSED_VAR(decodePid);
	UNUSED_VAR(numpids);
	return 0;
}

CHALPod::CHALPod()
 : cableCardDriver()
{
}

CHALPod::~CHALPod()
{
}

void CHALPod::initialize()
{
	return;
}

int CHALPod::get_device_count()
{
    return 0;
}

int
CHALPod::open_device(int device_instance)
{
    return 0;
}

int
CHALPod::close_device(int device_instance)
{
	return 0;
}

int
CHALPod::register_callback(eCallbackType type, void *func_ptr, int device_instance)
{
	return 0;
}

int
CHALPod::send_data(void *ptr, CHANNEL_TYPE_T  type, int device_instance)
{
	return 0;
}


int  CHALPod::configPodCipher(unsigned char ltsid,unsigned short PrgNum,unsigned long *decodePid,unsigned long numpids,unsigned long DesKeyAHi,unsigned long DesKeyALo,unsigned long DesKeyBHi,unsigned long DesKeyBLo,void *pStrPtr)
{
  return 0;
}

int  CHALPod::stopconfigPodCipher(unsigned char ltsid,unsigned short PrgNum,unsigned long *decodePid,unsigned long numpids)
{
    return 0;
}

DEVICE_HANDLE_t
CHALPod::GetHandleFromInstance(int device_instance)
{
    return 0;
}

void CHALPod::ConnectSourceToPOD(unsigned long tuner_in_handle)
{
}

void CHALPod::DisconnectSourceFromPOD(unsigned long tuner_in_handle)
{
}

int CHALPod::oob_control(int device_instance, unsigned long eCommand, void * pData)
{
    return 0;
}

int CHALPod::if_control(int device_instance, unsigned long eCommand, void * pData)
{
    return 0;
}

extern "C" void SetSignalMask()
{
    return;
}

