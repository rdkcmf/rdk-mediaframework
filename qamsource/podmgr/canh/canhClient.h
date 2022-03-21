/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2018 RDK Management
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
#ifndef __CANH_CLIENT_H__
#define __CANH_CLIENT_H__

#include "rmf_osal_types.h"
#include "rmf_osal_event.h"

#define CANH_SERVER_DFLT_PORT_NO 50508
#define RMF_IPPVSRC_MAX_PLANTID_LEN 64

typedef enum 
{
	CANH_TARGET_RESOURCE = 0,	
	CANH_TARGET_IPPV = 2,
}canhTargets;

typedef enum
{
		/* Dont change below values as it is used by CANH Java process */
	CANH_PPV_AUTHORIZATION_EVENT = 0x00AD,
	CANH_RESOURCE_AUTHORIZATION_EVENT = 0x00AE,
	CANH_SOURCE_AUTHORIZATION_EVENT = 0x00AF,	
	VOD_SERVER_ID_RECVD_EVENT = 0x00B0,		
	CANH_CLIENT_TERM_EVENT	
}CANHEvents;

rmf_Error canhClientStart(	);
rmf_osal_Bool isCANHReady(  );
rmf_Error canhClientRegisterEvents(rmf_osal_eventqueue_handle_t queueId );
rmf_Error canhClientUnRegisterEvents(rmf_osal_eventqueue_handle_t queueId );
//rmf_Error canhGetPlantId( uint8_t *pPlantId );
rmf_Error canhClientStop();
rmf_Error canhStartEIDMonitoring( uint32_t eID, canhTargets targetType );
rmf_Error canhStopEIDMonitoring( uint32_t eID, canhTargets targetType );
rmf_Error IsCanhResourceAuthorized( uint32_t eID,
											uint8_t * isAuthorized );
rmf_Error IsCanhSourceAuthorized( uint16_t sourceId,
											uint8_t * isAuthorized );
#endif

