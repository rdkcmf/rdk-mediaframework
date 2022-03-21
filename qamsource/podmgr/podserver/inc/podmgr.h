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

#ifndef _PODMGR_H_
#define _PODMGR_H_

#include <podimpl.h>
#include <rmf_osal_error.h>
#include <rmf_osal_sync.h>

#ifdef __cplusplus
extern "C"
{
#endif


/**
 * Tag definition from CCIF-2.0-I18
 */
#define APDU_TAG_SIZE               (3)         /* 3 BYTES */
#define CA_PMT_HEADER_LENGTH        (14)
#define CA_PMT_OK_DESCRAMBLE_CMD    (1)
#define CA_PMT_UNKNOWN_PROGRAM_NUM  (0)
#define CA_PMT_UNKNOWN_SOURCE_ID    (0)
/*CA PMT Command ID required to send pmt to the POD stack*/

#define MAX_UDP_DATAGRAM_SIZE (4 * 1024)

/****************************************************************************************
 *
 * POD FUNCTIONS
 *
 ***************************************************************************************/


/**
 * Structure representing data from an application_info_cnf() message.
 * See SCTE28 Table 8.4-L.
 */
typedef struct _podmgr_AppInfo
{
    uint16_t ai_manufacturerId; /* POD manufacturer identifier. */
    uint16_t ai_versionNumber; /* POD version number. */
    uint8_t ai_appCount; /* Number of application to follow. */
    uint8_t* ai_apps; /* Array of 0 or more applications (see above). */
} podmgr_AppInfo;


/* handle to Copy Protection session */
typedef struct _podmgr_CPSession
{
    int unused1;
}*podmgr_CPSession;


/**
 * This structure is the main database container for all of the POD related information.
 */
typedef struct _rmf_PODDatabase
{
    rmf_osal_Mutex pod_mutex; /* Mutex for synchronous access */
    rmf_osal_Bool pod_isPresent; /* POD present flag (possibly not yet initialized). */
    rmf_osal_Bool pod_isReady; /* POD present & initialized flag. */
    podmgr_AppInfo *pod_appInfo; /* POD application information. */
    rmf_PodmgrFeatures *pod_features; /* POD generic feature list. */
    rmf_PodmgrFeatureParams pod_featureParams; /* POD generic feature parameters. */
    uint32_t pod_maxElemStreams; /* how many Elementary streams the POD can decode at one time */
    uint32_t pod_maxPrograms; /* how many programs the POD can decode at one time */
    uint32_t pod_maxTransportStreams; /* how many transport streams the POD can decode at one time */
} rmf_PODDatabase;

typedef uint8_t rmf_pod_cablecard_generic_feature;


/**
 * Basic CableCARD information, including applications located on the card.
 *
 * Most of this data comes from the application_info_cnf() APDU defined in
 * OC-SP-CCIF2.0 Table 9.5-4.
 */
typedef struct cablecard_info_s
{
    uint16_t manuf_id;
    uint16_t version_number;

    uint8_t mac_address[6];

    uint8_t serialnum_length;
    uint8_t* card_serialnum;

    uint8_t app_data_length; // The length (in bytes) of the application_data byte array
    uint8_t* application_data; // The exact byte contents of the app loop from Table 9.5-4
    // (including the number_of_applications field)
    uint8_t max_programs;
    uint8_t max_elementary_streams;
    uint8_t max_transport_streams;

} cablecard_info_t;

/**
 * Represents an open connection to a CableCARD resource
 */


typedef void* pod_session_id;

rmf_Error podImplInit(void);


/**
 * <i>podInit()</i>
 *
 * Perform any target specific operations to enable interfacing with the POD
 * device via the target HOST-POD devices stack interface.  Depending on the platform
 * implementation this may include stack API call(s) to get the HOST-POD stack resources
 * initialized, or it may simply involve stack API calls(s) to access the data
 * already exchanged between the HOST and POD during the initial platform bootstrap
 * process.
 *
 * @param podDB is a pointer to the platform-independent POD information
 *              database used to cache the POD-Host information.
 *
 * @return RMF_SUCCESS if the initialization process was successful.
 */
rmf_Error podInit(rmf_PODDatabase *podDB);

rmf_Error registerForInternalApdu(  rmf_Error ( * pPodProcessApduInt ) ( uint32_t sessionId,  uint8_t * apdu,
								 int32_t len)  );
rmf_Error podStartCPSession( uint32_t tunerId,
                                   uint16_t programNumber,
                                   uint32_t ltsid,
                                   uint16_t ecmPid,
                                   uint8_t programIndex,
                                   uint32_t numPids,
                                   uint32_t pids[],
                                   podmgr_CPSession *session );



/**
 * <i>podGetAppInfo</i>
 *
 * Get the pointer to the POD application information.  The application information
 * should have been acquired from the POD-Host interface during the initialization
 * phase and is usually cached within the POD database.  But, on some
 * platforms it may be necessary to take additional steps to acquire the application
 * information.  This API provides that on-demand support if necessary.
 *
 * @param podDB is a pointer to the platform-independent POD information
 *              database used to cache the POD-Host information.
 *
 * @return RMF_SUCCESS if the application information was successfully returned.
 */
rmf_Error podGetAppInfo(rmf_PODDatabase *podDB);

/**
 * <i>podGetFeatures</i>
 *
 * Get the POD's generic features list.
 *
 * @param podDB is a pointer to the platform-independent POD information
 *              database used to cache the POD-Host information.
 *
 * @return RMF_SUCCESS if the features list was successfully acquired.
 */
rmf_Error podGetFeatures(rmf_PODDatabase *podDB);

/**
 * <i>podGetFeatureParam</i>
 *
 * Populate the internal POD database with the specified feature parameter value.
 *
 * @param featureId is the identifier of the feature parameter of interest.
 *
 * @return RMF_SUCCESS if the value of the feature was acquired successfully.
 */
rmf_Error podGetFeatureParam(rmf_PODDatabase *podDBPtr,
        uint32_t featureId);



/**
 * The podReceiveAPDU() function retrieves the next available APDU from the POD.
 * This function also will receive "copy" APDU's that failed to be sent to the POD so that
 * the APDU can be returned to the original sender to be notified of the failure.
 *
 * This API utilizes the definition of an APDU as defined by CCIF:
 *
 * APDU() {
 *   apdu_tag 24 uimsbf
 *   length_field()
 *   for (i=0; i<length_value; i++) {
 *     data_byte 8 uimsbf
 * }}
 *
 * rmf_podmgrReleaseAPDU() will release the APDU buffer returned by this function.
 *
 * @param sessionId location to store the native session handle associated with
 *                   the received APDU.
 * @param apdu location to store a pointer to the received APDU
 * @param len location to store the length of the APDU data, in bytes
 * @return Upon successful completion, this function shall return
 * <ul>
 * <li>     RMF_SUCCESS - The APDU was successfully retrieved, the associated session
 *                        ID was copied to *sessionId, a pointer to the buffer
 *                        was stored in *apdu, and the length of the buffer copied to *len
 * <li>     RMF_OSAL_ENOMEM - There was insufficient memory available to complete the operation.
 * <li>     RMF_OSAL_EINVAL - One or more parameters were out of range or unusable.
 * <li>     RMF_OSAL_ENODATA - Indicates that the APDU received is actually the last APDU that
 *                       failed to be sent.
 * </ul>
 */

rmf_Error podReceiveAPDU(uint32_t *sessionId, uint8_t **apdu,
        int32_t *len);

rmf_Error podClearApduQ(uint32_t ca_systemId, uint32_t count );
rmf_Error podInsertAPDU(uint32_t sessionId, uint8_t *data, int32_t len);
rmf_Error podProxyApdu( uint32_t *sessionID, uint8_t **data,
		int32_t *length, uint8_t set );


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
rmf_Error podStopCPSession(podmgr_CPSession session);


/**
 * Retrieve basic CableCARD information.  The returned pointer must be subsequently
 * passed to ri_cablecard_release_data() so that the platform may release allocated
 * resources associated with the info structure
 *
 * @param info the address of a pointer to a cablecard_info_t structure that
 *        will hold the platform-allocated CableCARD information upon successful
 *        return
 *
 * @return Upon success, returns RI_ERROR_NONE.  Otherwise:
 *     RI_ERROR_CABLECARD_NOT_READY: Card not inserted or not ready
 *     RI_ERROR_ILLEGAL_ARG: Illegal or NULL arguments specified
 */
rmf_Error cablecard_get_info(cablecard_info_t** info);

/**
 * Returns the list of Generic Features supported by this CableCARD.  The returned
 * feature list pointer must be subsequently passed to ri_cablecard_release_data()
 * so that the platform may release allocated resources (feature_list only)
 *
 * @param feature_list the address of an array of generic features IDs that
 *        that will hold the platform-allocated list of generic features
 *        supported by this card upon successful return
 * @param num_features the address where the platform will return the length of
 *        the returned generic feature array      
 *
 * @return Upon success, returns RI_ERROR_NONE.  Otherwise:
 *     RI_ERROR_CABLECARD_NOT_READY: Card not inserted or not ready
 *     RI_ERROR_ILLEGAL_ARG: Illegal or NULL arguments specified
 */

rmf_Error cablecard_get_supported_features(
        rmf_pod_cablecard_generic_feature** feature_list, uint8_t* num_features);

/**
 * Returns the generic feature associated with the given feature ID.  The returned
 * feature data poitner must be subsequently passed to ri_cablecard_release_data()
 * so that the platform may release allocated resources (feature_data only)
 *
 * @param feature_id the requested generic feature ID
 * @param feature_data the address of a byte buffer that will hold the generic
 *        feature data upon successful return
 * @param data_length the address where the platform will return the length of
 *        the returned generic feature data bufer
 *
 * @return Upon success, returns RI_ERROR_NONE.  Otherwise:
 *     RI_ERROR_CABLECARD_NOT_READY: Card not inserted or not ready
 *     RI_ERROR_GF_NOT_SUPPORTED: The given generic feature ID is not supported
 *     RI_ERROR_ILLEGAL_ARG: Illegal or NULL arguments specified
 */

rmf_Error cablecard_get_generic_feature(
        uint32_t feature_id, uint8_t** feature_data,
        uint8_t* data_length);


rmf_Error getCardMfgId( uint16_t *ai_manufacturerId);
rmf_Error getCardVersionNo( uint16_t *ai_versionNumber);

#ifdef __cplusplus
}
;
#endif

#endif /* _PODMGR_H_ */
