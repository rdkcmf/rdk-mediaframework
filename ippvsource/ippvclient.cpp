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
 * @file ippvclient.cpp
 * @brief Interactive/Impulsive pay per view client side.
 */
#include "ippvclient.h"
#include "canhClient.h"
#include "rmf_osal_util.h"
#include <iostream>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <stdint.h>
#include <inttypes.h>
#include <rmf_osal_ipc.h>
#include <rdk_debug.h>
#include <rmf_pod.h>


using namespace std;

/* IPPV Status Identifiers */
#define IPPV_SUCCESS 256
#define IPPV_PURCHASE_PENDING 257
#define IPPV_PURCHASE_NOT_ATTEMPTED 258
#define IPPV_ALREADY_AUTHORIZED 259
#define IPPV_IMPULSE_DISABLED 260
#define IPPV_NOMEM 261
#define IPPV_PIN_FAILED 262
#define IPPV_CREDIT_FAILURE 263
#define IPPV_NO_PERSISTENT_MEMORY 264
#define IPPV_PURCHASE_UNAVAILABLE 265
#define IPPV_OUTSIDE_PURCHASE_WINDOW 266
#define IPPV_MAX_EVENTS 267
#define IPPV_CANCEL_SUCCESS 268
#define IPPV_CANCEL_FAILED 269
#define IPPV_CANCEL_BAD_EID 270
#define IPPV_UNCONFIGURED_PROGRAM_INDEX 304
#define IPPV_INVALID_SOURCE_ID 305
#define IPPV_INVALID_START_TIME 306
#define IPPV_MAX_SIMULTANEOUS_PURCHASES 307
#define IPPV_PURCHASE_LIMIT_REACHED 308
#define IPPV_SYSTEM_TIME_NOT_RXD 309
#define IPPV_ZERO_VALUE_PURCHASE_WINDOW 310
#define IPPV_OTHER_FAILURE 335
#define IPPV_NOT_READY 500
#define EXPIRATION_PERIOD 1987200000L
#define PURGE_PERIOD 86400000L
static volatile uint32_t canh_server_port_no;

static const uint32_t COMMAND_PURCHASE_IPPV_EVENT = 1;
static const uint32_t COMMAND_CANCEL_IPPV_EVENT = 2;
static const uint32_t COMMAND_IS_IPPV_ENABLED = 3;
static const uint32_t COMMAND_GET_PURCHASE_STATUS = 4;
static const uint32_t COMMAND_IS_IPPV_EVENT_AUTHORIZED = 5;

uint32_t getRMFVal( uint32_t response );


/**
 * @fn IppvClient::IppvClient(  )
 * @brief This constructor is used to initialize and create an instance of ippv client source.
 * It reads the environment variable for "CANH_SERVER_PORT_NO" from rmfconfig.ini configuration file.
 *
 * @param None.
 * @return None.
 * @ingroup ippvclientapi
 */
IppvClient::IppvClient(  )
{
	eID = 0;
	sourceID = 0;
	const char *canh_server_port;
	canh_server_port = rmf_osal_envGet( "CANH_SERVER_PORT_NO" );
	if( NULL == canh_server_port )
	{
	       RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV", "%s: NULL POINTER  received for name string\n", __FUNCTION__ );
		canh_server_port_no = CANH_SERVER_DFLT_PORT_NO;
	}
	else
	{
		canh_server_port_no = atoi( canh_server_port );
	}
        
}

void IppvClient::setEID( uint32_t eID )
{
	//VL_AUTO_LOCK(m_lock);
	this->eID = eID;
}

void IppvClient::setSourceID( uint16_t sourceID )
{
	//VL_AUTO_LOCK(m_lock);
	this->sourceID = sourceID;
}

uint32_t IppvClient::getEID(  )
{
	//VL_AUTO_LOCK(m_lock);
	return this->eID;
}


