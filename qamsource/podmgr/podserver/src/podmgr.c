/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2018 RDK Management
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

#include <podmgr.h>
#include <podimpl.h>
#include <rmf_osal_mem.h>
#include <rmf_osal_sync.h>
#include <rdk_debug.h>

#include <string.h>
#include <podIf.h>

#if USE_SYSRES_MLT
#include "mlt_malloc.h"
#endif

#include "cardUtils.h"
/**
 * Static global variables.
 */

/**
 * This is the actual POD-HOST database used to maintain POD data
 * acquired from the target POD-Host interface.
 */
rmf_PODDatabase podDB;

/**
 * Session ID for the CAS resource
 */
uint32_t casSessionId = 0;
static uint8_t podmgr_initialized = 0;
/**
 * Desired resource version for the CAS resource
 */
uint16_t casResourceVersion = 1;

/**
 * <i>rmf_podmgrInit()</i>
 *
 * Initialize interface to POD-HOST stack.	Depending on the underlying OS-specific
 * support this initialization process will likely populate the internal to podmgr
 * POD database used to cache information communicated between the Host and the
 * POD.  The rmf_PODDatabase structure is used to maintain the POD data.  The
 * actual mechanics of how the database is maintained is an implementation-specific
 * issues that is dependent upon the functionality of the target POD-Host interface.
 */
void podmgrInit( void )
{
	rmf_Error ec;

	/* If the POD has not been initialized, do so. */
	static rmf_osal_Bool inited = false;

	if ( !inited )
	{

		ec = create_pod_event_manager(	);
		if ( RMF_SUCCESS != ec )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					 "<<POD>> rmf_podmgrInit, create_pod_event_manager failed=%d\n",
					 ec );
		}
		/* At init time the POD has not been inserted and is not ready */
		podDB.pod_isPresent = FALSE;
		podDB.pod_isReady = FALSE;

		inited = true;
		/* initialize the decryption capabilities to the default (no POD ready) value of 0 */
		podDB.pod_maxElemStreams = 0;
		podDB.pod_maxPrograms = 0;
		podDB.pod_maxTransportStreams = 0;

		/* Initialize variable size parameter fields to NULL. */
		podDB.pod_featureParams.pc_pin = NULL;
		podDB.pod_featureParams.pc_setting = NULL;
		podDB.pod_featureParams.ippv_pin = NULL;
		podDB.pod_featureParams.cable_urls = NULL;
		podDB.pod_featureParams.term_assoc = NULL;
		podDB.pod_featureParams.zip_code = NULL;

		/* Initialize the rmf_PodmgrFeatures member struct pointer */
		podDB.pod_features = NULL;
		podDB.pod_appInfo = NULL;
		/* Allocate the POD DB Mutex */
		ec = rmf_osal_mutexNew( &podDB.pod_mutex );

		/* If we failed to create the mutex, set the POD Database struct as NOT READY */
		if ( ec != RMF_SUCCESS )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					 "<<POD>> rmf_podmgrInit, failed to get mutex, error=%d\n",
					 ec );
		}

		( void ) podInit( &podDB );

		( void ) podImplInit(  );
		podmgr_initialized  = 1;
	}
}


/**
 * <i>rmf_podmgrIsReady()</i>
 *
 * Determine availability of POD device.
 *
 * @return RMF_SUCCESS if the POD is available.
 */
rmf_Error podmgrIsReady( void )
{
	rmf_osal_Bool ready;
	if( 1 == podmgr_initialized  )
	{
		rmf_osal_mutexAcquire( podDB.pod_mutex );
		ready = podDB.pod_isReady;
		rmf_osal_mutexRelease( podDB.pod_mutex );
	}
	else
	{
		ready = FALSE;
	}
	/* Return state of POD based on initialization phase. */
	/* To sync with log of old implementation  */
	if ( FALSE == ready )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "CableCard device not available\n" );
	}
	return ( ready == TRUE ? RMF_SUCCESS : RMF_OSAL_ENODATA );

}

/**
 * <i>podGetManfacturerId</i>
 *
 * Get manufacturer Id from POD database	
 *
 * @param pManufacturerId is a pointer for returning manufacturer Id
 *
 * @return RMF_SUCCESS if the features list was successfully acquired.
 */
