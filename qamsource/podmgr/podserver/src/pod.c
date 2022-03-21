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

/* Header Files */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <rmf_osal_types.h>
#include <podimpl.h>
#include <podmgr.h>
#include <rdk_debug.h>
#include <rmf_osal_mem.h>
#include <rmf_osal_socket.h>
#include "rmf_osal_sync.h"
#include "rmf_osal_thread.h"

#include <podIf.h>
#include <cardManagerIf.h>
//#include <glib.h>
#include <stdint.h>
#include <netdb.h>

#if USE_SYSRES_MLT
#include "mlt_malloc.h"
#endif

#define APDU_TAG_LENGTH 3
#define POD_APDU_THREAD "PodMgrAPDUThread"

/*
 * podDB_plat is a pointer used by the RI layer platform-independent POD information
 * database to cache the POD-Host information.
*/
rmf_PODDatabase *podDB_plat;

static rmf_osal_ThreadId cablecard_apdu_threadId = 0;
static int POD_enabled = 1;
// Only need a single event queue
rmf_osal_eventqueue_handle_t gPodEventQueue;
void *gEventQueueACT = NULL;

// Global cache of CableCARD streaming capabilities
uint32_t gMaxCardElemStreams = 10;
uint32_t gMaxCardTransStreams = 2;
uint32_t gMaxCardPrograms = 3;
rmf_Error ( * fnPtrpodProcessApduInt ) ( uint32_t sessionId,  uint8_t * apdu, int32_t len) = NULL;
/**
 * Defines a doubly linked list node that holds APDU data
 */
typedef struct os_apdu_node
{
	uint32_t sessionID;
	uint8_t *data;
	int32_t length;
	struct os_apdu_node *next;

} os_apdu_node_t;



os_apdu_node_t *gAPDUQueue = NULL;
os_apdu_node_t *gAPDUQueueEnd = NULL;
rmf_osal_Mutex gAPDUMutex;
rmf_osal_Cond gAPDUReadyCond;

static int FAKE_SESSION_ID = 0xa1b1c1d1;
static rmf_osal_Bool exitWorkerThread = false;
static rmf_osal_Socket podSocket;

static os_apdu_node_t *allocateAPDU( uint32_t sessionID, uint8_t * data,
									 int32_t length );
static void addAPDU( os_apdu_node_t * apdu );
static void cablecard_apdu_thread( void *data );


// Allocates memory for a new APDU node and initializes it with the
// given data.	Returns NULL if the allocation fails
static os_apdu_node_t *allocateAPDU( uint32_t sessionID, uint8_t * data,
									 int32_t length )
{
	os_apdu_node_t *node;

	if ( rmf_osal_memAllocP
		 ( RMF_OSAL_MEM_POD, sizeof( os_apdu_node_t ),
		   ( void ** ) &node ) != RMF_SUCCESS )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "%s: Could not allocate APDU node memory!\n", __FUNCTION__ );
		return NULL;
	}

	node->sessionID = sessionID;
	node->data = data;
	node->length = length;
	node->next = NULL;

	return node;
}

// Add the given APDU to the end of the queue
static void addAPDU( os_apdu_node_t * apdu )
{
	// Just ignore NULL nodes.	This can happen if the allocation fails
	// for any reason
	if ( apdu == NULL )
		return;

	( void ) rmf_osal_mutexAcquire( gAPDUMutex );

	// If the queue is empty, add the node and set the condition var
	if ( gAPDUQueue == NULL )
	{
		gAPDUQueue = gAPDUQueueEnd = apdu;
		rmf_osal_condSet( gAPDUReadyCond );
	}
	else
	{
		// Add to the end
		gAPDUQueueEnd->next = apdu;
		gAPDUQueueEnd = apdu;
	}

	( void ) rmf_osal_mutexRelease( gAPDUMutex );
}

// Removes the next APDU node from the front of the queue and returns it
// to the caller. Returns NULL if the queue is empty
static os_apdu_node_t *removeAPDU(	)
{
	os_apdu_node_t *node;

	( void ) rmf_osal_mutexAcquire( gAPDUMutex );

	// Empty queue
	if ( gAPDUQueue == NULL )
	{
		( void ) rmf_osal_mutexRelease( gAPDUMutex );
		return NULL;
	}

	// Always return the front node from the queue
	node = gAPDUQueue;

	// Last one in the queue?
	if ( gAPDUQueue == gAPDUQueueEnd )
	{
		// Empty the queue and unset our condition var
		gAPDUQueue = gAPDUQueueEnd = NULL;
		rmf_osal_condUnset( gAPDUReadyCond );
	}
	else
	{
		// Just remove the front node
		gAPDUQueue = gAPDUQueue->next;
	}

	( void ) rmf_osal_mutexRelease( gAPDUMutex );

	return node;
}

/**
 * Updates the given feature in the rmf_PodmgrFeatureParams with the provided data.
 */
