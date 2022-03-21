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



#ifndef _RSMGR_MAIN_H_
#define _RSMGR_MAIN_H_

#include <utils.h>

    // Resource ID layout - as described in CEA-679-B section 8.2.2
#define RESOURCE_ID_TYPE_MASK   0xc0000000
#define RESOURCE_CLASS_MASK     0x3fff0000
#define RESOURCE_TYPE_MASK      0x0000ffc0
#define RESOURCE_VERSION_MASK   0x0000003f

    // Resource Manager APDU tags - as shown in SCTE-28 2003 section 6.7.6.4
#define PROFILE_INQ_TAG           0x9f8010
#define PROFILE_REPLY_TAG         0x9f8011
#define PROFILE_CHANGED_TAG       0x9f8012

#define MAX_OPEN_SESSIONS      32 //( 32 )      // for Resource Manager


typedef struct
{
    ULONG   resId;          // resource ID from SCTE-28 2003 Table 8.1-A
    int     maxSessions;    // max open sessions from SCTE-28 2003 Table 8.1-B
    int32   resQ;           // resource's LCSM queue
    char    resName[ 16 ];  // strictly for debugging
    BOOL    supported;      // TRUE if implemented
    BOOL    registered;     // TRUE if resource registered (POD_RESOURCE_PRESENT)
} VL_POD_RESOURCE;

// State machine
typedef enum
{
    RM_UNINITIALIZED = 0,
    RM_INITIALIZED,
    RM_STARTED,
    RM_SHUTDOWN
} rmState_t;

typedef struct
{
    rmState_t       state;  // RM state
    int             loop;   // controls thread execution (when 0, thread will exit)
    pthread_mutex_t lock;   // protects contents of this structure
} rmControl_t;


#ifdef __cplusplus
extern "C" {
#endif

    // "services" offered to App Mgr & other Host resources
unsigned long RM_Get_Class_Type( unsigned long id );
unsigned long RM_Get_ResourceId(unsigned long Resource);
unsigned long RM_Get_Ver( unsigned long id );
unsigned long RM_Is_Private_ReSrc( unsigned long id );
VL_POD_RESOURCE * RM_Find_ReSrc( unsigned long id );
int32 RM_Get_Queue( unsigned long id );

#ifdef __cplusplus
}
#endif

#endif //_RSMGR_MAIN_H_