rmf_Error podmgrGetManufacturerId( uint16_t *pManufacturerId )
{
	return getCardMfgId( pManufacturerId  );
}

/**
 * <i>podGetCardVersion</i>
 *
 * Get manufacturer Id from POD database	
 *
 * @param podGetCardVersion is a pointer for returning version 
 * of the cable card
 *
 * @return RMF_SUCCESS if the features list was successfully acquired.
 */
rmf_Error podmgrGetCardVersion( uint16_t *pVersionNo )
{
	return getCardVersionNo( pVersionNo );
}

/**
 * <i>rmf_podmgrGetFeatures</i>
 *
 * Get the POD's generic features list.  Note the generic features list
 * can be updated by the HOST-POD interface asynchronously and since there
 * is no mechanism in the native POD support APIs to get notification of this
 * asynchronous update, the list must be refreshed from the interface every
 * the list is requested. The last list acquired will be buffered within the
 * POD database, but it may not be up to date.	
 *
 * @param features is a pointer for returning a pointer to the current features
 *				   list.
 *
 * @return RMF_SUCCESS if the features list was successfully acquired.
 */
rmf_Error rmf_podmgrGetFeatures( rmf_PodmgrFeatures ** features )
{
	rmf_Error ec;

	/* Make sure caller parameter is valid. */
	if ( NULL == features )
		return RMF_OSAL_EINVAL;

	/* Call to acquire feature list from POD-Host interface. */
	if ( ( ec = podGetFeatures( &podDB ) ) != RMF_SUCCESS )
	{
		return ec;
	}

	/* Validate POD features were available. */
	if ( ( TRUE == podDB.pod_isReady ) && ( NULL != podDB.pod_features ) )
	{
		/* Return POD database feature list reference. */
		*features = podDB.pod_features;
		return RMF_SUCCESS;
	}

	return RMF_OSAL_ENODATA;
}

/**
 * <i>rmf_podmgrGetFeatureParam<i/>
 *
 * Get the specified feature parameter.
 *
 * @param featureId is the Generic Feature identifier of the feature to retrieve.
 * @param param is a pointer for returning the pointer to the parameter value.
 * @param length is a pointer for returning the length of the associated parameter value.
 *
 * @return RMF_SUCCESS if the feature parameter was successfully retrieved.
 */
