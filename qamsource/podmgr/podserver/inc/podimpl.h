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


#include <rmf_inbsi_common.h>

#ifndef RMF_POD_H
#define RMF_POD_H
#ifdef __cplusplus
extern "C"
{
#endif

#include <rmf_osal_types.h>
#include <rmf_osal_error.h>
#include <rmf_osal_event.h>


/****************************************************************************************
 *
 * TYPE AND STRUCTURE DEFINITIONS
 *
 ***************************************************************************************/
/* POD Generic Feature Identifiers per CCIF Table 9.15-2 */
#define RMF_PODMGR_GF_RF_OUTPUT_CHANNEL 0x1  // rf output channel
#define RMF_PODMGR_GF_P_C_PIN           0x2  // parental control pinM
#define RMF_PODMGR_GF_P_C_SETTINGS      0x3  // parental control settings
#define RMF_PODMGR_GF_IPPV_PIN          0x4  // ippv pin
#define RMF_PODMGR_GF_TIME_ZONE         0x5  // time zone
#define RMF_PODMGR_GF_DAYLIGHT_SAVINGS  0x6  // daylight savings control
#define RMF_PODMGR_GF_AC_OUTLET         0x7  // ac outlet
#define RMF_PODMGR_GF_LANGUAGE          0x8  // language
#define RMF_PODMGR_GF_RATING_REGION     0x9  // rating region
#define RMF_PODMGR_GF_RESET_P_C_PIN     0xa  // reset pin
#define RMF_PODMGR_GF_CABLE_URLS        0xb  // cable urls
#define RMF_PODMGR_GF_EA_LOCATION       0xc  // EAS location code
#define RMF_PODMGR_GF_VCT_ID            0xd  // emergency alert location code
#define RMF_PODMGR_GF_TURN_ON_CHANNEL   0xe  // turn-on channel
#define RMF_PODMGR_GF_TERM_ASSOC        0xf  // terminal association
#define RMF_PODMGR_GF_DOWNLOAD_GRP_ID   0x10 // download group ID
#define RMF_PODMGR_GF_ZIP_CODE          0x11 // zip code

/* from pod_util.h  */
/*CA PMT Command ID required to send pmt to the POD stack*/

#define RMF_PODMGR_CA_INFO_INQUIRY_TAG         (0x9F8030)
#define RMF_PODMGR_CA_INFO_TAG                 (0x9F8031)
#define RMF_PODMGR_CA_PMT_TAG                  (0x9F8032)
#define RMF_PODMGR_CA_PMT_REPLY_TAG            (0x9F8033)
#define RMF_PODMGR_CA_PMT_UPDATE_TAG           (0x9F8034)
#define RMF_PODMGR_CA_ENABLE_NO_VALUE                      (0x0)
#define RMF_PODMGR_CA_ENABLE_SET                           (0x80)

#define RMF_PODMGR_CA_ENABLE_DESCRAMBLING_NO_CONDITIONS    (0x1)
#define RMF_PODMGR_CA_ENABLE_DESCRAMBLING_WITH_PAYMENT     (0x2)
#define RMF_PODMGR_CA_ENABLE_DESCRAMBLING_WITH_TECH        (0x3)
#define RMF_PODMGR_CA_ENABLE_DESCRAMBLING_PAYMENT_FAIL     (0x71)
#define RMF_PODMGR_CA_ENABLE_DESCRAMBLING_TECH_FAIL        (0x73)

#define RMF_PODMGR_IP_ADDR_LEN          90

#define RMF_PODSRV_INB_FWU_TUNING_ACCEPTED 0x00// Tuning accepted
#define RMF_PODSRV_INB_FWU_INVALID_FREQUENCY 0x01// Invalid frequency (Host does not support this frequency)
#define RMF_PODSRV_INB_FWU_INVALID_MODULATION 0x02// Invalid modulation (Host does not support this modulation type)
#define RMF_PODSRV_INB_FWU_TUNE_FAILURE 0x03// Hardware failure (Host has hardware failure)
#define RMF_PODSRV_INB_FWU_TUNER_BUSY 0x04// Tuner busy (Host is not relinquishing control of inband tuner)


typedef enum
{
	RMF_PODMGR_CA_OK_DESCRAMBLE = 0x01,
	RMF_PODMGR_CA_OK_MMI = 0x02,
	RMF_PODMGR_CA_QUERY = 0x03,
	RMF_PODMGR_CA_NOT_SELECTED = 0x04
} rmf_PODMGR_CA_PMT_CMD_ID;


/* POD Events */
typedef enum
{
    RMF_PODSRV_EVENT_NO_EVENT = 0x9000,
    RMF_PODSRV_EVENT_GF_UPDATE, /* Generic Feature values updated */
    RMF_PODSRV_EVENT_APPINFO_UPDATE, /* App Info updated */
    RMF_PODSRV_EVENT_INSERTED, /* CableCard inserted into slot */
    RMF_PODSRV_EVENT_READY, /* CableCard is ready for use */
    RMF_PODSRV_EVENT_REMOVED, /* CableCard removed from slot */
    RMF_PODSRV_EVENT_RECV_APDU, /* Message is available to a mpe_podReceiveAPDU call. optionalEventData1 == sessionID of APDU. */

    /*
     * Note that the mpe_podReceiveAPDU also receives APDU's that failed to be sent to the POD.  This is so that
     * the APDU can be returned to the original sender so they can be notified of the failure.
     *
     * MPEOS will send a APDU_FAILURE message when there is a failed SEND message available to the RECEIVE function.
     */
    RMF_PODSRV_EVENT_SEND_APDU_FAILURE,
    RMF_PODSRV_EVENT_RESOURCE_AVAILABLE, /* resource has been freed and is available to an interested party */
    RMF_PODSRV_EVENT_SHUTDOWN, /* close down the queue */
    RMF_PODSRV_EVENT_RESET_PENDING, /* A CableCard reset will start shortly
                                   (5 seconds or more). POD resource use should
                                   be discontinued until RMF_PODSRV_EVENT_READY
                                   is received. */
    //Make this as the last enum if there are new ones introduced. 
    RMF_PODSRV_EVENT_CCI_CHANGED,	
    RMF_PODSRV_EVENT_DSGIP_ACQUIRED,
    RMF_PODSRV_EVENT_ENTITLEMENT,
    RMF_PODSRV_EVENT_CA_SYSTEM_READY = 0xA000, /* CableCard CA System Ready*/
    RMF_PODSRV_EVENT_INB_FWU_START = 0xB001,
    RMF_PODSRV_EVENT_INB_FWU_COMPLETE = 0xB002,	

    RMF_PODSRV_EVENT_INBAND_TUNE_START = 0xB003,
    RMF_PODSRV_EVENT_INBAND_TUNE_COMPLETE = 0xB004,
    RMF_PODSRV_EVENT_INBAND_TUNE_RELEASE = 0xB005,
} rmf_PODSRVEvent;

/**
 * POD Decrypt Session Events - Events sent by a decrypt session
 *
 * Note: In all cases, only one event is generated by ca_pmt_reply/update.
 * If the program/streams are signaled with different values of CA_enable,
 * the rules corresponding to the event are evaluated in the order listed
 * below (per OCAP 1.1.3 16.2.1.7 Figure 16–1).
 *
 * For all events:
 *   OptionalEventData1: Decrypt session handle (rmf_PodSrvDecryptSessionHandle)
 *   OptionalEventData2: data/act value from mpe_podStartDecrypt (queue registration)
 *   OptionalEventData3: message-specific value
 */
typedef enum
{
    /**
     * One or more streams in the decrypt request cannot be descrambled due to
     * inability to purchase (ca_pmt_reply/update with CA_enable of 0x71).
     *
     * OptionalEventData3 (msb to lsb): ((unused (24 bits) | LTSID (8 bits))
     */
    RMF_PODSRV_DECRYPT_EVENT_CANNOT_DESCRAMBLE_ENTITLEMENT = 0x9271,

    /**
     * One or more streams in the decrypt request cannot be descrambled due to
     * lack of resources (ca_pmt_reply/update with CA_enable of 0x73).
     *
     * OptionalEventData3 (msb to lsb): ((unused (24 bits) | LTSID (8 bits))
     */
    RMF_PODSRV_DECRYPT_EVENT_CANNOT_DESCRAMBLE_RESOURCES = 0x9273,

    /**
     * One or more streams in the decrypt request required MMI interaction
     * for a purchase dialog (ca_pmt_reply/update CA_enable of 0x02) and user
     * interaction is now in progress.
     *
     * OptionalEventData3 (msb to lsb): ((unused (24 bits) | LTSID (8 bits))
     */
    RMF_PODSRV_DECRYPT_EVENT_MMI_PURCHASE_DIALOG = 0x9202,

    /**
     * One or more streams in the decrypt request required MMI interaction
     * for a technical dialog (ca_pmt_reply/update CA_enable of 0x03) and user
     * interaction is now in progress.
     *
     * OptionalEventData3 (msb to lsb): ((unused (24 bits) | LTSID (8 bits))
     */
    RMF_PODSRV_DECRYPT_EVENT_MMI_TECHNICAL_DIALOG = 0x9203,

    /**
     * All streams in the decrypt request can be descrambled (ca_pmt_reply/update
     *  with CA_enable of 0x01).
     * OptionalEventData3 (msb to lsb): ((unused (24 bits) | LTSID (8 bits))
     */
    RMF_PODSRV_DECRYPT_EVENT_FULLY_AUTHORIZED = 0x9201,

   /**
     * The session is terminated. All resources have been released.
     * No more events will be received on the registered queue/ed handler.
     * Only induced as a response to mpe_podStopDecrypt().
     */
    RMF_PODSRV_DECRYPT_EVENT_SESSION_SHUTDOWN = 0x9300,

    /**
     * CableCard removed. The session is terminated and will not recover.
     */
    RMF_PODSRV_DECRYPT_EVENT_POD_REMOVED = 0x9301,

    /**
     * The CableCard resources for this session have been lost to a higher priority session.
     * The session is still active and is awaiting resources. An appropriate
     * rmf_PODSRVDecryptSessionEvent is signaled when resources become available (see OCAP 16.2.1.7).
     */
    RMF_PODSRV_DECRYPT_EVENT_RESOURCE_LOST = 0x9302,
    /**
     * Indicate a CCI (Copy Control Information) change for the decrypt session
     * (see the OpenCable CableCARD Copy Protection 2.0 Specification for details)
     *
     * OptionalEventData3: 3 fields (msb to lsb): (PROGRAM # (16 bits) | CCI (8 bits) | LTSID (8 bits))
     */
    RMF_PODSRV_DECRYPT_EVENT_CCI_STATUS  = 0x9303
} rmf_PODSRVDecryptSessionEvent;


/*
 * Generic feature parameter database structure.  This structure contains
 * the current values of the generic features that are configurable between
 * the CableCard device and the Host.  It does not represent all of the
 * feature parameter messages possible within SCTE28 Table 8.12-I, but does
 * include the parameters that are configurable by the CableCard and/or
 * and OCAP application.
 */
typedef struct _rmf_PodmgrFeatureParams
{
    /* RF Output Channel data. */
    uint8_t rf_output_channel[2]; /* RF output channel number & UI enable/disable flag. */

    /* Parental Control PIN. */
    uint8_t *pc_pin; /* Parental control PIN =
     * {
     *    pc_pin_length - 8 bits,
     *    for (i=0; i < pc_pin_length; ++i)
     *      pc_pin - 8 bits
     * }
     */
    /* Parental Control Settings. */
    uint8_t *pc_setting; /* Channel settings =
     * {
     *    p_c_factory_reset - 8 bits
     *    p_c_channel_count - 16 bits,
     *    for (i=0; i < p_c_channel_count; ++i) {
     *      reserved - 4 bits
     *      major_channel_number - 10 bits
     *      minor_channel_number - 10 bits
     * };
     */

    /* IPPV PIN */
    uint8_t *ippv_pin; /* IPPV PIN =
     * {
     *    ippv_pin_length - 8 bits
     *    for (i=0; i < ippv_pin_length; ++i)
     *      ippv_pin_chr - 8 bits
     * }
     */
    /* Time zone. */
    uint8_t time_zone_offset[2]; /* Time zone offset in minutes from GMT. */

    /* Daylight savings. */
    uint8_t daylight_savings[10]; /* Daylight savings data =
     * {
     *     daylight_savings_control  -- 8 bits
     *     if(daylight_savings_control == 0x02)
     *     {
     *         daylight_savings_delta_minutes  -- 8 bits
     *         daylight_savings_entry_time (GPS secs) -- 32 bits
     *         daylight_savings_exit_time (GPS secs) -- 32 bits
     *     }
     * }
     */

    /* AC outlet control. */
    uint8_t ac_outlet_ctrl;

    /* Language control. */
    uint8_t language_ctrl[3]; /* ISO 639 3-byte language code. */

    /* Ratings region code. */
    uint8_t ratings_region; /* Region identifier. */

    /* Reset PIN */
    uint8_t reset_pin_ctrl; /* Reset pin control indicator. */

    /* Cable URLs */
    uint8_t cable_urls_length; /* Total size of cable URL data. */
    uint8_t *cable_urls; /* Cable URLs =
     * {
     *     number_of_urls - 8 bits
     *     for (i=0; i < number_of_urls; ++i) {
     *       url_type - 8 bits
     *       url_length - 8 bits
     *       for (i=0; i < url_length; ++i)
     *         url_chr
     *     }
     * }
     */

    /* EA location code */
    uint8_t ea_location[3]; /* EA location code =
     * {
     *     state_code - 8 bits
     *     county_subdivision - 4 bits
     *     reserved - 2 bits
     *     county_code - 10 bits
     * }
     */

    /* VCT ID */
    uint8_t vct_id[2]; /* VCT ID */

    /* Turn-on Channel */
    uint8_t turn_on_channel[2]; /* Virtual channel to be tuned on power-up =
     * {
     *     reserved - 3 bits (set to zero)
     *     channel_defined - 1 bit
     *     if (channel_defined == 1) {
     *         turn_on_channel - 12 bits
     *     } else {
     *         reserved - 12 bits
     *     }
     * }
     */

    /* Terminal Association */
    uint8_t *term_assoc; /* Terminal association =
     * {
     *     identifier_length - 16 bits
     *     for (i=0; i < identifier_length; ++i) {
     *       term_assoc_identifier - 8 bits
     *     }
     * }
     */

    /* Common download group ID */
    uint8_t cdl_group_id[2]; /* Common download group ID assignment */

    /* Zip Code */
    uint8_t *zip_code; /* Zip Code =
     * {
     *     zip_code_length - 16 bits
     *     for (i=0; i < zip_code_length; ++i) {
     *       zip_code_byte - 8 bits
     *     }
     * }
     */

} rmf_PodmgrFeatureParams;


typedef enum
{
    RMF_PODMGR_DECRYPT_STATE_NOT_SELECTED = 0,
    RMF_PODMGR_DECRYPT_STATE_ISSUED_QUERY,
    RMF_PODMGR_DECRYPT_STATE_ISSUED_MMI,
    RMF_PODMGR_DECRYPT_STATE_DESCRAMBLING,
    RMF_PODMGR_DECRYPT_STATE_FAILED_DESCRAMBLING
} RMF_PODMGRDecryptState;

typedef enum
{
    RMF_PODMGR_DECRYPT_REQUEST_STATE_UNKNOWN = 0,
    RMF_PODMGR_DECRYPT_REQUEST_STATE_ACTIVE,
    RMF_PODMGR_DECRYPT_REQUEST_STATE_WAITING_FOR_RESOURCES
} rmf_PODMGRDecryptRequestState;

/* POD Params (not generic features) */
typedef enum
{
    RMF_PODMGR_PARAM_ID_MAX_NUM_ES = 1,
    RMF_PODMGR_PARAM_ID_MAX_NUM_PROGRAMS,
    RMF_PODMGR_PARAM_ID_MAX_NUM_TS
} rmf_PODMGRParamId;


typedef enum _rmf_PODMGR_InbandFWU_ModulationMode
{
	RMF_POD_MODULATION_QAM64 = 8,
	RMF_POD_MODULATION_QAM256 = 16,
}rmf_PODMGR_InbandFWU_ModulationMode;

/**
 * Structure representing data from a feature_list() response message.
 * See SCTE28 Table 8.12-E.
 */
typedef struct _rmf_PodmgrFeatures
{
    uint8_t number; /* Number of generic features identifiers. */
    uint8_t* featureIds; /* Array of feature identifiers of supported features. */
} rmf_PodmgrFeatures;

/* handle to Decrypt session structure */
typedef uint32_t rmf_PodSrvDecryptSessionHandle ;

#if 0
typedef struct _rmf_MediaPID
{
    rmf_SiElemStreamType pidType; /* PID type: audio, video, etc. */
    uint32_t pid; /* PID value. */
} rmf_MediaPID;
#endif

typedef struct _rmf_PodmgrStreamDecryptInfo
{
    uint16_t pid; 		// PID to query (filled in by the caller)
    uint16_t status; 	// ca_enable value (0x1,0x2,0x3,0x71,0x73) or 0 if no status provided (filled in by MPE)
} rmf_PodmgrStreamDecryptInfo;

typedef struct
{
        uint32_t prgNo;
        uint32_t srcId;
        uint16_t ecmPid;
        uint8_t ecmPidFound;
        uint8_t caDescFound;            /* encrypted or clear  */
        uint32_t noES;
        rmf_PodmgrStreamDecryptInfo *pESInfo;
        uint16_t ProgramInfoLen;
        uint8_t *pProgramInfo;
        uint16_t streamInfoLen;		
        uint8_t *pStreamInfo;
        uint16_t aComps[512];
	 uint16_t CAPMTCmdIdIndex;		
} parsedDescriptors;



typedef struct _decryptRequestParams
{
	uint32_t tunerId;
	uint32_t qamContext;
	uint8_t ltsId; 
	uint32_t numPids;
	rmf_MediaPID *pids;
	parsedDescriptors *pDescriptor; 
	uint32_t priority;
	rmf_osal_Bool mmiEnable;
} decryptRequestParams;

typedef struct _qamEventData_t
{
    uint32_t cci;
    uint32_t qamContext;
    uint32_t handle;
} qamEventData_t;


/****************************************************************************************
 *
 * POD FUNCTIONS
 *
 ***************************************************************************************/

/**
 * <i>rmf_podmgrInit()</i>
 *
 * Initialize interface to POD-HOST stack.  Depending on the underlying OS-specific
 * support this initialization process will likely populate the internal to MPE
 * POD database used to cache information communicated between the Host and the
 * POD.  The rmf_PODDatabase structure is used to maintain the POD data.  The
 * actual mechanics of how the database is maintained is an implementation-specific
 * issues that is dependent upon the functionality of the target POD-Host interface.
 */
void podmgrInit( void );

/**
 * The rmf_podmgrCASConnect() function shall establish a connection between a private
 * Host application and the POD Conditional Access Support (CAS) resource.  It is the MPEOS call's responsibility
 * determine the correct resource ID based upon whether the card is M or S mode.
 *
 * @param sessionId points to a location where the session ID can be returned. The session
 *          ID is implementation dependent and represents the CAS session to the POD application.
 * @param version a pointer to the location where the implementation will store the supported
 *                          version of the Conditional Access Support resource
 *
 * @return Upon successful completion, this function shall return RMF_SUCCESS. Otherwise,
 *          one of the errors below is returned.
 * <ul>
 * <li>     RMF_OSAL_ENOMEM - There was insufficient memory available to complete the operation.
 * <li>     RMF_OSAL_EINVAL - One or more parameters were out of range or unusable.
 * </ul>
 */
rmf_Error rmf_podmgrCASConnect(uint32_t *sessionId, uint16_t *version);

/**
 * The rmf_podmgrCASClose() function provides an optional API for systems that may
 * require additional work to maintain session resources when an application unregisters
 * its handler from POD.  The implementation of this function may
 * need to: 1) update internal implementation resources, 2) make an OS API call to
 * allow the OS to update session related resources or 3) do nothing since it's
 * entirely possible that the sessions can be maintained as "connected" upon
 * deregistration and simply reused if the same host or another host application makes a
 * later request to connect to the CAS application.
 *
 * @param sessionId the session identifier of the target CAS session.
 *
 * @return Upon successful completion, this function shall return RMF_SUCCESS. Otherwise,
 *          one of the errors below is returned.
 * <ul>
 * <li>     RMF_OSAL_EINVAL - One or more parameters were out of range or unusable.
 * </ul>
 */
rmf_Error rmf_podmgrCASClose(uint32_t sessionId);

/**
 * The rmf_podmgrSASConnect() function shall establish a connection between a private
 * Host application and the corresponding POD Module vendor-specific application.
 *
 * @param appId specifies a unique identifier of the private Host application.
 * @param sessionId points to a location where the session ID can be returned. The session
 *          ID is implementation dependent and represents the SAS session to the POD application.
 * @param version a pointer to the location where the implementation will store the supported
 *                          version of the Conditional Access Support resource
 * @return Upon successful completion, this function shall return RMF_SUCCESS. Otherwise,
 *          one of the errors below is returned.
 * <ul>
 * <li>     RMF_OSAL_ENOMEM - There was insufficient memory available to complete the operation.
 * <li>     RMF_OSAL_EINVAL - One or more parameters were out of range or unusable.
 * </ul>
 */
rmf_Error rmf_podmgrSASConnect(uint8_t *appId, uint32_t *sessionId, uint16_t *version);

/**
 * The rmf_podmgrSASClose() function provides an optional API for systems that may
 * require additional work to maintain session resources when an application unregisters
 * its handler from an SAS application.  The implementation of this function may
 * need to: 1) update internal implementation resouces, 2) make an OS API call to
 * allow the OS to update session related resources or 3) do nothing since it's
 * entirely possible that the sessions can be maintained as "connected" upon
 * deregistration and simply reused if the same host or another host application makes a
 * later request to connect to the SAS application.
 *
 * @param sessionId the session identifier of the target SAS session.
 *
 * @return Upon successful completion, this function shall return RMF_SUCCESS. Otherwise,
 *          one of the errors below is returned.
 * <ul>
 * <li>     RMF_OSAL_EINVAL - One or more parameters were out of range or unusable.
 * </ul>
 */
rmf_Error rmf_podmgrSASClose(uint32_t sessionId);

/**
 * The rmf_podmgrMMIConnect() function shall establish a connection with the MMI
 * resource on the POD device.
 *
 * @param sessionId the session identifier of the target MMI session.
 * @param version a pointer to the location where the implementation will store the supported
 *                          version of the Man-Machine Interface resource
 *
 * @return Upon successful completion, this function shall return RMF_SUCCESS. Otherwise,
 *          one of the errors below is returned.
 * <ul>
 * <li>     RMF_OSAL_ENOMEM - There was insufficient memory available to complete the operation.
 * <li>     RMF_OSAL_EINVAL - One or more parameters were out of range or unusable.
 * </ul>
 */
rmf_Error rmf_podmgrMMIConnect(uint32_t *sessionId, uint16_t *version);

/**
 * The rmf_podmgrMMIClose() function provides an optional API for systems that may
 * require additional work to maintain MMI resources when an application unregisters
 * its MMI handler from the MMI POD application.  The implementation of this function may
 * need to: 1) update internal implementation resouces, 2) make an OS API call to
 * allow the OS to update MMI session related resources or 3) do nothing since it's
 * entirely possible that the MMI sessions can be maintained as "connected" upon
 * deregistration.
 *
 * @return Upon successful completion, this function shall return RMF_SUCCESS. Otherwise,
 *          one of the errors below is returned.
 * <ul>
 * <li>     RMF_OSAL_EINVAL - One or more parameters were out of range or unusable.
 * </ul>
 */
rmf_Error rmf_podmgrMMIClose(void);

/**
 * The rmf_podmgrAIConnect() function shall establish a connection with the
 * Application Information resource on the POD device.
 *
 * @param sessionId the session identifier of the target application information session.
 * @param version a pointer to the location where the implementation will store the supported
 *                          version of the Application Information resource
 *
 * @return Upon successful completion, this function shall return RMF_SUCCESS. Otherwise,
 *          one of the errors below is returned.
 * <ul>
 * <li>     RMF_OSAL_ENOMEM - There was insufficient memory available to complete the operation.
 * <li>     RMF_OSAL_EINVAL - One or more parameters were out of range or unusable.
 * </ul>
 */
rmf_Error rmf_podmgrAIConnect(uint32_t *sessionId, uint16_t *version);

/**
 * The rmf_podmgrReleaseAPDU() function
 *
 * This will release an APDU retrieved via podReceiveAPDU()
 *
 * @param apdu the APDU pointer retrieved via podReceiveAPDU()
 * @return Upon successful completion, this function shall return
 * <ul>
 * <li>     RMF_OSAL_EINVAL - The apdu pointer was invalid.
 * <li>     RMF_SUCCESS - The apdu was successfully deallocated.
 * </ul>
 */
rmf_Error rmf_podmgrReleaseAPDU(uint8_t *apdu);

/**
 * The rmf_podmgrSendAPDU() function sends an APDU packet.
 *
 * @param sessionId is the native handle for the SAS session for the APDU.
 * @param apduTag APDU identifier
 * @param length is the length of the data buffer portion of the APDU
 * @param apdu is a pointer to the APDU data buffer
 *
 * @return Upon successful completion, this function shall return RMF_SUCCESS. Otherwise,
 *          one of the errors below is returned.
 * <ul>
 * <li>     RMF_OSAL_ENOMEM - There was insufficient memory available to complete the operation.
 * <li>     RMF_OSAL_EINVAL - One or more parameters were out of range or unusable.
 * </ul>
 */
rmf_Error rmf_podmgrSendAPDU(uint32_t sessionId, uint32_t apduTag,
        uint32_t length, uint8_t *apdu);

/**
 * <i>rmf_podmgrGetCCIBits<i/>
 *
 * Get the Copy Control Information for the specified service.
 *
 * @param frequency the frequency of the transport stream on which the desired
 *        service is carried
 * @param programNumber the program number of the desired service
 * @param tsId the transport stream ID associated with the transport stream on
 *        which the desired service is carried
 * @param cciBits output pointer where the requested CCI bits are returned
 *
 * @return RMF_SUCCESS if the feature parameter was successfully retrieved, RMF_OSAL_EINVAL
 * if the service information or the return parameter is invalid, RMF_OSAL_ENODATA if the
 * specified service is not tuned or CCI data is not available
 */
rmf_Error podmgrGetCCIBits(rmf_PodSrvDecryptSessionHandle sessionHandle, uint8_t *cciBits);

/**
 * The rmf_podmgrReceiveAPDU() function retrieves the next available APDU from the POD.
 * This function also will receive "copy" APDU's that failed to be sent to the POD so that
 * the APDU can be returned to the original sender to be notified of the failure.
 *
 * @param sessionId is a pointer for returning the native session handle associated with
 *          the received APDU.
 * @param apdu the next available APDU from the POD
 * @param len the length of the APDU in bytes
 * @return Upon successful completion, this function shall return RMF_SUCCESS. Otherwise,
 *          one of the errors below is returned.
 * <ul>
 * <li>     RMF_OSAL_ENOMEM - There was insufficient memory available to complete the operation.
 * <li>     RMF_OSAL_EINVAL - One or more parameters were out of range or unusable.
 * <li>     RMF_OSAL_ENODATA - Indicates that the APDU received is actually the last APDU that
 *                       failed to be sent.
 * </ul>
 */
rmf_Error rmf_podmgrReceiveAPDU(uint32_t *sessionId, uint8_t **apdu,
        int32_t *len);


/**
 * <i>rmf_podmgrStartDecrypt()</i>
 *
 * Tells the CableCARD to start decrypting the streams represented by decodeRequest
 *
 * @param decodeRequest   decode request params
 * @param queueId         the queue to which decrypt session events should be sent
 * @param act             usually the edHandle (if caller is at JNI level)
 * @param handle          a session handle used to represent the session
 *
 * @return RMF_SUCCESS, valid session handle      If a session is successfully created
 *         RMF_SUCCESS, NULL session handle       If analog or clear channel
 *
 */

rmf_Error podmgrStartDecrypt(
        decryptRequestParams* decryptRequestPtr,
        rmf_osal_eventqueue_handle_t queueId, 
        rmf_PodSrvDecryptSessionHandle* sessionHandlePtr);

/**
 * <i>rmf_podmgrStopDecrypt()</i>
 *
 * Tells the POD to stop decrypting the stream represented by the sessionHandle.  When this method returns, the
 * session represented by the sessionHandle is no longer valid.  Since this initiated by caller, no session lost or
 * other events are sent.
 *
 * @param sessionHandle  a session handle used to represent the session
 *
 * @return RMF_SUCCESS  if the unregistration process was successful.
 *         RMF_OSAL_EINVAL   if the handle does not represent an existing session or is NULL
 */

rmf_Error podmgrStopDecrypt(rmf_PodSrvDecryptSessionHandle sessionHandle);

/**
 * <i>rmf_podmgrGetDecryptStreamStatus()</i>
 *
 * Gets the per PID stream authorization status for decrypt session represented by the
 * decrypt session handle (returned from rmf_podmgrStartDecrypt).
 *
 * @param handle   session handle returned by rmf_podmgrStartDecrypt
 * @param numStreams the number of streams the info is being requested for
 * @param streamInfo  array of streamInfo data structures, to be filled in by the PODMgr
 *
 * @return RMF_SUCCESS if the handle represents a session.
 */
rmf_Error rmf_podmgrGetDecryptStreamStatus(
        rmf_PodSrvDecryptSessionHandle handle, 
        uint8_t numStreams, rmf_PodmgrStreamDecryptInfo streamInfo[]);

/**
 * <i>mpeos_podmgrGetFeatures</i>
 *
 * Get the POD's generic features list.  Note the generic features list
 * can be updated by the HOST-POD interface asynchronously and since there
 * is no mechanism in the native POD support APIs to get notification of this
 * asynchronous update, the list must be refreshed from the interface every
 * the list is requested. The last list acquired will be buffered within the
 * POD database, but it may not be up to date.  Therefore, the MPEOS layer will
 * release any previous list and refresh with a new one whenever a request for
 * the list is made.
 *
 * @param features is a pointer for returning a pointer to the current features
 *                 list.
 *
 * @return RMF_SUCCESS if the features list was successfully acquired.
 */
rmf_Error rmf_podmgrGetFeatures(rmf_PodmgrFeatures **features);

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
rmf_Error rmf_podmgrGetFeatureParam(uint32_t featureId, void * param, uint32_t * length);
		
/**
 * <i>rmf_podmgrSetFeatureParam<i/>
 *
 * Set the specified POD-Host feature parameter value.  The feature change
 * request will be passed on to the POD-Host interface and based on the
 * resulting acceptance or denial the internal MPE POD infromation database
 * will be updated (or not) to reflect the new feature parameter value.
 *
 * @param featureId is the identifier of the target feature.
 * @param param is a pointer to the byte array containing the new value.
 *
 * @return RMF_SUCCESS if the prosed value was accepted by the POD.
 */
rmf_Error rmf_podmgrSetFeatureParam(uint32_t featureId, void *param,
        uint32_t length);


rmf_Error podmgrRegisterEvents( rmf_osal_eventqueue_handle_t queueId );
rmf_Error podmgrUnRegisterEvents(rmf_osal_eventqueue_handle_t queueId);

rmf_Error podmgrIsReady(void); 

int podmgrConfigCipher(unsigned char ltsid,unsigned short PrgNum,unsigned long *decodePid,unsigned long numpids);

rmf_Error podImplGetCaSystemId(unsigned short * pCaSystemId);

void podmgrStartHomingDownload(uint32_t  ltsId, int32_t status);
rmf_Error podmgrStartInbandTune(uint32_t  ltsId, int32_t status);
rmf_Error podmgrRegisterHomingQueue(rmf_osal_eventqueue_handle_t eventQueueID, void* act);

rmf_Error podmgrGetManufacturerId( uint16_t *pManufacturerId );
rmf_Error podmgrGetCardVersion( uint16_t *pVersionNo );
rmf_osal_Bool  podmgrDsgEcmIsOperational();
rmf_osal_Bool podmgrSASResetStat(rmf_osal_Bool Set, rmf_osal_Bool *pStatus );
#ifdef __cplusplus
}
#endif
#endif