static rmf_Error updateGenericFeature( rmf_PodmgrFeatureParams * params,
									   rmf_pod_cablecard_generic_feature
									   featureID, uint8_t * data,
									   uint8_t length )
{
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
			 "%s: featureID = %d, data = %p, length = %d\n", __FUNCTION__,
			 featureID, data, length );

	switch ( featureID )
	{
	  case RMF_PODMGR_GF_RF_OUTPUT_CHANNEL:
		  if ( length != 2 )
		  {
			  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					   "%s: Invalid RF_OUTPUT_CHANNEL length - %d!\n",
					   __FUNCTION__, length );
			  return RMF_OSAL_EINVAL;
		  }
		  memcpy( ( void * ) params->rf_output_channel, ( void * ) data, 2 );
		  break;

	  case RMF_PODMGR_GF_P_C_PIN:
		  // Release old memory
		  if ( params->pc_pin != NULL )
		  {
			  rmf_osal_memFreeP( RMF_OSAL_MEM_POD, params->pc_pin );
		  }
		  if ( rmf_osal_memAllocP
			   ( RMF_OSAL_MEM_POD, length,
				 ( void ** ) &params->pc_pin ) != RMF_SUCCESS )
		  {
			  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					   "%s: Error allocating pc_pin memory!\n",
					   __FUNCTION__ );
			  return RMF_OSAL_EINVAL;
		  }
		  memcpy( ( void * ) params->pc_pin, ( void * ) data, length );
		  break;

	  case RMF_PODMGR_GF_P_C_SETTINGS:
		  // Release old memory
		  if ( params->pc_setting != NULL )
		  {
			  rmf_osal_memFreeP( RMF_OSAL_MEM_POD, params->pc_setting );
		  }
		  if ( rmf_osal_memAllocP
			   ( RMF_OSAL_MEM_POD, length,
				 ( void ** ) &params->pc_setting ) != RMF_SUCCESS )
		  {
			  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					   "%s: Error allocating pc_setting memory!\n",
					   __FUNCTION__ );
			  return RMF_OSAL_EINVAL;
		  }
		  memcpy( ( void * ) params->pc_setting, ( void * ) data, length );
		  break;

	  case RMF_PODMGR_GF_IPPV_PIN:
		  // Release old memory
		  if ( params->ippv_pin != NULL )
		  {
			  rmf_osal_memFreeP( RMF_OSAL_MEM_POD, params->ippv_pin );
		  }
		  if ( rmf_osal_memAllocP
			   ( RMF_OSAL_MEM_POD, length,
				 ( void ** ) &params->ippv_pin ) != RMF_SUCCESS )
		  {
			  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					   "%s: Error allocating ippv_pin memory!\n",
					   __FUNCTION__ );
			  return RMF_OSAL_EINVAL;
		  }
		  memcpy( ( void * ) params->ippv_pin, ( void * ) data, length );
		  break;

	  case RMF_PODMGR_GF_TIME_ZONE:
		  if ( length != 2 )
		  {
			  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					   "%s: Invalid TIME_ZONE length - %d!\n", __FUNCTION__,
					   length );
			  return RMF_OSAL_EINVAL;
		  }
		  memcpy( ( void * ) params->time_zone_offset, ( void * ) data, 2 );
		  break;

	  case RMF_PODMGR_GF_DAYLIGHT_SAVINGS:
		  if ( length != 10 && length != 1 )
		  {
			  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					   "%s: Invalid DAYLIGHT_SAVINGS length - %d!\n",
					   __FUNCTION__, length );
			  return RMF_OSAL_EINVAL;
		  }
		  memset( ( void * ) &params->daylight_savings, 0, 10 );
		  rmf_osal_memcpy( ( void * ) &params->daylight_savings, ( void * ) data,
				  length, sizeof(params->daylight_savings), length );

		  break;

	  case RMF_PODMGR_GF_AC_OUTLET:
		  if ( length != 1 )
		  {
			  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					   "%s: Invalid AC_OUTLET length - %d!\n", __FUNCTION__,
					   length );
			  return RMF_OSAL_EINVAL;
		  }
		  memcpy( ( void * ) &params->ac_outlet_ctrl, ( void * ) data, 1 );
		  break;

	  case RMF_PODMGR_GF_LANGUAGE:
		  if ( length != 3 )
		  {
			  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					   "%s: Invalid LANGUAGE length - %d!\n", __FUNCTION__,
					   length );
			  return RMF_OSAL_EINVAL;
		  }
		  memcpy( ( void * ) params->language_ctrl, ( void * ) data, 3 );
		  break;

	  case RMF_PODMGR_GF_RATING_REGION:
		  if ( length != 1 )
		  {
			  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					   "%s: Invalid RATING_REGION length - %d!\n",
					   __FUNCTION__, length );
			  return RMF_OSAL_EINVAL;
		  }
		  memcpy( ( void * ) &params->ratings_region, ( void * ) data, 1 );
		  break;

	  case RMF_PODMGR_GF_RESET_P_C_PIN:
		  if ( length != 1 )
		  {
			  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					   "%s: Invalid RESET_PIN length - %d!\n", __FUNCTION__,
					   length );
			  return RMF_OSAL_EINVAL;
		  }
		  memcpy( ( void * ) &params->reset_pin_ctrl, ( void * ) data, 1 );
		  break;

	  case RMF_PODMGR_GF_CABLE_URLS:
		  // Release old memory
		  if ( params->cable_urls != NULL )
		  {
			  rmf_osal_memFreeP( RMF_OSAL_MEM_POD, params->cable_urls );
		  }
		  if ( rmf_osal_memAllocP
			   ( RMF_OSAL_MEM_POD, length,
				 ( void ** ) &params->cable_urls ) != RMF_SUCCESS )
		  {
			  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					   "%s: Error allocating cable_urls memory!\n",
					   __FUNCTION__ );
			  return RMF_OSAL_EINVAL;
		  }
		  memcpy( ( void * ) params->cable_urls, ( void * ) data, length );
		  params->cable_urls_length = length;
		  break;

	  case RMF_PODMGR_GF_EA_LOCATION:
		  if ( length != 3 )
		  {
			  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					   "%s: Invalid EA_LOCATION length - %d!\n", __FUNCTION__,
					   length );
			  return RMF_OSAL_EINVAL;
		  }
		  memcpy( ( void * ) params->ea_location, ( void * ) data, 3 );
		  break;

	  case RMF_PODMGR_GF_VCT_ID:
		  if ( length != 2 )
		  {
			  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					   "%s: Invalid VCT_ID length - %d!\n", __FUNCTION__,
					   length );
			  return RMF_OSAL_EINVAL;
		  }
		  memcpy( ( void * ) params->vct_id, ( void * ) data, 2 );
		  break;

	  case RMF_PODMGR_GF_TURN_ON_CHANNEL:
		  if ( length != 2 )
		  {
			  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					   "%s: Invalid TURN_ON_CHANNEL length - %d!\n",
					   __FUNCTION__, length );
			  return RMF_OSAL_EINVAL;
		  }
		  memcpy( ( void * ) params->turn_on_channel, ( void * ) data, 2 );
		  break;

	  case RMF_PODMGR_GF_TERM_ASSOC:
		  // Release old memory
		  if ( params->term_assoc != NULL )
		  {
			  rmf_osal_memFreeP( RMF_OSAL_MEM_POD, params->term_assoc );
		  }
		  if ( rmf_osal_memAllocP
			   ( RMF_OSAL_MEM_POD, length,
				 ( void ** ) &params->term_assoc ) != RMF_SUCCESS )
		  {
			  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					   "%s: Error allocating term_assoc memory!\n",
					   __FUNCTION__ );
			  return RMF_OSAL_EINVAL;
		  }
		  memcpy( ( void * ) params->term_assoc, ( void * ) data, length );
		  break;

	  case RMF_PODMGR_GF_DOWNLOAD_GRP_ID:
		  if ( length != 2 )
		  {
			  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					   "%s: Invalid CDL_GROUP_ID length - %d!\n",
					   __FUNCTION__, length );
			  return RMF_OSAL_EINVAL;
		  }
		  memcpy( ( void * ) params->cdl_group_id, ( void * ) data, 2 );
		  break;

	  case RMF_PODMGR_GF_ZIP_CODE:
		  // Release old memory
		  if ( params->zip_code != NULL )
		  {
			  rmf_osal_memFreeP( RMF_OSAL_MEM_POD, params->zip_code );
		  }
		  if ( rmf_osal_memAllocP
			   ( RMF_OSAL_MEM_POD, length,
				 ( void ** ) &params->zip_code ) != RMF_SUCCESS )
		  {
			  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					   "%s: Error allocating zip_code memory!\n",
					   __FUNCTION__ );
			  return RMF_OSAL_EINVAL;
		  }
		  memcpy( ( void * ) params->zip_code, ( void * ) data, length );
		  break;

	  default:
		  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s: Invalid feature ID!\n",
				   __FUNCTION__ );
		  return RMF_OSAL_EINVAL;
	}

	return RMF_SUCCESS;
}

/**
 * Update the basic card information.  We only update this information when
 * the card has been newly inserted and becomes ready.	Assumes that the
 * podDB mutex is held by the caller.
 */