rmf_Error rmf_podmgrGetFeatureParam( uint32_t featureId, void *param,
									 uint32_t * length )
{
	rmf_Error ec;				/* Error condition code. */
	uint8_t **real_param = ( uint8_t ** ) param;

	if ( ( NULL == param ) || ( NULL == length ) )
		return RMF_OSAL_EINVAL;

	/* Make sure feature parameter is cached in the POD database. */
	if ( ( ec = podGetFeatureParam( &podDB, featureId ) ) != RMF_SUCCESS )
	{
		return ec;
	}

	/* Return requested generic feature parameter byte stream. */
	switch ( featureId )
	{
	  case RMF_PODMGR_GF_RF_OUTPUT_CHANNEL:
		  /* Return address of RF output channel info. */
		  *real_param = &( podDB.pod_featureParams.rf_output_channel[0] );
		  *length = sizeof( uint8_t ) * 2;
		  break;

	  case RMF_PODMGR_GF_P_C_PIN:
		  /* Return a pointer to the pin number list. */
		  *real_param = podDB.pod_featureParams.pc_pin;
		  *length =
			  sizeof( uint8_t ) * ( *podDB.pod_featureParams.pc_pin + 1 );
		  break;

	  case RMF_PODMGR_GF_RESET_P_C_PIN:
		  /* Return a pointer to the pin reset value. */
		  *real_param = &podDB.pod_featureParams.reset_pin_ctrl;
		  *length = sizeof( uint8_t );
		  break;

	  case RMF_PODMGR_GF_P_C_SETTINGS:
		  /* Return a pointer to the parental control settings list. */
		  *real_param = podDB.pod_featureParams.pc_setting;
		  {						/* Get channel count. */
			  uint16_t channel_count = 0;

			  channel_count +=
				  *( podDB.pod_featureParams.pc_setting + 1 ) << 8;
			  channel_count += *( podDB.pod_featureParams.pc_setting + 2 );

			  /* Calculate length: sizeof(channel_count) + (count * 24-bits). */
			  *length =
				  sizeof( uint8_t ) + sizeof( uint16_t ) +
				  ( channel_count * ( sizeof( uint8_t ) * 3 ) );
		  }
		  break;

	  case RMF_PODMGR_GF_IPPV_PIN:
		  *real_param = podDB.pod_featureParams.ippv_pin;
		  *length =
			  sizeof( uint8_t ) * ( *podDB.pod_featureParams.ippv_pin + 1 );
		  break;

	  case RMF_PODMGR_GF_TIME_ZONE:
		  /* Return address of time zone. */
		  *real_param = &( podDB.pod_featureParams.time_zone_offset[0] );
		  *length = sizeof( uint8_t ) * 2;
		  break;

	  case RMF_PODMGR_GF_DAYLIGHT_SAVINGS:
		  /* Return address of dalylight savings control. */
		  *real_param = &( podDB.pod_featureParams.daylight_savings[0] );
		  *length = sizeof( uint8_t ) * 10;
		  break;

	  case RMF_PODMGR_GF_AC_OUTLET:
		  /* Return address of AC outlet setting. */
		  *real_param = &( podDB.pod_featureParams.ac_outlet_ctrl );
		  *length = sizeof( uint8_t );
		  break;

	  case RMF_PODMGR_GF_LANGUAGE:
		  /* Return a pointer to the language encoding. */
		  *real_param = &( podDB.pod_featureParams.language_ctrl[0] );
		  *length = sizeof( uint8_t ) * 3;
		  break;

	  case RMF_PODMGR_GF_RATING_REGION:
		  /* Return address of ratings region. */
		  *real_param = &( podDB.pod_featureParams.ratings_region );
		  *length = sizeof( uint8_t );
		  break;

	  case RMF_PODMGR_GF_CABLE_URLS:
		  /* Return address of cable URLs. */
		  *real_param = podDB.pod_featureParams.cable_urls;
		  *length = podDB.pod_featureParams.cable_urls_length;	/* Get length. */
		  break;

	  case RMF_PODMGR_GF_EA_LOCATION:
		  /* Return address of emergency alert location. */
		  *real_param = &( podDB.pod_featureParams.ea_location[0] );
		  *length = sizeof( uint8_t ) * 3;
		  break;

	  case RMF_PODMGR_GF_VCT_ID:
		  /* Return address of VCT ID. */
		  *real_param = &( podDB.pod_featureParams.vct_id[0] );
		  *length = sizeof( uint8_t ) * 2;
		  break;

	  case RMF_PODMGR_GF_TURN_ON_CHANNEL:
		  /* Return address of turn-on channel. */
		  *real_param = &( podDB.pod_featureParams.turn_on_channel[0] );
		  *length = sizeof( uint8_t ) * 2;
		  break;

	  case RMF_PODMGR_GF_TERM_ASSOC:
		  /* Return address of terminal association ID. */
		  *real_param = podDB.pod_featureParams.term_assoc;
		  *length =
			  ( podDB.pod_featureParams.term_assoc[0] << 8 ) +
			  ( podDB.pod_featureParams.term_assoc[1] ) +
			  ( sizeof( uint8_t ) * 2 );
		  break;

	  case RMF_PODMGR_GF_DOWNLOAD_GRP_ID:
		  /* Return address of common download group ID. */
		  *real_param = &( podDB.pod_featureParams.cdl_group_id[0] );
		  *length = sizeof( uint8_t ) * 2;
		  break;

	  case RMF_PODMGR_GF_ZIP_CODE:
		  /* Return address of zip code. */
		  *real_param = podDB.pod_featureParams.zip_code;
		  *length =
			  ( podDB.pod_featureParams.zip_code[0] << 8 ) +
			  ( podDB.pod_featureParams.zip_code[1] ) +
			  ( sizeof( uint8_t ) * 2 );
		  break;

	  default:
		  /* Return invalid parameter error. */
		  *real_param = NULL;
		  *length = 0;
		  return RMF_OSAL_EINVAL;
	}

	return RMF_SUCCESS;
}