/**
 * @fn uint32_t IppvClient::purchaseIppvEvent( uint32_t eID )
 * @brief This function performs the purchase the Ippv event for the specified entitlement id [eId].
 * Function checks whether CANH is ready or not.
 * IPPV IPC client requests data through command to purchase IPPV event .
 *
 * @param[in] eID Entitlement ID to purchase IPPV event.
 *
 * @return rmfVal
 * @retval RMF_IPPVSRC_OTHER_FAILURE If CANH is not ready.
 * @retval IPPV_SUCCESS If IPPV event is successfully purchased. Else
 * @ref getRMFVal( uint32_t response ) function for other approximate return values.
 * @ingroup ippvclientapi
 */
uint32_t IppvClient::purchaseIppvEvent( uint32_t eID )
{
	uint32_t response = 0;
	int32_t result_recv;
	int32_t retry_cnt = 0;

	if ( !isCANHReady())
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV",
				 "Error!!Canh not started yet." );
		return RMF_IPPVSRC_OTHER_FAILURE;
	}

	do {
		IPCBuf ipcBuf = IPCBuf( ( uint32_t ) COMMAND_PURCHASE_IPPV_EVENT );
		ipcBuf.addData( ( const void * ) &eID, sizeof( eID ) );
		if( 0 != canh_server_port_no )
		{
			const char *canh_server_port;
			canh_server_port = rmf_osal_envGet( "CANH_SERVER_PORT_NO" );
			if( NULL == canh_server_port )
			{
			       RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV", "%s: NULL POINTER  received for name string\n", __FUNCTION__ );
				canh_server_port_no = CANH_SERVER_DFLT_PORT_NO;
			}
			else
			{
				canh_server_port_no = atoi( canh_server_port );
			}
		}
		
		int8_t res = ipcBuf.sendCmd( canh_server_port_no );
		if ( 0 == res )
		{
			result_recv = ipcBuf.getResponse( &response );
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.IPPV", "%s:ippv status is [%s]\n",
				 __FUNCTION__, getStatusByString( getRMFVal( response ) ) );
		}
		retry_cnt++;
		if ( response == IPPV_ALREADY_AUTHORIZED || response == IPPV_SUCCESS || retry_cnt >= 5 )
		{
			break;
		} 
		sleep(2);
	} while ( response != IPPV_SUCCESS );

	return getRMFVal( response );
}


/**
 * @fn uint32_t IppvClient::cancelIppvEvent( uint32_t purchaseID )
 * @brief This function is used to cancel the Ippv event using purchase id.
 * Function checks whether CANH is ready or not.
 * IPPV IPC client requests data through command to cancel IPPV event which has been purchased .
 *
 * @param[in] purchaseID Purchase ID to cancel IPPV event.
 *
 * @return rmfVal
 * @retval RMF_IPPVSRC_OTHER_FAILURE If CANH is not ready.
 * @retval RMF_IPPVSRC_SUCCESS If IPPV event is successfully cancelled the IPPV event. Else,
 * @ref getRMFVal( uint32_t response ) function for other approximate return values.
 * @ingroup ippvclientapi
 */
uint32_t IppvClient::cancelIppvEvent( uint32_t purchaseID )
{
	uint32_t response = 0;
	int32_t result_recv;

	if ( !isCANHReady() )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV",
				 "Error!!Canh not started yet." );
		return RMF_IPPVSRC_OTHER_FAILURE;
	}

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) COMMAND_CANCEL_IPPV_EVENT );
	ipcBuf.addData( ( const void * ) &purchaseID, sizeof( purchaseID ) );
		if( 0 != canh_server_port_no )
		{
			const char *canh_server_port;
			canh_server_port = rmf_osal_envGet( "CANH_SERVER_PORT_NO" );
			if( NULL == canh_server_port )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV", "%s: NULL POINTER  received for name string\n", __FUNCTION__ );
				canh_server_port_no = CANH_SERVER_DFLT_PORT_NO;
			}
			else
			{
				canh_server_port_no = atoi( canh_server_port );
			}
		}

	int8_t res = ipcBuf.sendCmd( canh_server_port_no );
	if ( 0 == res )
	{
		result_recv = ipcBuf.getResponse( &response );
	}

	return getRMFVal( response );
}