static rmf_Error updatePODDatabase( rmf_PODDatabase * podDB )
{
	rmf_Error err = RMF_SUCCESS;
	uint8_t i;

	cablecard_info_t *cc_info;
	podmgr_AppInfo *podAppInfo;

	rmf_pod_cablecard_generic_feature *cc_feature_list;
	uint8_t cc_feature_list_length;
	rmf_PodmgrFeatures *podFeatureList;

	/** Generic Features **/

	// Retrieve card generic feature list
	if ( cablecard_get_supported_features
		 ( &cc_feature_list, &cc_feature_list_length ) != RMF_SUCCESS )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "%s: Error retrieving CableCARD feature list!\n",
				 __FUNCTION__ );
		return RMF_OSAL_EINVAL;
	}

	// Allocate a new generic feature list and copy in the feature IDs retrieved
	// from the platform
	if ( rmf_osal_memAllocP
		 ( RMF_OSAL_MEM_POD, sizeof( rmf_PodmgrFeatures ),
		   ( void ** ) &podFeatureList ) != RMF_SUCCESS )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "%s: Error allocating podFeatureList!\n", __FUNCTION__ );
		err = RMF_OSAL_ENOMEM;
		goto error1;
	}
	if ( rmf_osal_memAllocP
		 ( RMF_OSAL_MEM_POD, sizeof( uint8_t ) * cc_feature_list_length,
		   ( void ** ) &podFeatureList->featureIds ) != RMF_SUCCESS )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "updatePODDatabase(), PodFeatureList mem alloc failed value=%d \n",
				 err );
		err = RMF_OSAL_ENOMEM;
		goto error2;
	}
	podFeatureList->number = cc_feature_list_length;
	for ( i = 0; i < cc_feature_list_length; i++ )
		podFeatureList->featureIds[i] = cc_feature_list[i];

	/** Card applications and other information **/

	// Retrieve card capabilities and application info
	// First time through the card is not ready to respond this call
	if ( cablecard_get_info( &cc_info ) != RMF_SUCCESS )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "%s: Error retrieving general CableCARD info!\n",
				 __FUNCTION__ );
		err = RMF_OSAL_EINVAL;
		goto error3;
	}

	if ( cc_info->app_data_length == 0 || cc_info->application_data == NULL )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "%s: cc_info length should not be 0 and data should not be NULL!\n",
				 __FUNCTION__ );
		err = RMF_OSAL_EINVAL;
		goto error4;
	}

	// Allocate app info structure
	if ( rmf_osal_memAllocP
		 ( RMF_OSAL_MEM_POD, sizeof( podmgr_AppInfo ),
		   ( void ** ) &podAppInfo ) != RMF_SUCCESS )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "%s: Error allocating podAppInfo!\n", __FUNCTION__ );
		err = RMF_OSAL_ENOMEM;
		goto error4;
	}

	// Get the card manufacturer ID and version
	podAppInfo->ai_manufacturerId = cc_info->manuf_id;
	podAppInfo->ai_versionNumber = cc_info->version_number;
	podAppInfo->ai_appCount = *( cc_info->application_data );

	// If we have at least one app
	if ( podAppInfo->ai_appCount > 0 )
	{
		if ( rmf_osal_memAllocP
			 ( RMF_OSAL_MEM_POD, cc_info->app_data_length - 1,
			   ( void ** ) &podAppInfo->ai_apps ) != RMF_SUCCESS )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					 "%s: Error allocating podAppInfo apps data!\n",
					 __FUNCTION__ );
			err = RMF_OSAL_ENOMEM;
			goto error5;
		}

		// Populate the app info structure
		memcpy( ( void * ) podAppInfo->ai_apps,
				( void * ) ( cc_info->application_data + 1 ),
				cc_info->app_data_length - 1 );
	}
	else
	{
		podAppInfo->ai_apps = NULL;
	}

	/** Update the POD DB **/

	( void ) rmf_osal_mutexAcquire( podDB->pod_mutex );

	// Update card capability information
	podDB->pod_maxTransportStreams = cc_info->max_transport_streams;
	podDB->pod_maxElemStreams = cc_info->max_elementary_streams;
	podDB->pod_maxPrograms = cc_info->max_programs;

	// Update the generic feature list.  Release old memory if necessary
	if ( podDB->pod_features != NULL )
	{
		( void ) rmf_osal_memFreeP( RMF_OSAL_MEM_POD,
									podDB->pod_features->featureIds );
		( void ) rmf_osal_memFreeP( RMF_OSAL_MEM_POD, podDB->pod_features );
		podDB->pod_features = NULL;
	}
	podDB->pod_features = podFeatureList;

	// Update each generic feature
	for ( i = 0; i < cc_feature_list_length; i++ )
	{
		if ( podGetFeatureParam( podDB, cc_feature_list[i] ) != RMF_SUCCESS )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					 "%s: Error in podGetFeatureParam() %d!\n", __FUNCTION__,
					 cc_feature_list[i] );
			err = RMF_OSAL_EINVAL;
			goto error6;
		}
	}
	rmf_osal_memFreeP( RMF_OSAL_MEM_POD, ( void * ) cc_info );

	// Update the application list.  Release old memory if necessary
	if ( podDB->pod_appInfo != NULL )
	{
		( void ) rmf_osal_memFreeP( RMF_OSAL_MEM_POD,
									podDB->pod_appInfo->ai_apps );
		( void ) rmf_osal_memFreeP( RMF_OSAL_MEM_POD, podDB->pod_appInfo );
		podDB->pod_appInfo = NULL;
	}
	podDB->pod_appInfo = podAppInfo;

	( void ) rmf_osal_mutexRelease( podDB->pod_mutex );

	return RMF_SUCCESS;

  error6:( void ) rmf_osal_mutexRelease( podDB->pod_mutex );
	rmf_osal_memFreeP( RMF_OSAL_MEM_POD, ( void * ) podAppInfo->ai_apps );

  error5:rmf_osal_memFreeP( RMF_OSAL_MEM_POD, ( void * ) podAppInfo );

  error4:rmf_osal_memFreeP( RMF_OSAL_MEM_POD, ( void * ) cc_info );

  error3:( void ) rmf_osal_memFreeP( RMF_OSAL_MEM_POD,
								podFeatureList->featureIds );

  error2:( void ) rmf_osal_memFreeP( RMF_OSAL_MEM_POD, podFeatureList );

  error1:
	return err;
}



/**
 * <i>podInit()</i>
 *
 * Perform any target specific operations to enable interfacing with the POD
 * device via the target HOST-POD devices stack interface.	Depending on the platform
 * implementation this may include stack API call(s) to get the HOST-POD stack resources
 * initialized, or it may simply involve stack API calls(s) to access the data
 * already exhanged between the HOST and POD during the initial platform bootstrap
 * process.
 *
 * @param podDB is a pointer to the platform-independent POD information
 *				database used to cache the POD-Host information.
 *
 * @return RMF_SUCCESS if the initialization process was successful.
 */
rmf_Error podInit( rmf_PODDatabase * podDB )
{
	rmf_Error err;

	// Create our APDU list mutex
	if ( ( err = rmf_osal_mutexNew( &gAPDUMutex ) ) != RMF_SUCCESS )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "%s: Error creating APDU list mutex!\n", __FUNCTION__ );
		return err;
	}

	// Create our APDU "available" cond var
	if ( ( err = rmf_osal_condNew( FALSE, FALSE, &gAPDUReadyCond ) ) != RMF_SUCCESS )	//&gAPDUReadyCond
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "%s: Error creating APDU ready cond var!\n", __FUNCTION__ );
		return err;
	}


	rmf_osal_memAllocP( RMF_OSAL_MEM_POD, sizeof( rmf_PODDatabase ),
						( void ** ) &podDB_plat );
	if ( podDB_plat == NULL )
	{

		RDK_LOG( RDK_LOG_FATAL, "LOG.RDK.POD",
				 "line %d of %s, %s memory allocation failure!\n", __LINE__,
				 __FILE__, __func__ );
	}

	/* At init time the POD has not been inserted and is not ready */
	podDB_plat->pod_isPresent = FALSE;
	podDB_plat->pod_isReady = FALSE;

	/* initialize the decryption capabilities to the default (no POD ready) value of 0 */
	podDB_plat->pod_maxElemStreams = 0;
	podDB_plat->pod_maxPrograms = 0;
	podDB_plat->pod_maxTransportStreams = 0;

	/* Initialize variable size parameter fields to NULL. */
	podDB_plat->pod_featureParams.pc_pin = NULL;
	podDB_plat->pod_featureParams.pc_setting = NULL;
	podDB_plat->pod_featureParams.ippv_pin = NULL;
	podDB_plat->pod_featureParams.cable_urls = NULL;
	podDB_plat->pod_featureParams.term_assoc = NULL;
	podDB_plat->pod_featureParams.zip_code = NULL;

	/* Initialize the rmf_PodmgrFeatures and appInfo member struct pointer */
	podDB_plat->pod_features = NULL;
	podDB_plat->pod_appInfo = NULL;

	/* Allocate the POD DB Mutex */
	err = rmf_osal_mutexNew( &podDB_plat->pod_mutex );

	/* If we failed to create the mutex, set the POD Database struct as NOT READY */
	if ( err != 0 )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "Failed to get mutex, error\n" );

	}

	err = vlMpeosPodInit( podDB_plat );
	if ( err != RMF_SUCCESS )
	{
		POD_enabled = 0;
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
				 "vlMpeosPodInit() Error result=%d \n", err );

		return err;
	}

	//create thread for removing the next APDU 

        err =
                rmf_osal_threadCreate( cablecard_apdu_thread, NULL,
                                                           RMF_OSAL_THREAD_PRIOR_DFLT,
                                                           RMF_OSAL_THREAD_STACK_SIZE,
                                                           &cablecard_apdu_threadId, POD_APDU_THREAD );
        if ( err != RMF_SUCCESS )
        {
                POD_enabled = 0;
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
                                 "failed to create cablecard handler thread .\n" );

                return RMF_OSAL_ENODATA;
        }

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
			 "FEATURE.CABLECARD.SUPPORT: POD Initialized\n" );

	POD_enabled = 1;


	/**
	 * At this point the cable card may not be ready and respond with an error.
	 * Therefore, the return err statement was removed and an error message has
	 * been added. this was necessary to allow the remainder of this init method to
	 * complete its set up. The "podDB->pod_isReady = TRUE" is still need due to the
	 * podmgr_ftable needs this value set true in order too complete execution. The sendEvent
	 * ri card ready only is executed upon success of the updatePodDatabase call.
	 *
	*/

	if ( ( err = updatePODDatabase( podDB ) ) != RMF_SUCCESS )
	{
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
				 "updatePODDatabase(podDB) NOT READY\n" );
	}
	else
	{
		podDB->pod_isReady = TRUE;
		//sendEvent(RMF_PODSRV_EVENT_READY, NULL);
		vlMpeosPostPodEvent( RMF_PODSRV_EVENT_READY, NULL, 0 );
	}

	return RMF_SUCCESS;
}

