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

#ifndef _RMF_GLI_H
#define _RMF_GLI_H

#ifdef __cplusplus
extern "C" 
{
#endif
#ifdef GLI
#include "libIBus.h"
#endif
#include <stdint.h>

#define IARM_GLI_RMF_NAME "rmfStreamerClient"

typedef enum
{
	IARM_RMF_POD_DISCONNECT_EVENT = 0,
	IARM_RMF_POD_TIMEZONE_CHANGED_EVENT = 1,
	
	IARM_RMF_GENERIC_MAX_EVENTS,
}IARM_GLI_RMF_GENERIC_EventId_t;

/*-----------------------------------------------------*/

typedef enum
{
	IARM_RMF_POD_SUCCESS,		
	IARM_RMF_POD_NOT_INITIALIZED,
	IARM_RMF_GEN_ERROR,	
}IARM_GLI_RMF_POD_Status_t;

typedef struct
{
	/* for ChannelMapId, ControllerId, PlantId and VODServerId */
	uint64_t Id;
	IARM_GLI_RMF_POD_Status_t status;
}IARM_GLI_RMF_POD_ServiceIds_t;

#define IARM_GLI_RMF_GET_CHANNEL_MAP_ID "getChannelMapId"
#define IARM_GLI_RMF_GET_CONTROLLER_ID "getControllerId"
#define IARM_GLI_RMF_GET_PLANT_ID "getPlantId"
#define IARM_GLI_RMF_GET_VOD_SERVER_ID "getVODServerId"
/*-----------------------------------------------------*/

typedef enum
{
	IARM_RMF_POD_DISCONNECTED,
	IARM_RMF_POD_CONNECTED,	
	IARM_RMF_POD_CONNECT_STATUS_UNKNOWN,		
}IARM_GLI_RMF_POD_DisconnectStatus;

typedef struct
{
	IARM_GLI_RMF_POD_DisconnectStatus status;
}IARM_GLI_RMF_POD_Disconnect_t;

#define IARM_GLI_RMF_GET_DISCONNECT_STATUS "getDisconnectStatus"

/*-----------------------------------------------------*/

typedef enum
{
        GLI_TZ_HST =-11,
        GLI_TZ_AKST = -9,
        GLI_TZ_PST,
	 GLI_TZ_MST,
        GLI_TZ_CST,
        GLI_TZ_EST,
} GLI_TimeZones;

typedef enum
{
	GLI_DLS_OFF,
	GLI_DLS_ON,
} GLI_TimeDLS;

typedef struct
{
	GLI_TimeZones timeZone;
	GLI_TimeDLS dlsFlag;
	IARM_GLI_RMF_POD_Status_t status;
}IARM_GLI_TimeInfo_t;

#define IARM_GLI_RMF_GET_TIMEZONE "getTimeZone"

/*-----------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif

