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
 * @file ippvclient.h
 * @brief IPPV client
 */

/**
 * @defgroup ippvsource IPPV Source
 * @ingroup RMF
 *
 * @defgroup ippvclientapi IPPV Client API list
 * <b>IPPV Client is responsible for the following operations. </b>
 * <ul>
 * <li> IPPV event reads the environment variable for CANH Server Port number from rmfconfig.ini configuration file.
 * <li> IPPV IPC client sends commands to CANH IPC server which uses CANH IPC buffer mechanism and it gets the response in string
 * from uint32_t getRMFVal( uint32_t response ) function.
 * <li> Mediastreamer takes  IPPV event streaming requests and parses the url and uses iPPVSrc to handle requests from outside.
 * </ul>
 * @ingroup ippvsource
 */
#ifndef __IPPVCLIENT_H__
#define __IPPVCLIENT_H__
#include <stdint.h>
//#include "rmf_osal_sync.h"
//#include "vlMutex.h"


/**
 * @defgroup ippvclass IPPV Class
 * @ingroup ippvsource
 */

/**
 * @defgroup ippvmacros IPPV Macros
 * @ingroup ippvsource
 * @{
 */

/* IPPV Status Identifiers */
/**  Returns if IPPV event is successfully purchased. */
#define RMF_IPPVSRC_SUCCESS RMF_SUCCESS
/** Returns if IPPV event is pending for purchase. */
#define RMF_IPPVSRC_PURCHASE_PENDING	( RMF_ERRBASE_IPPVSRC +1 )
/** Returns if IPPV event is not attempted to purchase. */
#define RMF_IPPVSRC_PURCHASE_NOT_ATTEMPTED	  ( RMF_ERRBASE_IPPVSRC +2 )
/** Returns when IPPV source is already authorized. */
#define RMF_IPPVSRC_ALREADY_AUTHORIZED	  ( RMF_ERRBASE_IPPVSRC +3 )
/** Returns if Impulsive PPV event is disabled. */
#define RMF_IPPVSRC_IMPULSE_DISABLED	( RMF_ERRBASE_IPPVSRC +4 )
/** Returns if there is no memory for IPPV event. */
#define RMF_IPPVSRC_NOMEM	 ( RMF_ERRBASE_IPPVSRC +5 )
/**  Returns when PIN code is failed to access IPPV event. */
#define RMF_IPPVSRC_PIN_FAILED	  ( RMF_ERRBASE_IPPVSRC +6 )
/** Returns if credits fails for purchasing IPPV events. */
#define RMF_IPPVSRC_CREDIT_FAILURE	  ( RMF_ERRBASE_IPPVSRC +7 )
/** Returns if IPPV event is not having no persistent memory. */
#define RMF_IPPVSRC_NO_PERSISTENT_MEMORY	( RMF_ERRBASE_IPPVSRC +8 )
/** Returns when IPPV event is not available for purchase. */
#define RMF_IPPVSRC_PURCHASE_UNAVAILABLE	( RMF_ERRBASE_IPPVSRC +9 )
/** Returns while IPPV event is outside of purchase window. */
#define RMF_IPPVSRC_OUTSIDE_PURCHASE_WINDOW    ( RMF_ERRBASE_IPPVSRC +10 )
/** Returns when maximum IPPV event is requesting for purchase. */
#define RMF_IPPVSRC_MAX_EVENTS	  ( RMF_ERRBASE_IPPVSRC +11 )
/** Returns if successfully cancelled the IPPV event. */
#define RMF_IPPVSRC_CANCEL_SUCCESS	  ( RMF_ERRBASE_IPPVSRC +12 )
/** Return if failed to cancel the IPPV event. */
#define RMF_IPPVSRC_CANCEL_FAILED	 ( RMF_ERRBASE_IPPVSRC +13 )
/** Returns when bad event is used to cancel the IPPV event. */
#define RMF_IPPVSRC_CANCEL_BAD_EID	  ( RMF_ERRBASE_IPPVSRC +14 )
/** Returns if program index in not configured. */
#define RMF_IPPVSRC_UNCONFIGURED_PROGRAM_INDEX	  ( RMF_ERRBASE_IPPVSRC +15 )
/** Returns when invalid source Id is used. */
#define RMF_IPPVSRC_INVALID_SOURCE_ID	 ( RMF_ERRBASE_IPPVSRC +16 )
/** Returns when invalid start time of purchase window has been set. */
#define RMF_IPPVSRC_INVALID_START_TIME	  ( RMF_ERRBASE_IPPVSRC +17 )
/** Returns when maximum IPPV event is requested for purchase simultaneously. */
#define RMF_IPPVSRC_MAX_SIMULTANEOUS_PURCHASES	  ( RMF_ERRBASE_IPPVSRC +18 )
/** Returns if IPPV event purchase limit is reached. */
#define RMF_IPPVSRC_PURCHASE_LIMIT_REACHED	  ( RMF_ERRBASE_IPPVSRC +19 )
/** Returns if IPPV event system time of purchase window is not received. */
#define RMF_IPPVSRC_SYSTEM_TIME_NOT_RXD    ( RMF_ERRBASE_IPPVSRC +20 )
/** Returns if value of zero would prevent IPPV purchases in purchase window. */
#define RMF_IPPVSRC_ZERO_VALUE_PURCHASE_WINDOW	  ( RMF_ERRBASE_IPPVSRC +21 )
/**  Returns if CANH is not ready. */
#define RMF_IPPVSRC_OTHER_FAILURE	 ( RMF_ERRBASE_IPPVSRC +22 )
/** Returns when IPPV event is not ready for purchase. */
#define RMF_IPPVSRC_NOT_READY ( RMF_ERRBASE_IPPVSRC +23 )

/** @} */

#define RMF_IPPVSRC_MAX_PLANTID_LEN 64
extern uint32_t getRMFVal( uint32_t response );


/**
 * @class IppvClient
 * @brief Class implements the interactive/impulsive pay per view source event mechanism.
 * It defines the method variables as event id and source id for IPPV event purchase.
 * Then it purchases the IPPV event and updates the purchase status.
 * @ingroup ippvclass
 */
class IppvClient
{
  private:
	uint32_t eID;
	uint16_t sourceID;
	uint32_t getEID(  );
	//vlMutex m_lock;

  public:
	IppvClient(  );
	uint32_t start(  );
	uint32_t stop(	);
	void setEID( uint32_t eID );
	void setSourceID( uint16_t sourceID );

	uint32_t purchaseIppvEvent( uint32_t eID );
	uint32_t cancelIppvEvent( uint32_t purchaseID );
	bool isIppvEnabled(  );
	uint32_t getPurchaseStatus( uint32_t eID );
	uint32_t isIppvEventAuthorized( uint32_t eID, uint8_t * isFreePreviewable,
									uint8_t * isAuthorized );

	const char *getStatusByString( uint32_t response );

};

#endif