/**
 * @fn bool IppvClient::isIppvEnabled(  )
 * @brief This function is used to check whether IPPV event is enabled or not.
 * Function checks whether CANH is ready or not.
 * IPPV IPC client requests data through command to check IPPV event is enabled or not.
 * Function checks whether ippv event is enabled or not by collecting the data from ipc buffer mechanism.
 *
 * @param None
 * @return bool
 * @retval true If IPPV event is enabled.
 * @retval false If IPPV event is disabled.
 * @ingroup ippvclientapi
 */
bool IppvClient::isIppvEnabled(  )
{
	uint32_t response = 0;
	int32_t result_recv;
	uint8_t isEnabled = 0;

	if ( !isCANHReady() )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV",
				 "Error!!Canh not started yet." );
		return false;
	}

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) COMMAND_IS_IPPV_ENABLED );
	if( 0 != canh_server_port_no )
	{
		const char *canh_server_port;
		canh_server_port = rmf_osal_envGet( "CANH_SERVER_PORT_NO" );
		if( NULL == canh_server_port )
		{
		       RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV", "%s: NULL POINTER  received for name string\n", __FUNCTION__ );
			canh_server_port_no = CANH_SERVER_DFLT_PORT_NO;
		}
		else
		{
			canh_server_port_no = atoi( canh_server_port );
		}
	}
	
	int8_t res = ipcBuf.sendCmd(canh_server_port_no );

	if ( 0 == res )
	{
		result_recv = ipcBuf.getResponse( &response );
		ipcBuf.collectData( ( void * ) &isEnabled, sizeof( isEnabled ) );
		if ( isEnabled )
			return true;
		else
			return false;
	}
	return false;
}


/**
 * @fn uint32_t IppvClient::getPurchaseStatus( uint32_t eID )
 * @brief This function gets the purchase status of Ippv event.
 * Function checks whether CANH is ready or not.
 * CANH client requests data through command to check the purchase status of IPPV event.
 *
 * @param[in] eID Entitlement ID to check the purchase status of IPPV event.
 *
 * @return rmfVal
 * @retval RMF_IPPVSRC_OTHER_FAILURE If CANH is not ready.
 * @retval RMF_IPPVSRC_SUCCESS If successfully gets the purchase status of IPPV event. Else,
 * @ref getRMFVal( uint32_t response ) function for other approximate return values.
 * @ingroup ippvclientapi
 */
uint32_t IppvClient::getPurchaseStatus( uint32_t eID )
{
	uint32_t response = 0;
	int32_t result_recv;

	if ( !isCANHReady() )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV",
				 "Error!!Canh not started yet." );
		return RMF_IPPVSRC_OTHER_FAILURE;
	}

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) COMMAND_GET_PURCHASE_STATUS );
	ipcBuf.addData( ( const void * ) &eID, sizeof( eID ) );
	if( 0 != canh_server_port_no )
	{
		const char *canh_server_port;
		canh_server_port = rmf_osal_envGet( "CANH_SERVER_PORT_NO" );
		if( NULL == canh_server_port )
		{
		       RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV", "%s: NULL POINTER  received for name string\n", __FUNCTION__ );
			canh_server_port_no = CANH_SERVER_DFLT_PORT_NO;
		}
		else
		{
			canh_server_port_no = atoi( canh_server_port );
		}
	}
	
	int8_t res = ipcBuf.sendCmd( canh_server_port_no );

	if ( 0 == res )
	{
		result_recv = ipcBuf.getResponse( &response );
	}
	else
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV", "CANH Comm failure in %s:%d\n",
				 __FUNCTION__, __LINE__ );
		return RMF_IPPVSRC_OTHER_FAILURE;
	}

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.IPPV", "%s:ippv status is [%s]\n",
			 __FUNCTION__, getStatusByString( getRMFVal( response ) ) );

	return getRMFVal( response );
}


/**
 * @fn uint32_t IppvClient::isIppvEventAuthorized( uint32_t eID,uint8_t * isFreePreviewable,uint8_t * isAuthorized )
 * @brief This function checks whether Ippv event is authorized or not.
 * Function checks whether CANH is ready or not.
 * IPPV IPC client requests data through command to check whether IPPV event is authorized or not.
 *
 * @param[in] eID Entitlement ID to check whether the IPPV event is authorized or not.
 * @param[in] isFreePreviewable Whether IPPV event is free previewable or not.
 * @param[in] isAuthorized Whether IPPV event is authorized or not
 *
 * @return rmfVal
 * @retval RMF_IPPVSRC_NOT_READY  If CANH is not ready.
 * @retval RMF_IPPVSRC_OTHER_FAILURE CANH Common failure.
 * @retval RMF_IPPVSRC_SUCCESS If successfully checks whether IPPV event is authorized or not.
 * @ingroup ippvclientapi
 */
uint32_t IppvClient::isIppvEventAuthorized( uint32_t eID,
											uint8_t * isFreePreviewable,
											uint8_t * isAuthorized )
{
	uint32_t response = 0;
	int32_t result_recv;
	uint16_t sourceID = ( 0xFFFF0000 & eID ) >> 16;

	if ( !isCANHReady() )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV",
				 "Error!!Canh not started yet." );
		return RMF_IPPVSRC_NOT_READY;
	}

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) COMMAND_IS_IPPV_EVENT_AUTHORIZED );
	ipcBuf.addData( ( const void * ) &eID, sizeof( eID ) );
	ipcBuf.addData( ( const void * ) &sourceID, sizeof( sourceID ) );
	if( 0 != canh_server_port_no )
	{
		const char *canh_server_port;
		canh_server_port = rmf_osal_envGet( "CANH_SERVER_PORT_NO" );
		if( NULL == canh_server_port )
		{
		       RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV", "%s: NULL POINTER  received for name string\n", __FUNCTION__ );
			canh_server_port_no = CANH_SERVER_DFLT_PORT_NO;
		}
		else
		{
			canh_server_port_no = atoi( canh_server_port );
		}
	}
	int8_t res = ipcBuf.sendCmd( canh_server_port_no );

	if ( 0 == res )
	{
		result_recv = ipcBuf.getResponse( &response );
		result_recv =
			ipcBuf.collectData( isFreePreviewable, sizeof( uint8_t ) );
		result_recv = ipcBuf.collectData( isAuthorized, sizeof( uint8_t ) );
	}
	else
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV", "CANH Comm failure in %s:%d\n",
				 __FUNCTION__, __LINE__ );
		return RMF_IPPVSRC_OTHER_FAILURE;
	}

	return RMF_IPPVSRC_SUCCESS;
}

uint32_t IppvClient::start(  )
{
	return RMF_IPPVSRC_SUCCESS;
}


uint32_t IppvClient::stop(	)
{	
	return RMF_IPPVSRC_SUCCESS;
	
}


/**
 * @fn uint32_t getRMFVal( uint32_t response )
 * @brief This function gets the RMF value for the corresponding response parameter (which indicates the response from IPC buffer command).
 *
 * @param[in] response To get the RMF value.
 * @return rmfVal
 * @retval 0xFFFFFFFF. Else, refer the ippv macros for other return values.
 * @ref ippvmacros
 * @ingroup ippvclientapi
 */