/**
 * <i>rmf_podmgrSetFeatureParam<i/>
 *
 * Set the specified POD-Host feature parameter value.	The feature change
 * request will be passed on to the POD-Host interface and based on the
 * resulting acceptance or denial the internal POD infromation database
 * will be updated (or not) to reflect the new feature parameter value.
 *
 * @param featureId is the identifier of the target feature.
 * @param param is a pointer to the byte array containing the new value.
 *
 * @return RMF_SUCCESS if the prosed value was accepted by the POD.
 */
rmf_Error rmf_podmgrSetFeatureParam( uint32_t featureId, void *param,
									 uint32_t length )
{
	rmf_Error ec;				/* Error condition code. */
	uint32_t size;				/* size used for memory allocations. */
	uint32_t i;					/* loop index. */
	uint8_t *real_param = ( uint8_t * ) param;

	if ( NULL == param )
		return RMF_OSAL_EINVAL;

	switch ( featureId )
	{
	  case RMF_PODMGR_GF_RF_OUTPUT_CHANNEL:

		  if ( length != sizeof( uint8_t ) * 2 )
		  {
			  return RMF_OSAL_EINVAL;
		  }
		  /* Call POD to perform & verify set operation. */
		  if ( ( ec =
				 vlMpeosPodSetFeatureParam( featureId, real_param,
											sizeof( uint8_t ) * 2 ) ) !=
			   RMF_SUCCESS )
		  {
			  return ec;		/* Set rejected. */
		  }

		  /* Value accepted by POD so, copy RF channel setting to database. */
		  rmf_osal_mutexAcquire( podDB.pod_mutex );
		  memcpy( podDB.pod_featureParams.rf_output_channel, real_param,
				  sizeof( uint8_t ) * 2 );
		  rmf_osal_mutexRelease( podDB.pod_mutex );
		  break;

	  case RMF_PODMGR_GF_P_C_PIN:
		  size = real_param[0] + 1;		/* Get size of PIN parameter data. */
		  if ( length != size )
		  {
			  return RMF_OSAL_EINVAL;
		  }

		  /* Call POD to perform & verify set operation. */
		  if ( ( ec =
				 vlMpeosPodSetFeatureParam( featureId, real_param,
											size ) ) != RMF_SUCCESS )
		  {
			  return ec;
		  }

		  /* Need to allocate PIN array. */
		  rmf_osal_mutexAcquire( podDB.pod_mutex );
		  if ( ( NULL == podDB.pod_featureParams.pc_pin )
			   || ( podDB.pod_featureParams.pc_pin[0] != real_param[0] ) )
		  {
			  /* Release memory for previous PIN array. */
			  if ( NULL != podDB.pod_featureParams.pc_pin )
			  {
				  /* Free space for previous PIN array. */
				  rmf_osal_memFreeP( RMF_OSAL_MEM_POD,
									 ( void * ) podDB.pod_featureParams.
									 pc_pin );
			  }
			  /* Allocate new PIN array. */
			  if ( ( ec =
					 rmf_osal_memAllocP( RMF_OSAL_MEM_POD, size,
										 ( void ** ) &podDB.pod_featureParams.
										 pc_pin ) ) != RMF_SUCCESS )
			  {
				  rmf_osal_mutexRelease( podDB.pod_mutex );
				  return ec;
			  }
		  }
		  /* Copy new PIN length and setting. */
		  memcpy( podDB.pod_featureParams.pc_pin, real_param, size );
		  rmf_osal_mutexRelease( podDB.pod_mutex );
		  break;

	  case RMF_PODMGR_GF_P_C_SETTINGS:
		  {
			  uint16_t channel_count = 0;

			  channel_count += *( real_param + 1 ) << 8;
			  channel_count += *( real_param + 2 );

			  /* Determine size of space for new setting. */
			  size =
				  sizeof( uint8_t ) + sizeof( uint16_t ) +
				  ( channel_count * ( sizeof( uint8_t ) * 3 ) );

			  if ( length != size )
			  {
				  return RMF_OSAL_EINVAL;
			  }

			  /* Call POD to perform & verify set operation. */
			  if ( ( ec =
					 vlMpeosPodSetFeatureParam( featureId, real_param,
												size ) ) != RMF_SUCCESS )
			  {
				  return ec;	/* Set rejected. */
			  }

			  /* Determine need to allocate parental control settings array. */
			  rmf_osal_mutexAcquire( podDB.pod_mutex );
			  if ( ( NULL == podDB.pod_featureParams.pc_setting ) || ( podDB.pod_featureParams.pc_setting[1] != real_param[1] ) || ( podDB.pod_featureParams.pc_setting[2] != real_param[2] ) ) /* Different size? */
			  {
				  /* Release memory for previous channel setting array. */
				  if ( NULL != podDB.pod_featureParams.pc_setting )
				  {
					  /* Free space for previous settings. */
					  rmf_osal_memFreeP( RMF_OSAL_MEM_POD,
										 ( void * ) podDB.pod_featureParams.
										 pc_setting );
				  }
				  /* Allocate new array: reset_flag(8-bits), count(16-bits), count * 24-bits */
				  if ( ( ec =
						 rmf_osal_memAllocP( RMF_OSAL_MEM_POD, size,
											 ( void ** ) &podDB.
											 pod_featureParams.
											 pc_setting ) ) != RMF_SUCCESS )
				  {
					  rmf_osal_mutexRelease( podDB.pod_mutex );
					  return ec;
				  }
			  }
			  /* Copy new PC settings. */
			  memcpy( podDB.pod_featureParams.pc_setting, real_param, size );
			  rmf_osal_mutexRelease( podDB.pod_mutex );
			  break;
		  }
	  case RMF_PODMGR_GF_IPPV_PIN:
		  /* Determine size of space for new IPPV PIN. */
		  size = ( real_param[0] + 1 );
		  if ( length != size )
		  {
			  return RMF_OSAL_EINVAL;
		  }

		  /* Call POD to perform & verify set operation. */
		  if ( ( ec =
				 vlMpeosPodSetFeatureParam( featureId, real_param,
											size ) ) != RMF_SUCCESS )
		  {
			  return ec;		/* Set rejected. */
		  }

		  /* Need to allocate PIN array. */
		  rmf_osal_mutexAcquire( podDB.pod_mutex );
		  if ( ( NULL == podDB.pod_featureParams.ippv_pin )
			   || ( podDB.pod_featureParams.ippv_pin[0] != real_param[0] ) )
		  {
			  /* Release memory for previous PIN array. */
			  if ( NULL != podDB.pod_featureParams.ippv_pin )
			  {
				  /* Free previous IPPV PIN. */
				  rmf_osal_memFreeP( RMF_OSAL_MEM_POD,
									 ( void * ) podDB.pod_featureParams.
									 ippv_pin );
			  }
			  /* Allocate new PIN array. */
			  if ( ( ec =
					 rmf_osal_memAllocP( RMF_OSAL_MEM_POD, size,
										 ( void ** ) &podDB.pod_featureParams.
										 ippv_pin ) ) != RMF_SUCCESS )
			  {
				  rmf_osal_mutexRelease( podDB.pod_mutex );
				  return ec;
			  }
		  }
		  /* Copy new PIN length and setting. */
		  memcpy( podDB.pod_featureParams.ippv_pin, real_param, size );
		  rmf_osal_mutexRelease( podDB.pod_mutex );
		  break;

	  case RMF_PODMGR_GF_TIME_ZONE:
		  if ( length != sizeof( uint8_t ) * 2 )
		  {
			  return RMF_OSAL_EINVAL;
		  }

		  /* Call POD to perform & verify set operation. */
		  if ( ( ec =
				 vlMpeosPodSetFeatureParam( featureId, real_param,
											sizeof( uint8_t ) * 2 ) ) !=
			   RMF_SUCCESS )
		  {
			  return ec;		/* Set rejected. */
		  }

		  /* Copy time zone to database. */
		  rmf_osal_mutexAcquire( podDB.pod_mutex );
		  podDB.pod_featureParams.time_zone_offset[0] = real_param[0];
		  podDB.pod_featureParams.time_zone_offset[1] = real_param[1];
		  rmf_osal_mutexRelease( podDB.pod_mutex );
		  break;

	  case RMF_PODMGR_GF_DAYLIGHT_SAVINGS:
		  size = 10;
		  if ( length != ( 10 * sizeof( uint8_t ) )
			   && length != ( 1 * sizeof( uint8_t ) ) )
		  {
			  return RMF_OSAL_EINVAL;
		  }

		  /* Call POD to perform & verify set operation. */
		  if ( ( ec =
				 vlMpeosPodSetFeatureParam( featureId, real_param,
											length ) ) != RMF_SUCCESS )
		  {
			  return ec;		/* Set rejected. */
		  }

		  /* Copy new daylight savings setting. */
		  rmf_osal_mutexAcquire( podDB.pod_mutex );
		  memcpy( podDB.pod_featureParams.daylight_savings, real_param,
				  size );
		  rmf_osal_mutexRelease( podDB.pod_mutex );
		  break;

	  case RMF_PODMGR_GF_AC_OUTLET:
		  if ( length != sizeof( uint8_t ) )
		  {
			  return RMF_OSAL_EINVAL;
		  }
		  /* Call POD to perform & verify set operation. */
		  if ( ( ec =
				 vlMpeosPodSetFeatureParam( featureId, real_param,
											sizeof( uint8_t ) ) ) !=
			   RMF_SUCCESS )
		  {
			  return ec;		/* Set rejected. */
		  }

		  /* Copy AC Outlet setting. */
		  rmf_osal_mutexAcquire( podDB.pod_mutex );
		  podDB.pod_featureParams.ac_outlet_ctrl = *real_param;
		  rmf_osal_mutexRelease( podDB.pod_mutex );
		  break;

	  case RMF_PODMGR_GF_LANGUAGE:
		  if ( length != sizeof( uint8_t ) * 3 )
		  {
			  return RMF_OSAL_EINVAL;
		  }
		  /* Call POD to perform & verify set operation. */
		  if ( ( ec =
				 vlMpeosPodSetFeatureParam( featureId, real_param,
											sizeof( uint8_t ) * 3 ) ) !=
			   RMF_SUCCESS )
		  {
			  return ec;		/* Set rejected. */
		  }

		  /* Copy the new language setting. */
		  rmf_osal_mutexAcquire( podDB.pod_mutex );
		  for ( i = 0; i < 3; ++i )
		  {
			  podDB.pod_featureParams.language_ctrl[i] = *real_param++;
		  }
		  rmf_osal_mutexRelease( podDB.pod_mutex );
		  break;

	  case RMF_PODMGR_GF_RATING_REGION:
		  if ( length != sizeof( uint8_t ) )
		  {
			  return RMF_OSAL_EINVAL;
		  }
		  /* Call POD to perform & verify set operation. */
		  if ( ( ec =
				 vlMpeosPodSetFeatureParam( featureId, real_param,
											sizeof( uint8_t ) ) ) !=
			   RMF_SUCCESS )
		  {
			  return ec;		/* Set rejected. */
		  }

		  /* Copy new ratings region. */
		  rmf_osal_mutexAcquire( podDB.pod_mutex );
		  podDB.pod_featureParams.ratings_region = *real_param;
		  rmf_osal_mutexRelease( podDB.pod_mutex );
		  break;

	  default:
		  /* Return invalid parameter error. */
		  return RMF_OSAL_EINVAL;
	}

	return RMF_SUCCESS;
}

/**
 * The rmf_podmgrAIConnect() function shall establish a connection with the
 * Application Information resource on the POD device.
 *
 * @param sessionId the session identifier of the target application information session.
 *
 * @return Upon successful completion, this function shall return RMF_SUCCESS. Otherwise,
 *			one of the errors below is returned.
 * <ul>
 * <li>		RMF_OSAL_ENOMEM - There was insufficient memory available to complete the operation.
 * <li>		RMF_OSAL_EINVAL - One or more parameters were out of range or unusable.
 * </ul>
 */
rmf_Error rmf_podmgrAIConnect( uint32_t * sessionId, uint16_t * version )
{

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
			 "%s: AI session successfully opened. ID = %p!\n", __FUNCTION__,
			 sessionId );

	return RMF_SUCCESS;
}