rmf_Error registerForInternalApdu(  rmf_Error (* pPodProcessApduInt ) ( uint32_t sessionId,  uint8_t * apdu,
								 int32_t len)  )
{
	fnPtrpodProcessApduInt = pPodProcessApduInt;
	return RMF_SUCCESS;
}

/**
 * This thread is used to remove the next APDU pass it events.
 *
 * @param sessionId is a pointer for returning the associated session Id (i.e. Ocm_Sas_Handle).
 * @param apdu the next available APDU from the POD
 * @param len the length of the APDU in bytes
 * @return Upon successful completion, this function shall return RMF_SUCCESS. Otherwise,
 *			one of the errors below is returned.
 * <ul>
 * <li>		RMF_OSAL_ENOMEM - There was insufficient memory available to complete the operation.
 * <li>		RMF_OSAL_EINVAL - One or more parameters were out of range or unuseable.
 * <li>		RMF_OSAL_ENODATA - Indicates that the APDU received is actually the last APDU that
 *						 failed to be sent.
 * </ul>
 */

static void cablecard_apdu_thread( void *data )
{
	rmf_Error result = RMF_SUCCESS;
	uint32_t sessionId = 0;
	uint8_t *apdu = NULL;
	int32_t len;

	while ( 1 )
	{
		if ( RMF_SUCCESS !=
			 ( result = vlMpeosPodReceiveAPDU( &sessionId, &apdu, &len ) ) )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					 "cablecard_apdu_thread() error returned %d\n", result );
			continue;
		}

		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
				 "%s: CCARD_EVENT_APDU_RECV -- id = %x, event_data = %p!\n",
				 __FUNCTION__, sessionId, apdu );
		if( fnPtrpodProcessApduInt )
		{
			result = (*fnPtrpodProcessApduInt )( sessionId, apdu, len );
		}
		if( result == RMF_SUCCESS)
		{
			addAPDU( allocateAPDU( sessionId, apdu, len ) );
			vlMpeosPostPodEvent( RMF_PODSRV_EVENT_RECV_APDU,
							 ( void * ) sessionId, 0 );
		}
	}
}

/**
 * <i>podGetAppInfo</i>
 *
 * Get the pointer to the POD application information.	The application information
 * should have been acquired from the POD-Host interface during the initialization
 * phase and is usually cached within the the database.  But, on some
 * platforms it may be necessary to take additional steps to acquire the application
 * information.  This API provides that on-demand support if necessary.
 *
 * @param podDB is a pointer to the platform-independent POD information
 *				database used to cache the POD-Host information.
 *
 * @return RMF_SUCCESS if the application information was successfully returned.
 */
rmf_Error podGetAppInfo( rmf_PODDatabase * podDB )
{
	cablecard_info_t *cc_info;
	podmgr_AppInfo *podAppInfo;
	uint8_t i;

	rmf_Error result = RMF_SUCCESS;
	if ( !POD_enabled )
		return RMF_OSAL_ENODATA;

	if ( RMF_SUCCESS != ( result = cablecard_get_info( &cc_info ) ) )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "ri_cableCardGetAppInfo() returned %d\n", result );
		return RMF_OSAL_ENODATA;
	}

	// Allocate app info structure
	if ( rmf_osal_memAllocP
		 ( RMF_OSAL_MEM_POD, sizeof( podmgr_AppInfo ),
		   ( void ** ) &podAppInfo ) != RMF_SUCCESS )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "Error allocating podAppInfo!\n" );
		rmf_osal_memFreeP( RMF_OSAL_MEM_POD, ( void * ) cc_info );
		return RMF_OSAL_ENOMEM;
	}
	// Get the card manufacturer ID and version
	podAppInfo->ai_manufacturerId = cc_info->manuf_id;
	podAppInfo->ai_versionNumber = cc_info->version_number;
	podAppInfo->ai_appCount = cc_info->app_data_length;

	// Update card capability information
	// If we have at least one app
	if ( podAppInfo->ai_appCount > 0 )
	{
		if ( rmf_osal_memAllocP
			 ( RMF_OSAL_MEM_POD, cc_info->app_data_length - 1,
			   ( void ** ) &podAppInfo->ai_apps ) != RMF_SUCCESS )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					 "Error allocating podAppInfo apps data!\n" );
		}

		// Populate the app info structure
		/*
		 * for(i=0; i<cc_info->app_data_length; i++)
		 * {
		 * podAppInfo->ai_apps[i] = cc_info->application_data[i];
		 * } */
		memcpy( ( void * ) podAppInfo->ai_apps,
				( void * ) ( cc_info->application_data + 1 ),
				cc_info->app_data_length - 1 );
	}
	else
	{
		podAppInfo->ai_apps = NULL;
	}

	( void ) rmf_osal_mutexAcquire( podDB->pod_mutex );
	//podDB->pod_maxPrograms = gMaxCardPrograms;								//cc_info-> max_programs;
	//podDB->pod_maxElemStreams = gMaxCardElemStreams;			//cc_info-> max_elementary_streams;
	//podDB->pod_maxTransportStreams = gMaxCardTransStreams;	//cc_info->max_transport_streams;

	if ( podDB->pod_appInfo != NULL )
	{
		if ( podDB->pod_appInfo->ai_apps != NULL )
		{
			( void ) rmf_osal_memFreeP( RMF_OSAL_MEM_POD,
										podDB->pod_appInfo->ai_apps );
		}

		( void ) rmf_osal_memFreeP( RMF_OSAL_MEM_POD, podDB->pod_appInfo );
		podDB->pod_appInfo = NULL;

	}

	podDB->pod_appInfo = podAppInfo;
	( void ) rmf_osal_mutexRelease( podDB->pod_mutex );
	rmf_osal_memFreeP( RMF_OSAL_MEM_POD, ( void * ) cc_info );

	return RMF_SUCCESS;
}


/**
 * <i>podGetFeatures</i>
 *
 * Get the POD's generic features list.  Note the generic features list
 * can be updated by the HOST-POD interface asynchronously and since there
 * is no mechanism in the PTV POD support APIs to get notification of this
 * asynchronous update, the list must be refreshed from the interface every
 * the list is requested. The last list acquired will be buffered within the
 * POD database, but it may not be up to date.	
 *
 * @param podDB is a pointer to the platform-independent POD information
 *				database used to cache the POD-Host information.
 *
 * @return RMF_SUCCESS if the features list was successfully acquired.
 */
rmf_Error podGetFeatures( rmf_PODDatabase * podDB )
{
	uint8_t i;
	uint8_t cc_feature_list_length;
	rmf_Error err = RMF_SUCCESS;
	rmf_pod_cablecard_generic_feature *cc_feature_list;

	rmf_PodmgrFeatures *podFeatureList;

	if ( !POD_enabled )
		return RMF_OSAL_ENODATA;

	if ( RMF_SUCCESS !=
		 ( err =
		   cablecard_get_supported_features( &cc_feature_list,
											 &cc_feature_list_length ) ) )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "podGetFeatures() returned NULL\n" );
		goto error0;
	}

	if ( rmf_osal_memAllocP
		 ( RMF_OSAL_MEM_POD, sizeof( rmf_PodmgrFeatures ),
		   ( void ** ) &podFeatureList ) != RMF_SUCCESS )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "%s: Error allocating podFeatureList!\n", __FUNCTION__ );
		err = RMF_OSAL_ENOMEM;
		goto error0;
	}

	err =
		rmf_osal_memAllocP( RMF_OSAL_MEM_POD,
							sizeof( uint8_t ) * cc_feature_list_length,
							( void ** ) &podFeatureList->featureIds );
	if ( err != RMF_SUCCESS )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "podGetFeatures(), PodFeatureList mem alloc failed value=%d \n",
				 err );
		err = RMF_OSAL_ENOMEM;
		goto error2;
	}

	podFeatureList->number = cc_feature_list_length;
	for ( i = 0; i < cc_feature_list_length; i++ )
	{
		podFeatureList->featureIds[i] = cc_feature_list[i];
	}

	( void ) rmf_osal_mutexAcquire( podDB->pod_mutex );

	if ( podDB->pod_features != NULL )
	{
		( void ) rmf_osal_memFreeP( RMF_OSAL_MEM_POD,
									podDB->pod_features->featureIds );
		( void ) rmf_osal_memFreeP( RMF_OSAL_MEM_POD, podDB->pod_features );
		podDB->pod_features = NULL;
	}
	podDB->pod_features = podFeatureList;

	( void ) rmf_osal_mutexRelease( podDB->pod_mutex );


// Update each generic feature
	for ( i = 0; i < cc_feature_list_length; i++ )
	{
		if ( podGetFeatureParam( podDB, cc_feature_list[i] ) != RMF_SUCCESS )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					 "Error in podGetFeatureParam()!\n" );
			return RMF_OSAL_EINVAL;
		}
	}

	return err;

  error2:( void ) rmf_osal_memFreeP( RMF_OSAL_MEM_POD, podFeatureList );

  error0:

	return err;
}

/**
 * <i>podGetFeatureParam</i>
 *
 * Populate the internal POD database with the specified feature parameter value.
 * Ideally this routine can acquire the full set of feature parameters from the
 * HOST-POD interface, but the interface may only support acquisition of a single
 * parameter value at a time.  Unfortunately, at this time PTV does not support
 * acquisition of the full set of feature parameters with a single call (SCTE-28
 * does), so this routine will acquire each feature parameter upon individual
 * request.  Either way, the target value will be cached within the internal database
 * and the calling pod_ API will return the value from the internal database.
 *
 * @param featureId is the identifier of the feature parameter of interest.
 *
 * @return RMF_SUCCESS if the value of the feature was acquired successfully.
 */
rmf_Error podGetFeatureParam( rmf_PODDatabase * podDBPtr, uint32_t featureId )
{
	uint8_t *featureData;
	uint8_t featureDataLength;

	( void ) rmf_osal_mutexAcquire( podDBPtr->pod_mutex );
	// Retrieve feature from platform
	if ( cablecard_get_generic_feature
		 ( featureId, &featureData, &featureDataLength ) != RMF_SUCCESS )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "%s: Error retrieving feature (%d) from platform!\n",
				 __FUNCTION__, featureId );
		( void ) rmf_osal_mutexRelease( podDBPtr->pod_mutex );
		return RMF_OSAL_EINVAL;
	}

	// Update POD DB
	if ( updateGenericFeature
		 ( &podDBPtr->pod_featureParams, featureId, featureData,
		   featureDataLength ) != RMF_SUCCESS )
	{
		rmf_osal_memFreeP( RMF_OSAL_MEM_POD, ( void * ) featureData );
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "%s: Error updating generic feature database (%d)!\n",
				 __FUNCTION__, featureId );
		( void ) rmf_osal_mutexRelease( podDBPtr->pod_mutex );
		return RMF_OSAL_EINVAL;
	}

	rmf_osal_memFreeP( RMF_OSAL_MEM_POD, ( void * ) featureData );
	( void ) rmf_osal_mutexRelease( podDBPtr->pod_mutex );

	return RMF_SUCCESS;
}


/**
 * The podReceiveAPDU() function retrieves the next available APDU from the POD.
 *
 * @param sessionId is a pointer for returning the associated session Id
 * @param apdu the next available APDU from the POD
 * @param len the length of the APDU in bytes
 * @return Upon successful completion, this function shall return RMF_SUCCESS. Otherwise,
 *			one of the errors below is returned.
 * <ul>
 * <li>		RMF_OSAL_ENOMEM - There was insufficient memory available to complete the operation.
 * <li>		RMF_OSAL_EINVAL - One or more parameters were out of range or unuseable.
 * <li>		RMF_OSAL_ENODATA - Indicates that the APDU received is actually the last APDU that
 *						 failed to be sent.
 * </ul>
 */
rmf_Error podReceiveAPDU( uint32_t * sessionId, uint8_t ** apdu,
						  int32_t * len )
{
	*sessionId = FAKE_SESSION_ID;
	os_apdu_node_t *apdu_node = NULL;
	rmf_Error err_value;

	while ( ( apdu_node = removeAPDU(  ) ) == NULL )
	{
		err_value = rmf_osal_condWaitFor( gAPDUReadyCond, 0 );
	}

	if ( 1 )
	{
		// Set return values
		*sessionId = apdu_node->sessionID;
		*apdu = apdu_node->data;
		*len = apdu_node->length;

		// Release our APDU node memory.  Actual APDU memory will be released
		// by rmf_podmgrReleaseAPDU()
		rmf_osal_memFreeP( RMF_OSAL_MEM_POD, apdu_node );
	}


	return RMF_SUCCESS;
}


rmf_Error podInsertAPDU(uint32_t sessionId, uint8_t *data,
        int32_t len)
{
	addAPDU( allocateAPDU( sessionId, data, len ) );
	return RMF_SUCCESS;
}

rmf_Error podProxyApdu( uint32_t *sessionID, uint8_t **data,
									 int32_t *length, uint8_t set )
{
	static os_apdu_node_t proxy_apdu = {0};
	if(( 0 == proxy_apdu.sessionID ) && (set))
	{
		proxy_apdu.data = *data;
		proxy_apdu.length = *length;
		proxy_apdu.sessionID = *sessionID;
		
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
		 "%s: Cached APDU \n", __FUNCTION__);	
	}
	else if(( 0 == proxy_apdu.sessionID ) && ( !set ))
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
		 "%s: No APDU stored in cache\n", __FUNCTION__);
		return RMF_OSAL_EBUSY;
	}	
	else if( !set )
	{
		*data = proxy_apdu.data;
		*length = proxy_apdu.length;
		*sessionID = proxy_apdu.sessionID;
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
		 "%s: Returning cached APDU \n", __FUNCTION__);	
	}

	return RMF_SUCCESS;
}


rmf_Error podClearApduQ(uint32_t ca_systemId, uint32_t count )
{		
	os_apdu_node_t *apdu_node = NULL;
	rmf_Error err_value;
	uint32_t tempCount = 0;
	
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
			 "%s: ca_systemId is %d and count is %d \n",
			 __FUNCTION__, ca_systemId, count);	

	/* to clean up the orphan APDUs */
	while ( ( apdu_node = removeAPDU(  ) ) != NULL )
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				 "%s: Cleaning APDUs \n", __FUNCTION__);	
		if(( NULL != apdu_node->data ) && 
			( apdu_node->sessionID != ca_systemId) )
		{
			if( (count) && (count == tempCount++)) 
			{
				podProxyApdu(&apdu_node->sessionID,
					&apdu_node->data,&apdu_node->length, 1);
			}
			else
			{
				rmf_osal_memFreeP(RMF_OSAL_MEM_POD, apdu_node->data );
			}
		}
		rmf_osal_memFreeP( RMF_OSAL_MEM_POD, apdu_node );
	}
	
	/* to ensure there is no thread waiting for the APDUs */
	addAPDU( allocateAPDU( 0, NULL, 0 ) );
	while ( 1 )
	{
		rmf_osal_threadSleep(50,0);
		if(( apdu_node = removeAPDU(  ) ) == NULL)
		{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				 "%s: Detaching APDU waits \n", __FUNCTION__);	
	    	addAPDU( allocateAPDU( 0, NULL, 0 ) );
		}
		else
		{
			/* there is a possibility for recving CA-PMT */
			uint32_t temp = apdu_node->sessionID;
			rmf_osal_memFreeP( RMF_OSAL_MEM_POD, apdu_node );
			if(0 == temp)
			{
				/* found what send from here */
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
					 "%s: Detached all the waits\n", __FUNCTION__);	
				break;
			}
		}
	}
	
    return RMF_SUCCESS;
}

