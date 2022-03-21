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

#if !defined(_RMF_ERROR_H_)
#define _RMF_ERROR_H_

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef uint32_t
typedef unsigned int uint32_t;
#endif

#define RMF_SUCCESS          0

#define RMF_ERRBASE_OSAL              (0x0100)
#define RMF_ERRBASE_OSAL_STORAGE      (0x0200)
#define RMF_ERRBASE_OSAL_FS           (0x0300)
#define RMF_ERRBASE_MEDIASTREAMER     (0x0400)
#define RMF_ERRBASE_GSTQAMSRC         (0x0500)
#define RMF_ERRBASE_PLATFORM          (0x0600)
#define RMF_ERRBASE_IPPVSRC           (0x0700)

#define RMF_IPPVSRC_EVENT_BASE        (0x100)
#define RMF_QAMSRC_EVENT_BASE         (0x200)
#define RMF_HNSINK_EVENT_BASE         (0x300)
#define RMF_VODSRC_EVENT_BASE         (0x500)
#define RMF_DVRSRC_EVENT_BASE         (0x600)
#define RMF_DVRSINK_EVENT_BASE        (0x700)
#define RMF_HNSRC_EVENT_BASE          (0x800)

typedef uint32_t rmf_Error;


/* This may not be ideal place to add this kind of inter process communitcation
 * details, but we are forced to do that since there is no header file
 * available at present. This we will move to appropriate file later when we come
 * up with some header file for system wide interprocess communication details.
 */
#define IARM_BUS_STREAMER_STORAGE_BUS_NAME "rmfStreamer"

typedef enum _IARM_Bus_Streamer_EventId_t {
	IARM_BUS_STREAMER_TSB_ERROR = 1,
	IARM_BUS_STREAMER_ERROR_MAX,
} IARM_Bus_Streamer_EventId_t;

typedef enum _IARM_Bus_TSB_Status_t {
	IARM_BUS_STREAMER_TSB_READ_ERROR	= 16,
	IARM_BUS_STREAMER_TSB_WRITE_ERROR	= 32,
	IARM_BUS_STREAMER_TSB_UNKNOWN_ERROR	= 64,
} IARM_Bus_TSB_Status_t;

typedef struct _IARM_BUS_Streamer_EventData_t {
	IARM_Bus_TSB_Status_t tsbStatus;
} IARM_Bus_Streamer_EventData_t;

#ifdef __cplusplus
}
#endif

#endif /* _RMF_ERROR_H_ */

