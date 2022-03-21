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

#ifndef RMFSDVSOURCE_H
#define RMFSDVSOURCE_H
#include "rmfqamsrc.h"
#include "rmf_osal_sync.h"

#include "SDVForceTuneEventHandler.h"


/**
 * @file rmfsdvsource.h
 * @brief It contains the SDV source data.
 */

/**
 * @defgroup rmfsdv QAM Source Base SDV Source
 * @ingroup rmfqamsrc
 * @defgroup rmfsdvclass QAM SDV Class
 * @ingroup rmfsdv
 */


/**
 * @class RMFSDVSrc
 * @brief Class extending RMFQAMSrc to implement RMFSDVSrc.
 * @ingroup rmfsdvclass
 */
class RMFSDVSrc:public RMFQAMSrc
{
public:
    RMFSDVSrc(  );
    ~RMFSDVSrc(  );
    RMFMediaSourcePrivate *createPrivateSourceImpl(  );
    static bool isSwitched(const char * ocapLocator);
    static RMFResult init_platform();
    static RMFResult uninit_platform();
    static void forceTuneEventHandler(void * ptrObj, uint32_t sourceId);
    static rmf_Error sdvSwitchedListUpdated(void * ptrObj, uint8_t *data, uint32_t size);
    /**
     * @brief IARM Bus callback used by SDV Agent to enable the SDV subcomponent in RMF.
     * @details RMF SDV will only be enabled if this function is able to acquire both configuration template URLs.
     * If URLs are not detected, assumption is made that this receiver is not currently on an SDV network and
     * therefore SDV will remain dormant except for this registered IARM Bus callback.
     *
     * @param payload - IARM bus payload used to indicate to the caller whether or not the RMF SDV was enabled.
     * @return IARM_RESULT_SUCCESS
     */
    static IARM_Result_t enableSdv(IARM_RMFSDV_ENABLE_PAYLOAD *payload);
    static bool isSdvEnabled();
    /**
     * @brief Static reference to the SDVForceTuneEventHandler that all SDV Sources can register itself for Force Tune Events when
     * tuning to a SDV Channel.
     */
    static SDVForceTuneEventHandler * forceTuneHandler;

private:
    static uint16_t *           m_sdvChanneMap;                       //!< Array of SDV Sourceids
    static uint16_t             m_sdvChanneMapSize;                   //!< Number of sourceids in the switched list
    static rmf_osal_Mutex       m_sdvSwitchedListMutex;               //!< Mutex to protect access to switched list
    static bool                 m_isEnabled;                          //!< indicates RMF SDV has been enabled
};
#endif // RMFSDVSOURCE_H