#define POD_FAKE_CP_SESSION_HANDLE ((podmgr_CPSession)0xBAADF00D)

/**
  * <i>podStartCPSession<i/>
  *
  * Start the CP (Copy Protection) session for the specified service.
  *
  * This method will be called after the initiation of CA (a ca_pmt
  * sent via the CAS session) and will precede initiation of any
  * functions which operate on encrypted content. 
  *
  * @param tunerId that's tuned to the transport stream on which
  * the desired service is carried.
  *
  * @param programNumber the program_number of the associated
  * service.
  *
  * @param ltsid The Logical Transport Stream ID associated with
  * the CA resource (setup via the ca_pmt).
  *
  * @param ecmPid the ECM PID associated with the CableCARD
  * program to monitor.
  *
  * @param programIndex the ca_pmt program index. The program
  * index is used to uniquely identify a service when multiple
  * programs are being decrypted for the same transport stream.
  *
  * @param numPids the number of PIDs in the pids array.
  *
  * @param pids array of PIDs as supplied in the CA_PMT APDU
  * used to initiate the CA session.
  *
  * @return RMF_SUCCESS if the Copy Protection session is
  * successfully started for the identified program.
  */
rmf_Error podStartCPSession( uint32_t tunerId, uint16_t programNumber,
							 uint32_t ltsid, uint16_t ecmPid,
							 uint8_t programIndex, uint32_t numPids,
							 uint32_t pids[], podmgr_CPSession * session )
{
	RMF_OSAL_UNUSED_PARAM( tunerId );
	RMF_OSAL_UNUSED_PARAM( programNumber );
	RMF_OSAL_UNUSED_PARAM( ltsid );
	RMF_OSAL_UNUSED_PARAM( ecmPid );
	RMF_OSAL_UNUSED_PARAM( programIndex );
	RMF_OSAL_UNUSED_PARAM( numPids );
	RMF_OSAL_UNUSED_PARAM( pids );
	RMF_OSAL_UNUSED_PARAM( session );

	// Nothing really to do on the RI. Just setup a token session.

	*session = POD_FAKE_CP_SESSION_HANDLE;
	return RMF_SUCCESS;
}								// END podStartCPSession


/**
  * <i>podStopCPSession<i/>
  *
  * Stop the CP (Copy Protection) session for the specified service.
  *
  * This method will be called after the termination of any
  * functions which operate on encrypted content and precede the
  * termination of the ca_pmt session.
  *
  * @return RMF_SUCCESS if the Copy Protection session is
  * successfully stopped.
  */
rmf_Error podStopCPSession( podmgr_CPSession session )
{
	if ( session != POD_FAKE_CP_SESSION_HANDLE )
	{
		RDK_LOG( RDK_LOG_WARN, "LOG.RDK.POD",
				 "%s: Bad session ID %p (expected %p)\n", __FUNCTION__,
				 session, POD_FAKE_CP_SESSION_HANDLE );
		return RMF_OSAL_EINVAL;
	}
	return RMF_SUCCESS;
}

/**
 * <i>cablecard_get_info</i>
 * Get the pointer to the POD application information.	The application information
 * should have been acquired from the POD-Host interface during the initialization
 * phase and is usually cached here.  But, on some
 * platforms it may be necessary to take additional steps to acquire the application
 * information.  This API provides that on-demand support if necessary.
 * @param podDB is a pointer to the platform-independent POD information
 * database used to cache the POD-Host information.
 *
 * @return RMF_SUCCESS if the application information was successfully returned.
 */
rmf_Error cablecard_get_info( cablecard_info_t ** cc_info )
{
	rmf_Error result;
	uint8_t i;
	uint8_t *app_bytes;
	uint8_t *p;
	uint8_t offset = 0;

	rmf_osal_mutexAcquire( podDB_plat->pod_mutex );
	result = vlMpeosPodGetAppInfo( podDB_plat );

	if ( result == RMF_SUCCESS )
	{
		rmf_osal_memAllocP( RMF_OSAL_MEM_POD, sizeof( cablecard_info_t ),
							( void ** ) cc_info );
		if ( *cc_info == NULL )
		{

			RDK_LOG( RDK_LOG_FATAL, "LOG.RDK.POD",
					 "line %d of %s, %s memory allocation failure!\n",
					 __LINE__, __FILE__, __func__ );
			rmf_osal_mutexRelease( podDB_plat->pod_mutex );
			return RMF_OSAL_ENODATA;

		}

		//Map podDB_plat info to cablecard_info_t structure
		( *cc_info )->manuf_id = podDB_plat->pod_appInfo->ai_manufacturerId;

		( *cc_info )->version_number =
			podDB_plat->pod_appInfo->ai_versionNumber;

		//Not used
		//uint8_t mac_address[6];
		//uint8_t serialnum_length;
		//uint8_t* card_serialnum;

		/*  Need to calculate app_data_length from ai_appCount and ai_apps */
		if ( NULL != podDB_plat->pod_appInfo )
		{
			app_bytes = podDB_plat->pod_appInfo->ai_apps;
			p = app_bytes;
			offset = 0;
			if ( NULL != app_bytes )
			{
				/* Calculate the total length of app_bytes */
				int app_count = podDB_plat->pod_appInfo->ai_appCount;

				for ( i = 0; i < app_count; ++i )
				{
					/* Calculate length per Table 9.5-4 of CCIF2.0 */
					offset += 1;		/* application_type */
					offset += 2;		/* application _version_number */
					offset += 1;		/* application_name_length */
					offset += p[offset - 1];	/* application_name_bytes */
					offset += 1;		/* application_url_length */
					offset += p[offset - 1];	/* application_url_bytes */
				}
			}
		}
		( *cc_info )->app_data_length = offset;
		( *cc_info )->app_data_length += 1;		/* Add 1 byte to hold number_of_applications */
		rmf_osal_memAllocP( RMF_OSAL_MEM_POD,
							( ( *cc_info )->app_data_length ),
							( void ** ) &( ( *cc_info )->application_data ) );
		if ( ( ( *cc_info )->application_data ) == NULL )
		{
			RDK_LOG( RDK_LOG_FATAL, "LOG.RDK.POD",
					 "line %d of %s, %s memory allocation failure!\n",
					 __LINE__, __FILE__, __func__ );
		}

		*( ( *cc_info )->application_data ) =
			podDB_plat->pod_appInfo->ai_appCount;
		memcpy( ( *cc_info )->application_data + 1,
				podDB_plat->pod_appInfo->ai_apps,
				( *cc_info )->app_data_length - 1 );

		( *cc_info )->max_programs = podDB_plat->pod_maxPrograms;

		( *cc_info )->max_elementary_streams = podDB_plat->pod_maxElemStreams;

		( *cc_info )->max_transport_streams =
			podDB_plat->pod_maxTransportStreams;
	}
	else
	{
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
				 "vlMpeosPodGetAppInfo(podDB_plat) FAILED !!! \n" );
		rmf_osal_mutexRelease( podDB_plat->pod_mutex );
		return RMF_OSAL_ENODATA;
	}
	rmf_osal_mutexRelease( podDB_plat->pod_mutex );
	return RMF_SUCCESS;
}


/**
 * <i>cablecard_get_supported_features</i>
 *
 * Get the POD's generic features list.  Note the generic features list
 * can be updated by the HOST-POD interface asynchronously and since there
 * is no mechanism in the PTV POD support APIs to get notification of this
 * asynchronous update, the list must be refreshed from the interface every
 * the list is requested. The last list acquired will be buffered within the
 * POD database, but it may not be up to date.	 any previous list and 
 * refresh with a new one whenever a request for
 * the list is made.
 *
 * @param podDB is a pointer to the platform-independent POD information
 *				database used to cache the POD-Host information.
 *
 * @return RMF_SUCCESS if the features list was successfully acquired.
 */
