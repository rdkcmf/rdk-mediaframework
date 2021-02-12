/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2021 RDK Management
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
**/

#include "open_cdm.h"
#include "rdk_debug.h"

#ifdef __cplusplus
extern "C" {
#endif

OpenCDMSessionCallbacks* pCallbacks;
void *pUserData;

static uint32_t gOcdmSystem = 0x1000;
static uint32_t gOcdmSession = 0x2001;
struct OpenCDMSystem* opencdm_create_system(const char keySystem[])
{
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] opencdm_create_system - Enter\n", __FUNCTION__, __LINE__);
    gOcdmSystem++;
    return ((struct OpenCDMSystem*)gOcdmSystem);
}

OpenCDMError opencdm_construct_session(struct OpenCDMSystem* system, const LicenseType licenseType,
    const char initDataType[], const uint8_t initData[], const uint16_t initDataLength,
    const uint8_t CDMData[], const uint16_t CDMDataLength, OpenCDMSessionCallbacks* callbacks, void* userData,
    struct OpenCDMSession** session)
{
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] opencdm_construct_session - Enter\n", __FUNCTION__, __LINE__);
    pCallbacks = callbacks;
    *session = (struct OpenCDMSession*) gOcdmSession;
    gOcdmSession++;
    pUserData = userData;

    return ERROR_NONE;
}

OpenCDMError opencdm_destruct_system(struct OpenCDMSystem* system)
{
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] opencdm_destruct_system - Enter\n", __FUNCTION__, __LINE__);
	return ERROR_NONE;
}

OpenCDMError opencdm_session_update(struct OpenCDMSession* session,
									const uint8_t keyMessage[],
									const uint16_t keyLength)
{
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] opencdm_session_update - Enter - Message = %d, KeyLength = %d\n", __FUNCTION__, __LINE__, keyMessage[0], keyLength);

    const char url[] = "MockDestinationURL";
    const uint8_t testChallenge[4][256] = {
		// 'individualization-request' or '3' should be accepted
        //"3:Type:{\"method\" : \"filter-commands\", \"commands\": [ {\"command\" : \"create\" }, {\"command\" : \"start\" }, {\"command\" : \"destroy\" }] }",
        "3:Type:{\"method\" : \"filter-commands\", \"commands\": [ {\"command\" : \"create\" }] }",
        //"3:Type:{\"method\" : \"private-data\", \"data\" : \"This is the Private Data\" }",
        "3:Type:{\"method\": \"private-data\", \"data\": \"{\\\"name\\\": \\\"keySlotHandle\\\", \\\"handle\\\": [232, 180, 153, 192]}\"}",
        "individualization-request:Type:{\"method\" : \"public-data\", \"data\" : \"This is the public Data\" }",
	"3:Type:{\"method\" : \"filter-commands\", \"commands\": [ {\"command\" : \"start\" }] }"
	};

    if((keyLength == 1) && (keyMessage[0] < 4)) {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] - keyMessage[0] - For Simulation\n", __FUNCTION__, __LINE__);
        pCallbacks->process_challenge_callback(session, pUserData,  url, testChallenge[keyMessage[0]], sizeof(testChallenge[keyMessage[0]]));
        const char message[] = "{\"status\" : \"FATAL\", \"errorNo\" : 3601, \"message\" : \"Tuning Failed\" }";
        pCallbacks->error_message_callback(session, pUserData, message);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] - testChallenge[1] - For Simulation\n", __FUNCTION__, __LINE__);
        pCallbacks->process_challenge_callback(session, pUserData,  url, testChallenge[1], sizeof(testChallenge[1]));
    }
    else if(keyLength > 1) {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] - keyMessage = %s\n", __FUNCTION__, __LINE__, (char*)keyMessage);
        pCallbacks->process_challenge_callback(session, pUserData,  url, testChallenge[2], sizeof(testChallenge[2]));
    }
    else
    {
        const char message[] = "{\"status\" : \"FATAL\", \"errorNo\" : 3601, \"message\" : \"Tuning Failed\" }";
        pCallbacks->error_message_callback(session, pUserData, message);
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] - testChallenge[3] - For Simulation\n", __FUNCTION__, __LINE__);
        pCallbacks->process_challenge_callback(session, pUserData,  url, testChallenge[3], sizeof(testChallenge[3]));
    }

	return ERROR_NONE;
}

OpenCDMError opencdm_session_close(struct OpenCDMSession* session)
{
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] opencdm_session_close - Enter\n", __FUNCTION__, __LINE__);
	return ERROR_NONE;
}

OpenCDMError opencdm_destruct_session(struct OpenCDMSession* session)
{
     RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] opencdm_destruct_session - Enter\n", __FUNCTION__, __LINE__);
	return ERROR_NONE;
}

#ifdef __cplusplus
}
#endif