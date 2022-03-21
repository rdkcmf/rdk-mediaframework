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

/**
 * @file rmfproxyservice.h
 * @brief RMF Proxy Service header file.
 * This file contains the public interfaces for RMF Proxy Service.
 *
 * @defgroup rmfproxyservice RMF Proxy Service
 * RMF Proxy Service provides APIs to extract the system ids required for
 * authentication of the box. These are following operation available for this
 * sub module.
 * <ul>
 * <li> Get Channel Map ID and Controller ID from OOB SI.
 * <li> Get VOD Server Id.
 * <li> Get Plant ID.
 * </ul>
 * @ingroup RMF
 * @defgroup rmfproxyserviceapi RMF Proxy Service API list
 * Describe the details about RMF Proxy service API specifications.
 * @ingroup rmfproxyservice
*/

#ifndef RMFPROXYSERVICE_H
#define RMFPROXYSERVICE_H

#include "rmf_osal_error.h"

rmf_Error getsystemids(uint16_t *pChannelMapID, uint16_t *pControllerID);

rmf_Error getvodserverid(uint32_t *pVodServerID);

rmf_Error getplantid(uint32_t *pPlantID);

void getDiagnostics();

//------------------------------
#endif // RMFPROXYSERVICE_H