rmf_Error cablecard_get_supported_features( uint8_t ** cc_feature_list,
											uint8_t * cc_feature_list_length )
{
	rmf_Error result;

	rmf_osal_mutexAcquire( podDB_plat->pod_mutex );
	result = vlMpeosPodGetFeatures( podDB_plat );
	if ( RMF_SUCCESS != result )
	{

		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "ri_cabelCardGetFeatures() returned %d\n", result );

		rmf_osal_mutexRelease( podDB_plat->pod_mutex );
		return result;
	}
	//Put the value of "numbers of available features" in the address pointed to by the cc_feature_list_length object
	*cc_feature_list_length = podDB_plat->pod_features->number;
	*cc_feature_list = podDB_plat->pod_features->featureIds;
	rmf_osal_mutexRelease( podDB_plat->pod_mutex );

	return result;
}


/**
 * <i>cablecard_get_generic_feature</i>
 *
 * Populate the internal POD database with the specified feature parameter value.
 * Ideally this routine can acquire the full set of feature parameters from the
 * HOST-POD interface, but the interface may only support acquisition of a single
 * parameter value at a time.  Unfortunately, at this time PTV does not support
 * acquisition of the full set of feature parameters with a single call (SCTE-28
 * does), so this routine will acquire each feature parameter upon individual
 * request.  Either way, the target value will be cached within the internal database
 * and the calling pod_ API will return the value from the internal database.
 *
 * @param featureId is the identifier of the feature parameter of interest.
 *
 * @return RMF_SUCCESS if the value of the feature was acquired successfully.
 */