uint32_t getRMFVal( uint32_t response )
{
	uint32_t rmfVal;

	switch ( response )
	{
	  case IPPV_SUCCESS:
		  rmfVal = RMF_IPPVSRC_SUCCESS;
		  break;

	  case IPPV_PURCHASE_PENDING:
		  rmfVal = RMF_IPPVSRC_PURCHASE_PENDING;
		  break;

	  case IPPV_PURCHASE_NOT_ATTEMPTED:
		  rmfVal = RMF_IPPVSRC_PURCHASE_NOT_ATTEMPTED;
		  break;

	  case IPPV_ALREADY_AUTHORIZED:
		  rmfVal = RMF_IPPVSRC_ALREADY_AUTHORIZED;
		  break;

	  case IPPV_IMPULSE_DISABLED:
		  rmfVal = RMF_IPPVSRC_IMPULSE_DISABLED;
		  break;

	  case IPPV_NOMEM:
		  rmfVal = RMF_IPPVSRC_NOMEM;
		  break;

	  case IPPV_PIN_FAILED:
		  rmfVal = RMF_IPPVSRC_PIN_FAILED;
		  break;

	  case IPPV_CREDIT_FAILURE:
		  rmfVal = RMF_IPPVSRC_CREDIT_FAILURE;
		  break;

	  case IPPV_NO_PERSISTENT_MEMORY:
		  rmfVal = RMF_IPPVSRC_NO_PERSISTENT_MEMORY;
		  break;

	  case IPPV_PURCHASE_UNAVAILABLE:
		  rmfVal = RMF_IPPVSRC_PURCHASE_UNAVAILABLE;
		  break;

	  case IPPV_OUTSIDE_PURCHASE_WINDOW:
		  rmfVal = RMF_IPPVSRC_OUTSIDE_PURCHASE_WINDOW;
		  break;

	  case IPPV_MAX_EVENTS:
		  rmfVal = RMF_IPPVSRC_MAX_EVENTS;
		  break;

	  case IPPV_CANCEL_SUCCESS:
		  rmfVal = RMF_IPPVSRC_CANCEL_SUCCESS;
		  break;

	  case IPPV_CANCEL_FAILED:
		  rmfVal = RMF_IPPVSRC_CANCEL_FAILED;
		  break;

	  case IPPV_CANCEL_BAD_EID:
		  rmfVal = RMF_IPPVSRC_CANCEL_BAD_EID;
		  break;

	  case IPPV_UNCONFIGURED_PROGRAM_INDEX:
		  rmfVal = RMF_IPPVSRC_UNCONFIGURED_PROGRAM_INDEX;
		  break;

	  case IPPV_INVALID_SOURCE_ID:
		  rmfVal = RMF_IPPVSRC_INVALID_SOURCE_ID;
		  break;

	  case IPPV_INVALID_START_TIME:
		  rmfVal = RMF_IPPVSRC_INVALID_START_TIME;
		  break;

	  case IPPV_MAX_SIMULTANEOUS_PURCHASES:
		  rmfVal = RMF_IPPVSRC_MAX_SIMULTANEOUS_PURCHASES;
		  break;

	  case IPPV_PURCHASE_LIMIT_REACHED:
		  rmfVal = RMF_IPPVSRC_PURCHASE_LIMIT_REACHED;
		  break;

	  case IPPV_SYSTEM_TIME_NOT_RXD:
		  rmfVal = RMF_IPPVSRC_SYSTEM_TIME_NOT_RXD;
		  break;

	  case IPPV_ZERO_VALUE_PURCHASE_WINDOW:
		  rmfVal = RMF_IPPVSRC_ZERO_VALUE_PURCHASE_WINDOW;
		  break;

	  case IPPV_OTHER_FAILURE:
		  rmfVal = RMF_IPPVSRC_OTHER_FAILURE;
		  break;

	  case IPPV_NOT_READY:
		  rmfVal = RMF_IPPVSRC_NOT_READY;
		  break;

	  default:
		  rmfVal = 0xFFFFFFFF;
		  break;
	}							//end switch(response)
	return rmfVal;
}


/**
 * @fn const char *IppvClient::getStatusByString( uint32_t response )
 * @brief This function gets the status for the corresponding response parameter (which indicates the response from IPC buffer command).
 *
 * @param[in] response To check the IPPV status.
 * @return pStatus
 * @retval NULL . Else, refer the ippv macros for other return values.
 * @ref ippvmacros
 * @ingroup ippvclientapi
 */
