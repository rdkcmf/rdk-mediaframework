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


#include <stdio.h>
#include <stdlib.h>

#include "ai_state_machine.h"
#include "poddef.h"
#include <lcsm_log.h>                   /* To support log file */

#include "debug.h"
#include "utils.h"
#include <rdk_debug.h>
#ifdef __cplusplus
extern "C" {
#endif 

char szaStateNames [AI_EXIT+1][30] = {
    "AI_UNINITIALIZED",
    "AI_INITIALIZED",
    "AI_STARTED",
    "AI_SESSION_OPEN_REQUESTED",
    "AI_SESSION_OPENED",
    "AI_APPINFO_SENT",
    "AI_APPINFO_CONFIRMED",
    "AI_EXIT"
};

static AIState aiState = {
    state : AI_UNINITIALIZED,
    stateMgr : StateMgr_Uninitialized,
    exit : TRUE,
    sessionOpen : FALSE,
    sessionId : 0,
    aInfo : NULL,
    aInfo_len : 0
};

int aiChangeState(AIStateType newState)
{
    return (*aiState.stateMgr)(newState);
}

int aiExit()
{
    return aiState.exit;
}

uint16 aiSessionId()
{
    return aiState.sessionId;
}

Bool aiSessionOpen()
{
    return aiState.sessionOpen;
}

void aiSetSessionId(uint16 sid)
{
    aiState.sessionId = sid;
}

void aiSetInfo(uint8 *data, uint16 len)
{
    aiState.aInfo = data;
    aiState.aInfo_len = len;
}

uint8* aiInfo(uint16 *len)
{
    *len = aiState.aInfo_len;
    return aiState.aInfo;
}

AIStateType aiGetState()
{
    return aiState.state;
}

// State manager for UNINITIALIZED state
int StateMgr_Uninitialized (AIStateType newState)
{
    MDEBUG ( DPM_APPINFO, "ai:Called\n");

    switch (newState) {

        case AI_UNINITIALIZED:
        break;

        case AI_INITIALIZED:
            if (pthread_mutex_init(&(aiState.lock), NULL) != 0) {
                MDEBUG (DPM_ERROR, "ERROR:ai: Unable to initialize state mutex.\n");
                return EXIT_FAILURE;
            }
            aiState.exit = FALSE;
            aiState.sessionOpen = FALSE;
            aiState.sessionId = 0;
            aiState.stateMgr = StateMgr_Initialized;
            aiState.state = AI_INITIALIZED;
            aiState.aInfo = NULL;
            aiState.aInfo_len = 0;
        break;

        case AI_EXIT:
            aiState.exit = TRUE;
            aiState.state = AI_EXIT;
        break;

        default:
            MDEBUG (DPM_ERROR, "ERROR:ai: Invalid change of state. (@1)\n");
            return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int StateMgr_Initialized (AIStateType newState)
{
    MDEBUG ( DPM_APPINFO, "ai:newState = %s\n", szaStateNames [newState]);

    switch (newState) {

        case AI_INITIALIZED:
        break;

        case AI_STARTED:
            aiState.exit = FALSE;
            aiState.sessionOpen = FALSE;
            aiState.sessionId = 0;
            aiState.stateMgr = StateMgr_Started;
            aiState.state = AI_STARTED;
        break;

        case AI_EXIT:
            aiState.exit = TRUE;
            aiState.state = AI_EXIT;
        break;

        default:
            MDEBUG (DPM_ERROR, "ERROR:ai: Invalid change of state. (@2)\n");
            return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int StateMgr_Started (AIStateType newState)
{
    MDEBUG ( DPM_APPINFO, "ai:newState = %s\n", szaStateNames [newState]);

    switch (newState) {

        case AI_STARTED:
        break;

        case AI_SESSION_OPEN_REQUESTED:
            aiState.exit = FALSE;
            aiState.sessionOpen = FALSE;
            aiState.sessionId = 0;
            aiState.stateMgr = StateMgr_SessOpenRequested;
            aiState.state = AI_SESSION_OPEN_REQUESTED;
        break;

        case AI_EXIT:
            aiState.exit = TRUE;
            aiState.state = AI_EXIT;
        break;

        default:
            MDEBUG (DPM_ERROR, "ERROR:ai: Invalid change of state. (@3)\n");
            return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int StateMgr_SessOpenRequested (AIStateType newState)
{
    MDEBUG ( DPM_APPINFO, "ai:newState = %s\n", szaStateNames [newState]);

    switch (newState) {

        case AI_SESSION_OPEN_REQUESTED:
        break;

        case AI_STARTED:
            StateMgr_Initialized(AI_STARTED);
        break;

        case AI_SESSION_OPENED:
            aiState.exit = FALSE;
            aiState.sessionOpen = TRUE;
            aiState.stateMgr = StateMgr_SessOpened;
            aiState.state = AI_SESSION_OPENED;
        break;

        case AI_EXIT:
            aiState.exit = TRUE;
            aiState.state = AI_EXIT;
        break;

        default:
            MDEBUG (DPM_ERROR, "ERROR:ai: Invalid change of state. (@4)\n");
            return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int StateMgr_SessOpened (AIStateType newState)
{
    MDEBUG ( DPM_APPINFO, "ai:newState = %s\n", szaStateNames [newState]);

    switch (newState) {

        case AI_SESSION_OPENED:
        break;

        case AI_STARTED:
            StateMgr_Initialized(AI_STARTED);
        break;

        case AI_APPINFO_SENT:
            aiState.exit = FALSE;
            aiState.sessionOpen = TRUE;
            aiState.stateMgr = StateMgr_AppInfoSent;
            aiState.state = AI_APPINFO_SENT;
        break;

        case AI_EXIT:
            aiState.exit = TRUE;
            aiState.state = AI_EXIT;
        break;

        default:
            MDEBUG (DPM_ERROR, "ERROR:ai: Invalid change of state. (@5)\n");
            return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int StateMgr_AppInfoSent (AIStateType newState)
{
    MDEBUG ( DPM_APPINFO, "ai:newState = %s\n", szaStateNames [newState]);

    switch (newState) {

        case AI_APPINFO_SENT:
        break;

        case AI_STARTED:
            StateMgr_Initialized(AI_STARTED);
        break;

        case AI_APPINFO_CONFIRMED:
            aiState.exit = FALSE;
            //aiState.sessionOpen = TRUE;
            aiState.stateMgr = StateMgr_AppInfoConfirmed;
            aiState.state = AI_APPINFO_CONFIRMED;
        break;

        case AI_EXIT:
            aiState.exit = TRUE;
            aiState.state = AI_EXIT;
        break;

        default:
            MDEBUG (DPM_ERROR, "ERROR:ai: Invalid change of state. (@6)\n");
            return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;

}

int StateMgr_AppInfoConfirmed (AIStateType newState)
{
    MDEBUG ( DPM_APPINFO, "ai:newState = %s\n", szaStateNames [newState]);

    switch (newState) {

        case AI_APPINFO_CONFIRMED:
        break;

        case AI_STARTED:
            StateMgr_Initialized(AI_STARTED);
        break;

        //case AI_SERVER_QUERIED:
            /*aiState.exit = FALSE;
            aiState.sessionOpen = TRUE;
            aiState.stateMgr = StateMgr_AppInfoSent;
            aiState.state = AI_APPINFO_SENT;*/
        //break;

        case AI_EXIT:
            aiState.exit = TRUE;
            aiState.state = AI_EXIT;
        break;

        default:
            MDEBUG (DPM_ERROR, "ERROR:ai: Invalid change of state. (@7)\n");
            return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}


#ifdef __cplusplus
}
#endif 

