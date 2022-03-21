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
* ============================================================================
* @file SDVForceTuneEventHandler.cpp
* @brief Implementation of SDV Force Tune Event Handler
* @author Spencer Gilroy (sgilroy@cisco.com)
* @Date Sept 11 2014
*/
#include "include/SDVForceTuneEventHandler.h"
#include "rdk_debug.h"
#include "pthread.h"

#include <cstdio>
#include <cstddef>
#include <cstring> 
#include <cassert>

SDVForceTuneEventHandler::FORCE_TUNE_HANDLER_MAP SDVForceTuneEventHandler::handlerMap;
pthread_mutex_t SDVForceTuneEventHandler::mapLock;

/**
 * @fn SDVForceTuneEventHandler()
 * @brief This function is a default constructor. It initialises a mutex for accessing handlerMap.
 *
 * @return None
 */
SDVForceTuneEventHandler::SDVForceTuneEventHandler(){
    assert(pthread_mutex_init(&mapLock, NULL) == 0);
}

/**
 * @fn SDVForceTuneEventHandler::start()
 *
 * @brief This function starts the SDVForceTuneHandler by registering a handler for SDVAGENT_REQUEST_FORCE_TUNE event
 * with IARM bus.
 *
 * @result Returns RMF_RESULT_SUCCESS if registration of event handler with IARM BUS is successful
 * else returns RMF_RESULT_FAILURE indicating invalid parameters passed internally.
 */
RMFResult SDVForceTuneEventHandler::start(){
    IARM_Result_t res =  IARM_Bus_RegisterEventHandler( 
                            IARM_BUS_SDVAGENT_NAME,                                                  /* Owner of the Event */
                            SDVAGENT_REQUEST_FORCE_TUNE,                                      /* Event ID from this owner*/
                            (IARM_EventHandler_t)SDVForceTuneEventHandler::handleBroadcastEvent      /* IARM_EventHandler_t */
                         );
    if(res = IARM_RESULT_SUCCESS){
        return RMF_RESULT_FAILURE;
    }

    return RMF_RESULT_SUCCESS;
}

/**
 * @fn SDVForceTuneEventHandler::stop()
 *
 * @brief This function stops the SDVForceTuneHandler by unregistering the SDVAGENT_REQUEST_FORCE_TUNE
 * event handler from IARM bus and clearing the handler map.
 *
 * @return RMF_RESULT_SUCCESS after unregistering for IARM Bus Events.
 */
RMFResult SDVForceTuneEventHandler::stop(){
 
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "[SDVForceTuneEventHandler] Unregistering Force Tune Event Handler\n");
    IARM_Bus_UnRegisterEventHandler(IARM_BUS_SDVAGENT_NAME, SDVAGENT_REQUEST_FORCE_TUNE); 

    SDVForceTuneEventHandler::handlerMap.clear();

    return RMF_RESULT_SUCCESS;
}

/**
 * @fn SDVForceTuneEventHandler::registerEventHandler(uint32_t tunerId, SDV_FORCE_TUNE_HANDLER_t handler)
 * @brief This function registers the event handler for sdv force tune events against the specified tuner id.
 * The entry/registration is made in the force tune handler map.
 *
 * @param[in] tunerId Indicates the tuner id for registering handler with.
 * @param[in] handler Specifies the function to be called when force tune event is received for the
 * corresponding tuner Id
 *
 * @result RMF_RESULT_SUCCESS Indicates registration of event handler was successful.
 */
RMFResult SDVForceTuneEventHandler::registerEventHandler(uint32_t tunerId, SDV_FORCE_TUNE_HANDLER_t handler){
    pthread_mutex_lock(&mapLock);
    SDVForceTuneEventHandler::handlerMap[tunerId] = handler;
    pthread_mutex_unlock(&mapLock);

    return RMF_RESULT_SUCCESS;
}

/**
 * @fn SDVForceTuneEventHandler::unregisterEventHandler(uint32_t tunerId)
 * @brief This function removes the event handler registered for the specfied tunerId from
 * the force tune handler map.
 *
 * @param[in] tunerId Specifies the tuner Id for which the event handler should be removed.
 *
 * @return RMF_RESULT_SUCCESS Indicates unregistration of event handler was successful.
 */
RMFResult SDVForceTuneEventHandler::unregisterEventHandler(uint32_t tunerId){
    pthread_mutex_lock(&mapLock);
    SDVForceTuneEventHandler::FORCE_TUNE_HANDLER_MAP::iterator it = SDVForceTuneEventHandler::handlerMap.find(tunerId);
    SDVForceTuneEventHandler::handlerMap.erase(it);
    pthread_mutex_unlock(&mapLock);

    return RMF_RESULT_SUCCESS;
}

/**
 * @fn SDVForceTuneEventHandler::~SDVForceTuneEventHandler()
 * @brief This function is a default destructor. It releases and destroys the mutex which was
 * created for force tune handler map.
 *
 * @return None
 */
SDVForceTuneEventHandler::~SDVForceTuneEventHandler() {

    pthread_mutex_unlock(&mapLock);
    pthread_mutex_destroy(&mapLock);
}

/**
 * @fn SDVForceTuneEventHandler::handleBroadcastEvent(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
 * @brief This function gets invoked when SDV force tune event is broadcasted to the IARM bus. It makes a call back to the function
 * registered against the tuner id in the force tune handler map.
 *
 * @param[in] owner Specifies the owner of the event.
 * @param[in] eventId Specifies the id of the event received.
 * @param[in] data Pointer to the event data.
 * @param[in] len Specifies the size of the event data.
 *
 * @return Returns IARM_RESULT_SUCCESS if the call was successful else returns IARM_RESULT_INVALID_PARAM.
 */
IARM_Result_t SDVForceTuneEventHandler::handleBroadcastEvent(const char *owner, IARM_EventId_t eventId, void *data, size_t len){

    IARM_Result_t ret = IARM_RESULT_SUCCESS;

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVForceTuneEventHandler] Received Event: %d from %s\n", eventId, owner);
    if(strcmp(owner, IARM_BUS_SDVAGENT_NAME) == 0 && eventId == SDVAGENT_REQUEST_FORCE_TUNE){
        IARM_SDVAGENT_FORCE_TUNE_PAYLOAD* payload = (IARM_SDVAGENT_FORCE_TUNE_PAYLOAD*) data;

        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVForceTuneEventHandler] Received Force Tune event for tunerId: %d\n", payload->tunerId);
        pthread_mutex_lock(&mapLock);
        SDVForceTuneEventHandler::FORCE_TUNE_HANDLER_MAP::iterator iter = SDVForceTuneEventHandler::handlerMap.find(payload->tunerId);
        if(iter != SDVForceTuneEventHandler::handlerMap.end()){
            iter->second.callback(iter->second.ptrObj, payload->currentSourceId);
        }
        else {
            ret = IARM_RESULT_INVALID_PARAM;
        }
        pthread_mutex_unlock(&mapLock);
    }
    else {
        ret = IARM_RESULT_INVALID_PARAM;
    }

    return ret;
}