rmf_Error cablecard_get_generic_feature( uint32_t feature_id,
										 uint8_t ** feature_data,
										 uint8_t * data_length )
{
	rmf_Error result = RMF_SUCCESS;
	uint16_t cnt;

	rmf_osal_mutexAcquire( podDB_plat->pod_mutex );
	result = vlMpeosPodGetFeatureParam( podDB_plat, feature_id );
	if ( RMF_SUCCESS != result )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "cablecard_get_generic_feature() returned %d\n", result );

		rmf_osal_mutexRelease( podDB_plat->pod_mutex );
		return result;
	}

	switch ( feature_id )
	{
	  case RMF_PODMGR_GF_RF_OUTPUT_CHANNEL:
		  /* Calculate length per Table 9.15-12 of CCIF2.0 */
		  *data_length =
			  sizeof( podDB_plat->pod_featureParams.rf_output_channel ) /
			  sizeof( uint8_t );
		  rmf_osal_memAllocP( RMF_OSAL_MEM_POD,
							  ( *data_length ) * sizeof( uint8_t ),
							  ( void ** ) feature_data );
		  if ( *feature_data == NULL )
		  {

			  RDK_LOG( RDK_LOG_FATAL, "LOG.RDK.POD",
					   "line %d of %s, %s memory allocation failure!\n",
					   __LINE__, __FILE__, __func__ );

			  break;
		  }
		  rmf_osal_memcpy( *feature_data,
				  podDB_plat->pod_featureParams.rf_output_channel,
				  *data_length, *data_length, sizeof(podDB_plat->pod_featureParams.rf_output_channel));
		  break;
	  case RMF_PODMGR_GF_P_C_PIN:
		  *data_length = ( *podDB_plat->pod_featureParams.pc_pin ) + 1;
		  rmf_osal_memAllocP( RMF_OSAL_MEM_POD,
							  ( *data_length ) * sizeof( uint8_t ),
							  ( void ** ) feature_data );
		  if ( *feature_data == NULL )
		  {
			  RDK_LOG( RDK_LOG_FATAL, "LOG.RDK.POD",
					   "line %d of %s, %s memory allocation failure!\n",
					   __LINE__, __FILE__, __func__ );
			  break;
		  }
		  memcpy( *feature_data, podDB_plat->pod_featureParams.pc_pin,
				  *data_length );
		  break;
	  case RMF_PODMGR_GF_P_C_SETTINGS:
		  cnt =
			  ( podDB_plat->pod_featureParams.pc_setting[2] +
				( podDB_plat->pod_featureParams.pc_setting[1] << 8 ) * 3 ) +
			  3;
		  rmf_osal_memAllocP( RMF_OSAL_MEM_POD, ( cnt ) * sizeof( uint8_t ),
							  ( void ** ) feature_data );
		  if ( *feature_data == NULL )
		  {
			  RDK_LOG( RDK_LOG_FATAL, "LOG.RDK.POD",
					   "line %d of %s, %s memory allocation failure!\n",
					   __LINE__, __FILE__, __func__ );
			  break;
		  }
		  memcpy( *feature_data, podDB_plat->pod_featureParams.pc_setting,
				  cnt );
		  *data_length = cnt;
		  break;
	  case RMF_PODMGR_GF_IPPV_PIN:
		  *data_length = ( *podDB_plat->pod_featureParams.ippv_pin ) + 1;
		  rmf_osal_memAllocP( RMF_OSAL_MEM_POD,
							  ( *data_length ) * sizeof( uint8_t ),
							  ( void ** ) feature_data );
		  if ( *feature_data == NULL )
		  {
			  RDK_LOG( RDK_LOG_FATAL, "LOG.RDK.POD",
					   "line %d of %s, %s memory allocation failure!\n",
					   __LINE__, __FILE__, __func__ );
			  break;
		  }
		  memcpy( *feature_data, podDB_plat->pod_featureParams.ippv_pin,
				  *data_length );
		  break;
	  case RMF_PODMGR_GF_TIME_ZONE:
		  *data_length =
			  sizeof( podDB_plat->pod_featureParams.time_zone_offset ) /
			  sizeof( uint8_t );
		  rmf_osal_memAllocP( RMF_OSAL_MEM_POD,
							  ( *data_length ) * sizeof( uint8_t ),
							  ( void ** ) feature_data );
		  if ( *feature_data == NULL )
		  {
			  RDK_LOG( RDK_LOG_FATAL, "LOG.RDK.POD",
					   "line %d of %s, %s memory allocation failure!\n",
					   __LINE__, __FILE__, __func__ );
			  break;
		  }
		  rmf_osal_memcpy( *feature_data,
				  podDB_plat->pod_featureParams.time_zone_offset,
				  *data_length, *data_length, sizeof(podDB_plat->pod_featureParams.time_zone_offset) );
		  break;
	  case RMF_PODMGR_GF_DAYLIGHT_SAVINGS:
		  *data_length =
			  sizeof( podDB_plat->pod_featureParams.daylight_savings ) /
			  sizeof( uint8_t );
		  rmf_osal_memAllocP( RMF_OSAL_MEM_POD,
							  ( *data_length ) * sizeof( uint8_t ),
							  ( void ** ) feature_data );
		  if ( *feature_data == NULL )
		  {
			  RDK_LOG( RDK_LOG_FATAL, "LOG.RDK.POD",
					   "line %d of %s, %s memory allocation failure!\n",
					   __LINE__, __FILE__, __func__ );

			  break;
		  }
		  rmf_osal_memcpy( *feature_data,
				  podDB_plat->pod_featureParams.daylight_savings,
				  *data_length, *data_length, sizeof(podDB_plat->pod_featureParams.daylight_savings) );
		  break;
	  case RMF_PODMGR_GF_AC_OUTLET:
		  *data_length = 1;
		  rmf_osal_memAllocP( RMF_OSAL_MEM_POD,
							  ( *data_length ) * sizeof( uint8_t ),
							  ( void ** ) feature_data );
		  if ( *feature_data == NULL )
		  {
			  RDK_LOG( RDK_LOG_FATAL, "LOG.RDK.POD",
					   "line %d of %s, %s memory allocation failure!\n",
					   __LINE__, __FILE__, __func__ );
			  break;
		  }
		  rmf_osal_memcpy( *feature_data,
				  &podDB_plat->pod_featureParams.ac_outlet_ctrl,
				  *data_length, *data_length, sizeof(podDB_plat->pod_featureParams.ac_outlet_ctrl)  );
		  break;
	  case RMF_PODMGR_GF_LANGUAGE:
		  *data_length =
			  sizeof( podDB_plat->pod_featureParams.language_ctrl ) /
			  sizeof( uint8_t );
		  rmf_osal_memAllocP( RMF_OSAL_MEM_POD,
							  ( *data_length ) * sizeof( uint8_t ),
							  ( void ** ) feature_data );
		  if ( *feature_data == NULL )
		  {
			  RDK_LOG( RDK_LOG_FATAL, "LOG.RDK.POD",
					   "line %d of %s, %s memory allocation failure!\n",
					   __LINE__, __FILE__, __func__ );
			  break;
		  }
		  rmf_osal_memcpy( *feature_data, podDB_plat->pod_featureParams.language_ctrl,
				  *data_length, *data_length, sizeof( podDB_plat->pod_featureParams.language_ctrl ) );
		  break;
	  case RMF_PODMGR_GF_RATING_REGION:
		  *data_length = 1;
		  rmf_osal_memAllocP( RMF_OSAL_MEM_POD,
							  ( *data_length ) * sizeof( uint8_t ),
							  ( void ** ) feature_data );
		  if ( *feature_data == NULL )
		  {
			  RDK_LOG( RDK_LOG_FATAL, "LOG.RDK.POD",
					   "line %d of %s, %s memory allocation failure!\n",
					   __LINE__, __FILE__, __func__ );
			  break;
		  }
		  rmf_osal_memcpy( *feature_data,
				  &podDB_plat->pod_featureParams.ratings_region,
				  *data_length, *data_length, sizeof(podDB_plat->pod_featureParams.ratings_region) );
		  break;
	  case RMF_PODMGR_GF_RESET_P_C_PIN:
		  *data_length = 1;
		  rmf_osal_memAllocP( RMF_OSAL_MEM_POD,
							  ( *data_length ) * sizeof( uint8_t ),
							  ( void ** ) feature_data );
		  if ( *feature_data == NULL )
		  {
			  RDK_LOG( RDK_LOG_FATAL, "LOG.RDK.POD",
					   "line %d of %s, %s memory allocation failure!\n",
					   __LINE__, __FILE__, __func__ );
			  break;
		  }
		  rmf_osal_memcpy( *feature_data,
				  &podDB_plat->pod_featureParams.reset_pin_ctrl,
				  *data_length, *data_length, sizeof(podDB_plat->pod_featureParams.reset_pin_ctrl) );
		  break;
	  case RMF_PODMGR_GF_CABLE_URLS:
		  *data_length = podDB_plat->pod_featureParams.cable_urls_length;
		  rmf_osal_memAllocP( RMF_OSAL_MEM_POD,
							  ( *data_length ) * sizeof( uint8_t ),
							  ( void ** ) feature_data );
		  if ( *feature_data == NULL )
		  {
			  RDK_LOG( RDK_LOG_FATAL, "LOG.RDK.POD",
					   "line %d of %s, %s memory allocation failure!\n",
					   __LINE__, __FILE__, __func__ );
			  break;
		  }
		  memcpy( *feature_data, podDB_plat->pod_featureParams.cable_urls,
				  *data_length );
		  break;
	  case RMF_PODMGR_GF_EA_LOCATION:
		  *data_length =
			  sizeof( podDB_plat->pod_featureParams.ea_location ) /
			  sizeof( uint8_t );
		  rmf_osal_memAllocP( RMF_OSAL_MEM_POD,
							  ( *data_length ) * sizeof( uint8_t ),
							  ( void ** ) feature_data );
		  if ( *feature_data == NULL )
		  {
			  RDK_LOG( RDK_LOG_FATAL, "LOG.RDK.POD",
					   "line %d of %s, %s memory allocation failure!\n",
					   __LINE__, __FILE__, __func__ );
			  break;
		  }
		  rmf_osal_memcpy( *feature_data, podDB_plat->pod_featureParams.ea_location,
				  *data_length, *data_length, sizeof( podDB_plat->pod_featureParams.ea_location ) );
		  break;
	  case RMF_PODMGR_GF_VCT_ID:
		  *data_length =
			  sizeof( podDB_plat->pod_featureParams.vct_id ) /
			  sizeof( uint8_t );
		  rmf_osal_memAllocP( RMF_OSAL_MEM_POD,
							  ( *data_length ) * sizeof( uint8_t ),
							  ( void ** ) feature_data );
		  if ( *feature_data == NULL )
		  {
			  RDK_LOG( RDK_LOG_FATAL, "LOG.RDK.POD",
					   "line %d of %s, %s memory allocation failure!\n",
					   __LINE__, __FILE__, __func__ );
			  break;
		  }
		  rmf_osal_memcpy( *feature_data, podDB_plat->pod_featureParams.vct_id,
				  *data_length, *data_length, sizeof(podDB_plat->pod_featureParams.vct_id) );
		  break;
	  case RMF_PODMGR_GF_TURN_ON_CHANNEL:
		  *data_length =
			  sizeof( podDB_plat->pod_featureParams.turn_on_channel ) /
			  sizeof( uint8_t );
		  rmf_osal_memAllocP( RMF_OSAL_MEM_POD,
							  ( *data_length ) * sizeof( uint8_t ),
							  ( void ** ) feature_data );
		  if ( *feature_data == NULL )
		  {
			  RDK_LOG( RDK_LOG_FATAL, "LOG.RDK.POD",
					   "line %d of %s, %s memory allocation failure!\n",
					   __LINE__, __FILE__, __func__ );
			  break;
		  }
		  break;
	  case RMF_PODMGR_GF_TERM_ASSOC:
		  cnt =
			  ( podDB_plat->pod_featureParams.term_assoc[1] +
				( podDB_plat->pod_featureParams.term_assoc[0] << 8 ) ) + 2;
		  rmf_osal_memAllocP( RMF_OSAL_MEM_POD, ( cnt ) * sizeof( uint8_t ),
							  ( void ** ) feature_data );
		  if ( *feature_data == NULL )
		  {
			  RDK_LOG( RDK_LOG_FATAL, "LOG.RDK.POD",
					   "line %d of %s, %s memory allocation failure!\n",
					   __LINE__, __FILE__, __func__ );
			  break;
		  }
		  memcpy( *feature_data, podDB_plat->pod_featureParams.term_assoc,
				  cnt );
		  *data_length = cnt;
		  break;
	  case RMF_PODMGR_GF_DOWNLOAD_GRP_ID:
		  *data_length =
			  sizeof( podDB_plat->pod_featureParams.cdl_group_id ) /
			  sizeof( uint8_t );
		  rmf_osal_memAllocP( RMF_OSAL_MEM_POD,
							  ( *data_length ) * sizeof( uint8_t ),
							  ( void ** ) feature_data );
		  if ( *feature_data == NULL )
		  {
			  RDK_LOG( RDK_LOG_FATAL, "LOG.RDK.POD",
					   "line %d of %s, %s memory allocation failure!\n",
					   __LINE__, __FILE__, __func__ );
			  break;
		  }
		  memcpy( *feature_data, podDB_plat->pod_featureParams.cdl_group_id,
				  *data_length );
		  break;
	  case RMF_PODMGR_GF_ZIP_CODE:
		  cnt =
			  ( podDB_plat->pod_featureParams.zip_code[1] +
				( podDB_plat->pod_featureParams.zip_code[0] << 8 ) ) + 2;
		  rmf_osal_memAllocP( RMF_OSAL_MEM_POD, ( cnt ) * sizeof( uint8_t ),
							  ( void ** ) feature_data );
		  if ( *feature_data == NULL )
		  {
			  RDK_LOG( RDK_LOG_FATAL, "LOG.RDK.POD",
					   "line %d of %s, %s memory allocation failure!\n",
					   __LINE__, __FILE__, __func__ );
			  break;
		  }
		  memcpy( *feature_data, podDB_plat->pod_featureParams.zip_code,
				  cnt );
		  *data_length = cnt;
		  break;
	  default:

		  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				   "cablecard_get_generic_feature(), feature not supported= %d\n",
				   feature_id );
		  rmf_osal_mutexRelease( podDB_plat->pod_mutex );
		  return RMF_OSAL_ENODATA;
	}

	if ( *feature_data == NULL )
	{
		RDK_LOG( RDK_LOG_FATAL, "LOG.RDK.POD",
				 "line %d of %s, %s memory allocation failure!\n", __LINE__,
				 __FILE__, __func__ );

		rmf_osal_mutexRelease( podDB_plat->pod_mutex );
		return RMF_OSAL_EMUTEX;
	}

	rmf_osal_mutexRelease( podDB_plat->pod_mutex );
	return RMF_SUCCESS;
}