const char *IppvClient::getStatusByString( uint32_t response )
{
	const char *pStatus = NULL;
	switch ( response )
	{
	  case RMF_IPPVSRC_SUCCESS:
	  case IPPV_SUCCESS:
		  pStatus = "IPPV_SUCCESS";
		  break;

	  case RMF_IPPVSRC_PURCHASE_PENDING:
		  pStatus = "IPPV_PURCHASE_PENDING";
		  break;

	  case RMF_IPPVSRC_PURCHASE_NOT_ATTEMPTED:
		  pStatus = "IPPV_PURCHASE_NOT_ATTEMPTED";
		  break;

	  case RMF_IPPVSRC_ALREADY_AUTHORIZED:
		  pStatus = "IPPV_ALREADY_AUTHORIZED";
		  break;

	  case RMF_IPPVSRC_IMPULSE_DISABLED:
		  pStatus = "IPPV_IMPULSE_DISABLED";
		  break;

	  case RMF_IPPVSRC_NOMEM:
		  pStatus = "IPPV_NOMEM";
		  break;

	  case RMF_IPPVSRC_PIN_FAILED:
		  pStatus = "IPPV_PIN_FAILED";
		  break;

	  case RMF_IPPVSRC_CREDIT_FAILURE:
		  pStatus = "IPPV_CREDIT_FAILURE";
		  break;

	  case RMF_IPPVSRC_NO_PERSISTENT_MEMORY:
		  pStatus = "IPPV_NO_PERSISTENT_MEMORY";
		  break;

	  case RMF_IPPVSRC_PURCHASE_UNAVAILABLE:
		  pStatus = "IPPV_PURCHASE_UNAVAILABLE";
		  break;

	  case RMF_IPPVSRC_OUTSIDE_PURCHASE_WINDOW:
		  pStatus = "IPPV_OUTSIDE_PURCHASE_WINDOW";
		  break;

	  case RMF_IPPVSRC_MAX_EVENTS:
		  pStatus = "IPPV_MAX_EVENTS";
		  break;

	  case RMF_IPPVSRC_CANCEL_SUCCESS:
		  pStatus = "IPPV_CANCEL_SUCCESS";
		  break;

	  case RMF_IPPVSRC_CANCEL_FAILED:
		  pStatus = "IPPV_CANCEL_FAILED";
		  break;

	  case RMF_IPPVSRC_CANCEL_BAD_EID:
		  pStatus = "IPPV_CANCEL_BAD_EID";
		  break;

	  case RMF_IPPVSRC_UNCONFIGURED_PROGRAM_INDEX:
		  pStatus = "IPPV_UNCONFIGURED_PROGRAM_INDEX";
		  break;

	  case RMF_IPPVSRC_INVALID_SOURCE_ID:
		  pStatus = "IPPV_INVALID_SOURCE_ID";
		  break;

	  case RMF_IPPVSRC_INVALID_START_TIME:
		  pStatus = "IPPV_INVALID_START_TIME";
		  break;

	  case RMF_IPPVSRC_MAX_SIMULTANEOUS_PURCHASES:
		  pStatus = "IPPV_MAX_SIMULTANEOUS_PURCHASES";
		  break;

	  case RMF_IPPVSRC_PURCHASE_LIMIT_REACHED:
		  pStatus = "IPPV_PURCHASE_LIMIT_REACHED";
		  break;

	  case RMF_IPPVSRC_SYSTEM_TIME_NOT_RXD:
		  pStatus = "IPPV_SYSTEM_TIME_NOT_RXD";
		  break;

	  case RMF_IPPVSRC_ZERO_VALUE_PURCHASE_WINDOW:
		  pStatus = "IPPV_ZERO_VALUE_PURCHASE_WINDOW";
		  break;

	  case RMF_IPPVSRC_OTHER_FAILURE:
		  pStatus = "IPPV_OTHER_FAILURE";
		  break;

	  case RMF_IPPVSRC_NOT_READY:
		  pStatus = "IPPV_NOT_READY";
		  break;

	  default:
		  pStatus = NULL;
		  break;
	}							//end switch(response)
	return pStatus;
}
