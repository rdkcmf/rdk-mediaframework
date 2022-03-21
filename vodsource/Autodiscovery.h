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

#ifndef AUTODISCOVERY_H
#define AUTODISCOVER_H

/**
 * @file Autodiscovery.h
 * @brief This header file contains the prototypes of member functions of class AutoDiscovery
 */

#ifdef GCC4_XXX
#include <map>
#include <string>
#else
#include <map.h>
#include <string.h>
#endif 

#include "rmfvodsource.h"
#include "rmfcore.h"
#include "rmf_osal_thread.h"

#ifdef GLI
#include "libIBus.h"
#include "libIBusDaemon.h"
#endif

#define RMF_VOD_AD_INSERTED (0x100)

static rmf_Error getDacId(uint16_t *pDACId);
static rmf_Error getplantid(uint32_t *pPlantID);

/**
 * @class AutoDiscovery
 * @brief This class defines the parameters and methods for handling the auto discovery. The methods of this class
 * do the several operations such as register and unregister VOD service, tune the requested
 * frequency to get the TSID by using QAM Source module, handling power events by using IARMBUS components and
 * inform to VOD client about power state change, handles various notification such as end of stream, invoke
 * a task for handling auto discovery, etc.
 * @ingroup vodsourceclass
 */
class AutoDiscovery
{
public:
	AutoDiscovery();
	~AutoDiscovery();

	static AutoDiscovery* getInstance();
#ifdef GLI
	static IARM_Result_t _PowerPreChange(void *arg);
	static void PartnerIdChange( void *data, size_t len);
	static void PlantIdReceived(void *data, size_t len);
#endif
	RMFResult start();
	void adHandler( void* arg );
	RMFResult triggerAutoDiscovery();
	RMFResult stopAutoDiscovery();

#ifdef GLI
	RMFResult handleLowPower();
	RMFResult handleFullPower();
	RMFResult handlePartnerIdChange();
#endif
	RMFResult tuneAndGetTSID( int srcId, int *pTSID );
	RMFResult tuneAndGetTSID( int freq, int mod, int *pTSID );
	RMFResult GetNumTunersForAD( int *pNumTuners );
        /**
        * @fn RMFResult setAutoDiscoveryDone()
        * @brief This function is used to set the flag m_adCompleted to True.
        * @return None
        */
	RMFResult setAutoDiscoveryDone(){ m_adCompleted = true; }
        /**
         * @fn bool getAutoDiscoveryStatus()
         * @brief This function is used to get the status of auto discovery.
         * @return Returns True if auto discover has triggered for VOD client, else returns False.
         */
    void resetAutoDiscoveryStatus() { m_adCompleted = false; }
	bool getAutoDiscoveryStatus(){ return m_adCompleted; }
	bool isADTriggered(){ return m_adTriggered; }
	bool isADFailed(){ return m_adFailed; }
	bool registerVodSource(std::string sessionId, void* vodSrc);
	bool unregisterVodSource(std::string sessionId);
	void notifyAdInsertion( unsigned int event );
	bool getAdInsertionStatus();
	void clearAdInsertionQueue();
	RMFResult releaseADTuner();

private:
	int jsonMsgHandler( char *recvMsg, int recvMsgSz, char *respMsg,
		int *respMsgBufSz );
	void notifyEndOfStream(std::string sessionId);
	void notifyVODSrc( std::string sessionId, uint32_t eventCode );

	static AutoDiscovery* m_adInstance; //!< An instance of auto discovery class
	int m_servSock; //!< Socket descriptor
	bool m_done; 
	bool m_adTriggered; //!< boolean variable for triggering the autodiscovery is done or not
	bool m_adCompleted; //!< If the attribute is True means auto discovery completed by VOD client
	bool m_adFailed;
	bool m_adTuningInprogress;
	rmf_osal_ThreadId m_tid; //!< Thread Id for auto discovery handler
	std::map<std::string, void*> m_vodSourceMap; //!< A map table for keeping the object for each VOD sessions
	rmf_osal_eventqueue_handle_t m_adNotificationQueue;
};
#endif
 
