/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2013 RDK Management
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
 

/** @defgroup POD_MGR POD Manager
 * POD manager decrypts a given program, provides the copy control information (CCI) and authorization for a particular channel.
 * POD Manager supports the following functionalities:
 * - Initialization
 * - Tunnel data acquisition
 * - Channel Decryption
 * - CANH (Conditional Access Network Handler) Support
 * - Common Download
 *
 * @par Process oriented architecture details
 *
 * POD Manager works by involving interaction of following processes:
 * - runPod (All card manager functionalities are bundled under this process).
 * - podServer (Interface for all the card manager functionalities).
 * - podclient (Abstraction module in each process to interact with the podServer).
 *
 * @image html podm_process.png
 *
 *
 * @par API Interaction Sequence Diagram
 *
 * \mscfile podm_api.msc
 *
 * @defgroup POD_API POD Manager Generic API list
 * In General the POD Manager APIs can be categorized for following service types.
 * @n
 * @par Initialization APIs
 * - Initialize, un-initialize and getting current status of initialization.
 * @par Program Decryption APIs
 * - Start/stop the decryption and setting of the cipher
 * @par OOB SI abstraction APIs
 * - Getting Program information
 * @par Event Delivery APIs
 * - Generic event types related to card status such as card inserted, removed, card reset etc.
 * - In-band Firmware download start/end notifications.
 * - Decrypt related events such as program is authorized/not authorized, CCI notification etc.
 * @ingroup POD_MGR
 *
 * @defgroup POD_DS POD Manager Data Types
 * Described the different data types used in POD Manager.
 * @ingroup POD_MGR
 */

#include <rmf_inbsi_common.h>
#include <rmf_qamsrc_common.h>

#ifndef PODCLIENT_POD_H
#define PODCLIENT_POD_H
//#ifdef __cplusplus
//extern "C"
//{
//#endif
#include <rmf_osal_types.h>
#include <rmf_osal_error.h>
#include <rmf_osal_event.h>

#define RMF_PODMGR_INB_FWU_TUNING_ACCEPTED  0x00// Tuning accepted
#define RMF_PODMGR_INB_FWU_INVALID_FREQUENCY 0x01// Invalid frequency (Host does not support this frequency)
#define RMF_PODMGR_INB_FWU_INVALID_MODULATION 0x02// Invalid modulation (Host does not support this modulation type)
#define RMF_PODMGR_INB_FWU_TUNE_FAILURE 0x03// Hardware failure (Host has hardware failure)
#define RMF_PODMGR_INB_FWU_TUNER_BUSY 0x04// Tuner busy (Host is not relinquishing control of inband tuner)

/**
 * @brief List of Events used in POD Manager
 * @ingroup POD_DS
 */
typedef enum
{
/*	
    RMF_PODMGR_CORE_Card_Detected = 0x1770,	
    RMF_PODMGR_CORE_Card_Removed,
    RMF_PODMGR_CORE_Card_Ready,	
    RMF_PODMGR_CORE_CP_CCI_Changed, // Card Manager received the CCI it posts the CCI_data with ltsid
    RMF_PODMGR_CORE_CP_CCI_Error, // Card Manager  CP Authentication Error posts  with  ltsid	
    RMF_PODMGR_CORE_DSG_Cable_Card_VCT_ID,
    RMF_PODMGR_CORE_Generic_Feature_Changed,
    RMF_PODMGR_CORE_DSG_UCID,
    RMF_PODMGR_CORE_Device_In_Non_DSG_Mode,
    RMF_PODMGR_CORE_CA_Encrypt_Flag, // Card Manager Posts this event for the selected program with CardManagerCAEncryptionData_t
    RMF_PODMGR_CORE_CA_Pmt_Reply,// Card Manager Posts this event for the selected program with CardManagerCAPmtReply_t
    RMF_PODMGR_CORE_DSG_IP_ACQUIRED,
    RMF_PODMGR_CORE_CP_DHKey_Changed,
    RMF_PODMGR_CORE_CA_Update,		
*/   
    RMF_PODMGR_CORE_DSG_Entering_DSG_Mode = 0x177E,
    RMF_PODMGR_CORE_DSG_Leaving_DSG_Mode,
    RMF_PODMGR_CORE_DOCSIS_TLV_217,
    RMF_PODMGR_CORE_DSG_DownStream_Scan_Failed,
    RMF_PODMGR_CORE_POD_IP_ACQUIRED,
    RMF_PODMGR_CORE_Card_Mib_Access_Root_OID,
    RMF_PODMGR_CORE_Card_Mib_Access_Snmp_Reply,
    RMF_PODMGR_CORE_Card_Mib_Access_Shutdown,
    RMF_PODMGR_CORE_OOB_Downstream_Acquired,
    RMF_PODMGR_CORE_DSG_Operational_Mode,
    RMF_PODMGR_CORE_DSG_Downstream_Acquired,
    RMF_PODMGR_CORE_VCT_Channel_Map_Acquired,
    RMF_PODMGR_CORE_CP_Res_Opened, //!<Copy Protection Resource opened
    RMF_PODMGR_CORE_Host_Auth_Sent,//!<Host AuthKey sent
    RMF_PODMGR_CORE_Bnd_Fail_Card_Reasons,//!<Binding Failure: Card reasons
    RMF_PODMGR_CORE_Bnd_Fail_Invalid_Host_Cert,//!<Binding Failure: Invalid Host Cert
    RMF_PODMGR_CORE_Bnd_Fail_Invalid_Host_Sig, //!<Binding Failure: Invalid Host Signature
    RMF_PODMGR_CORE_Bnd_Fail_Invalid_Host_AuthKey, //!<Binding Failure: Invalid Host AuthKey
    RMF_PODMGR_CORE_Bnd_Fail_Other, //!<Binding Failure: Other
    RMF_PODMGR_CORE_Card_Val_Error_Val_Revoked, //!<Card Validation Error: Validation revoked
    RMF_PODMGR_CORE_Bnd_Fail_Incompatible_Module, //!<Binding Failure. Incompatible module
    RMF_PODMGR_CORE_Bnd_Comp_Card_Host_Validated, //!<Binding Complete: Card/Host Validated
    RMF_PODMGR_CORE_CP_Initiated, //!<Copy Protection initiated
    RMF_PODMGR_CORE_Card_Image_DL_Complete, //!<Card Image Download Complete
    RMF_PODMGR_CORE_CDL_Host_Img_DL_Cmplete, //!<Host Image Download Complete
    RMF_PODMGR_CORE_CDL_CVT_Error, //!<Common Download CVT Error
    RMF_PODMGR_CORE_CDL_Host_Img_DL_Error, //!<Host Image Download Error, <P1>

    RMF_PODMGR_EVENT_INSERTED = 0x9003, //!< Cable Card inserted into slot

    RMF_PODMGR_EVENT_READY = 0x9004, //!< Cable Card is ready for use

    RMF_PODMGR_EVENT_REMOVED = 0x9005, //!< Cable Card removed from slot
    /** A CableCard reset will start shortly. POD resource use should be discontinued until RMF_PODSRV_EVENT_READY is received. */
    RMF_PODMGR_EVENT_RESET_PENDING = 0x900A,

    RMF_PODMGR_EVENT_DSGIP_ACQUIRED = 0x900C, //!< DOCSIS IP Acquired
    RMF_PODMGR_EVENT_ENTITLEMENT = 0x900D, //!< Entitlement Events received


    /**
     * One or more streams in the decrypt request cannot be descrambled due to
     * inability to purchase (ca_pmt_reply/update with CA_enable of 0x71).
     *
     * OptionalEventData3 (msb to lsb): ((unused (24 bits) | LTSID (8 bits))
     */
    RMF_PODMGR_DECRYPT_EVENT_CANNOT_DESCRAMBLE_ENTITLEMENT = 0x9271,

    /**
     * One or more streams in the decrypt request cannot be descrambled due to
     * lack of resources (ca_pmt_reply/update with CA_enable of 0x73).
     *
     * OptionalEventData3 (msb to lsb): ((unused (24 bits) | LTSID (8 bits))
     */
    RMF_PODMGR_DECRYPT_EVENT_CANNOT_DESCRAMBLE_RESOURCES = 0x9273,

    /**
     * One or more streams in the decrypt request required MMI interaction
     * for a purchase dialog (ca_pmt_reply/update CA_enable of 0x02) and user
     * interaction is now in progress.
     *
     * OptionalEventData3 (msb to lsb): ((unused (24 bits) | LTSID (8 bits))
     */
    RMF_PODMGR_DECRYPT_EVENT_MMI_PURCHASE_DIALOG = 0x9202,

    /**
     * One or more streams in the decrypt request required MMI interaction
     * for a technical dialog (ca_pmt_reply/update CA_enable of 0x03) and user
     * interaction is now in progress.
     *
     * OptionalEventData3 (msb to lsb): ((unused (24 bits) | LTSID (8 bits))
     */
    RMF_PODMGR_DECRYPT_EVENT_MMI_TECHNICAL_DIALOG = 0x9203,

    /**
     * All streams in the decrypt request can be descrambled (ca_pmt_reply/update
     *  with CA_enable of 0x01).
     * OptionalEventData3 (msb to lsb): ((unused (24 bits) | LTSID (8 bits))
     */
    RMF_PODMGR_DECRYPT_EVENT_FULLY_AUTHORIZED = 0x9201,

   /**
     * The session is terminated. All resources have been released.
     * No more events will be received on the registered queue/ed handler.
     * Only induced as a response to mpe_podStopDecrypt().
     */
    RMF_PODMGR_DECRYPT_EVENT_SESSION_SHUTDOWN = 0x9300,

    /**
     * CableCard removed. The session is terminated and will not recover.
     */
    RMF_PODMGR_DECRYPT_EVENT_POD_REMOVED = 0x9301,

    /**
     * The CableCard resources for this session have been lost to a higher priority session.
     * The session is still active and is awaiting resources. An appropriate
     * rmf_PODSRVDecryptSessionEvent is signaled when resources become available (see OCAP 16.2.1.7).
     */
    RMF_PODMGR_DECRYPT_EVENT_RESOURCE_LOST = 0x9302,
    /**
     * Indicate a CCI (Copy Control Information) change for the decrypt session
     * (see the OpenCable CableCARD Copy Protection 2.0 Specification for details)
     *
     * OptionalEventData3: 3 fields (msb to lsb): (PROGRAM # (16 bits) | CCI (8 bits) | LTSID (8 bits))
     */
    RMF_PODMGR_DECRYPT_EVENT_CCI_STATUS  = 0x9303,

	RMF_PODMGR_EVENT_CA_SYSTEM_READY = 0xA000,
    RMF_PODMGR_BOOT_DIAG_EVENT_CARD = 0xA001,
    RMF_PODMGR_BOOT_DIAG_EVENT_CARD_CA,
    RMF_PODMGR_BOOT_DIAG_EVENT_CARD_CP,
    RMF_PODMGR_BOOT_DIAG_EVENT_CARD_OOB,
    RMF_PODMGR_BOOT_DIAG_EVENT_CARD_DSG_ONEWAY,
    RMF_PODMGR_BOOT_DIAG_EVENT_CARD_DSG_TWOWAY,
    RMF_PODMGR_BOOT_DIAG_EVENT_CARD_FWDNLD,
    RMF_PODMGR_BOOT_DIAG_EVENT_ESTB_IP,
    RMF_PODMGR_BOOT_DIAG_EVENT_CDL,
    RMF_PODMGR_BOOT_DIAG_EVENT_OCAP,
    RMF_PODMGR_BOOT_DIAG_EVENT_OCAP_XAIT,
    RMF_PODMGR_BOOT_DIAG_EVENT_OCAP_MONITOR,
    RMF_PODMGR_BOOT_DIAG_EVENT_OCAP_AUTO,
    RMF_PODMGR_BOOT_DIAG_EVENT_SI,
    RMF_PODMGR_BOOT_DIAG_EVENT_GENMSG,
    RMF_PODMGR_BOOT_DIAG_EVENT_TRU2WAY_PROXY,
    RMF_PODMGR_BOOT_DIAG_EVENT_XRE,
    RMF_PODMGR_BOOT_DIAG_EVENT_FLASH_WRITE,
    RMF_PODMGR_BOOT_DIAG_EVENT_MAX,

    RMF_PODMGR_EVENT_INB_FWU_START = 0xB001, //!<Firmware update started.

    RMF_PODMGR_EVENT_INB_FWU_COMPLETE = 0xB002, //!< Firmware update completed.
#ifdef USE_IPDIRECT_UNICAST
    RMF_PODMGR_EVENT_INB_TUNE_START = 0xB003,
    RMF_PODMGR_EVENT_INB_TUNE_COMPLETE = 0xB004,
    RMF_PODMGR_EVENT_INB_TUNE_RELEASE = 0xB005,
    RMF_PODMGR_EVENT_INB_TUNE_STATUS = 0xB006
#endif

} rmf_PODMgrEvent;

/** @} */

typedef enum
{
    RMF_PODMGR_DIAG_MESG_CODE_INIT,
    RMF_PODMGR_DIAG_MESG_CODE_START,
    RMF_PODMGR_DIAG_MESG_CODE_PROGRESS,
    RMF_PODMGR_DIAG_MESG_CODE_COMPLETE,
    RMF_PODMGR_DIAG_MESG_CODE_ERROR,
    RMF_PODMGR_DIAG_MESG_CODE_MSG
}rmf_PODMgrDiagMsgCode;


typedef enum
{
	POD_EID_GENERAL = 0,
    	POD_EID_DISCONNECT = 1,
    	POD_EID_DVR = 2,
	POD_EID_HD = 3,
	POD_EID_MDVR = 4,
	POD_EID_VOD = 5,
	POD_EID_MAX_NO,
	/* Do not add here */
}PODEntitlementAuthType;

typedef struct 
{
	uint32_t eId;
	uint8_t isAuthorized;
	uint32_t extendedInfo;
}PODEntitlementAuthEvent;

/**
 * @brief This structure is used for expanding the attributes required for a decryption request.
 * @ingroup POD_DS
 */
typedef struct _rmf_PodmgrDecryptRequestParams
{
	rmf_osal_Bool bClear; //!< Boolean value identifying whether it is a clear (un-encrypted) stream.
	uint32_t qamContext; //!< The current context of QAM source for this request.
	uint32_t tunerId; //!< ID of the Tuner to be used for this request.
	uint8_t ltsId; //!< The Local Transport Stream Id is used for decryption of a program.
	uint32_t srcId; //!< Source Id of a program.
	uint32_t prgNo; //!< The requested Program Number.
	uint32_t numPids; //!< Number of Audio/Video/Data PIDS associated with the program.

	/** Array of PID type and their values. The PID type field contains valid values for the elementary stream_type
	  * field in the PMT. For more information, please refer ISO/IEC 13818-1: 2000 (E) Pg.no: 66-80
	*/
	rmf_MediaPID *pids;

	/** Elementary stream list is a linked list of elementary streams with in a given service.
          * It also contains service component specific information such as the stream type, PID value,
          * associated language, descriptor length etc.
          */
	rmf_SiElementaryStreamList *pESList;

	/** Represents a linked list of MPEG-2 descriptors. (Contains an 8 bit tag that identifies each descriptor,
	  * 8 bit descriptor_length field specifying the number of bytes of the descriptor immediately following the
	  * descriptor_length field followed by the data contained within this descriptor and a pointer to the next
	  * descriptor in the list.
	  */
	rmf_SiMpeg2DescriptorList* pMpeg2DescList; 

	/** Indicates the priority of the request, when the incoming request has a higher priority
	  * than an existing request the lower priority request will be pre-empted and placed in a waiting for
	  * resources state and the new request will take its place.
	  */
	uint32_t priority;

	rmf_osal_Bool mmiEnable; //!< Boolean value specifying that whether MMI resource is enabled.
} rmf_PodmgrDecryptRequestParams;

typedef uint32_t rmf_PodmgrDecryptSessionHandle ;


/****************************************************************************************
 *
 * POD FUNCTIONS
 *
 ***************************************************************************************/
/**
 * @ingroup POD_API
 * @{
 */

/**
 * @brief Initialize interface to POD-HOST stack.
 *
 * Depending on the underlying OS-specific support this initialization process will likely populate
 * the internal POD database used to cache information communicated between the Host and the POD.
 * The rmf_PODDatabase structure is used to maintain the POD data. The actual mechanics of how the
 * database is maintained is an implementation-specific issues that is dependent upon the functionality
 * of the target POD-Host interface
 *
 * @return None
 */
void rmf_podmgrInit(void);


/**
 * @brief This will invoke a shutdown call on the POD and will de-register event handlers with releasing
 * all resources allocated.
 *
 * @return None.
 */
void rmf_podmgrUnInit(void);

/**
 * @brief This function shall establish a connection between a private Host application and the POD
 * Conditional Access System (CAS) resource.
 *
 * It is the underlying implementation call's responsibility determine the correct resource ID based
 * upon whether the card is M or S mode.
 *
 * @paramp[in] sessionHandle points to a location where the session ID can be returned. The session
 *          ID is implementation dependent and represents the CAS session to the POD application.
 * @param[out] cciBits ouput pointer where the requested CCI bits are returned.
 *
 * @return Upon successful completion, this function shall return RMF_SUCCESS. Otherwise,
 *          one of the errors below is returned.
 * @retval RMF_OSAL_ENOMEM There was insufficient memory available to complete the operation.
 * @retval RMF_OSAL_EINVAL One or more parameters were out of range or unusable.
 */
rmf_Error rmf_podmgrGetCCIBits(rmf_PodmgrDecryptSessionHandle sessionHandle, uint8_t *cciBits);

/**
 * @brief This function is used to inform the CableCARD to start decrypting the streams represented by decodeRequest.
 *
 * @param[in] decodeRequestPtr Decrypt request parameters. See rmf_PodmgrDecryptRequestParams for details.
 * @param[in] queueId The queue to which decrypt session events should be sent. The queue shall be
 * created using rmf_osal_eventqueue_create().
 * @param[out] sessionHandlePtr The output session handle used to represent the session.
 *
 * @return RMF_SUCCESS, valid session handle,  if a session is successfully created.
 *         RMF_SUCCESS, NULL session handle, if analog or clear channel.
 */
rmf_Error rmf_podmgrStartDecrypt(
        rmf_PodmgrDecryptRequestParams* decryptRequestPtr,
        rmf_osal_eventqueue_handle_t queueId, 
        rmf_PodmgrDecryptSessionHandle* sessionHandlePtr);

/**
 * @brief This function is used to inform the POD to stop decrypting the stream represented by the sessionHandle.
 *
 * When this method returns, the session represented by the sessionHandle is no longer valid. Since this
 * initiated by caller, no session lost or other events are sent.
 *
 * @param[in] sessionHandle A session handle used to represent the session.
 *
 * @retval RMF_SUCCESS If the unregistration process was successful.
 * @retval RMF_OSAL_EINVAL If the handle does not represent an existing session or is NULL
 */
rmf_Error rmf_podmgrStopDecrypt(rmf_PodmgrDecryptSessionHandle sessionHandle);

/**
 * @brief Depending upon the underlying implementation, this function will call the appropriate API for
 * registering a module with the event manager for POD events.
 *
 * @param[in] queueId The Event queue identifier. The queue shall be created using rmf_osal_eventqueue_create().
 *
 * @retval Upon successful completion of the call CA_SUCCESS will be returned otherwise appropriate error
 * value has to be returned
 */
rmf_Error rmf_podmgrRegisterEvents(rmf_osal_eventqueue_handle_t queueId);

/**
 * @brief This function will either use an appropriate API for un-registering a module from the event manager for POD events or 
 * use the interprocess communication mechanism for sending an unregister event to the podserver module.
 *
 * @param[in] queueId The Event queue identifier. The queue shall be created using rmf_osal_eventqueue_create().
 */
rmf_Error rmf_podmgrUnRegisterEvents(rmf_osal_eventqueue_handle_t queueId);

/**
 * @brief This function will return the availability of the POD manager for communicating with the calling routine.
 *
 * @return Returns RMF_SUCCESS, if the POD Manager Resource is ready, otherwise an error value is returned.
 */ 
rmf_Error rmf_podmgrIsReady(void); 

/**
 * @brief This API shall take the Local transport stream ID, program number as input and configures the list of PIDs
 * that need to be decrypted by the demux.
 *
 * @param[in] ltsid Local TSID used for decryption.
 * @param[in] PrgNum Program number whose audio/video is to be decrypted.
 * @param[in] decodePid	An array of the PIDs that has to be filtered.
 * @param[in] numpids Number of audio/video PIDs for which decryption keys are to be set.
 */
int rmf_podmgrConfigCipher(unsigned char ltsid,unsigned short PrgNum, rmf_MediaPID *decodePid, unsigned long numpids);


/**
 * @brief This function gets the program information of the specified decimalSrcId from the SI cache.
 *
 * The program information includes frequency, modulation, program number and the service handle.
 *
 * @param[in] decimalSrcId Source Id represented in decimal value
 * @param[out] pInfo A Pointer to the Program information data structure. See rmf_ProgramInfo_t for details.
 *
 * @return Returns RMF_SUCCESS on successful completion, returns an error code otherwise.
 */
rmf_Error rmf_GetProgramInfo( uint32_t decimalSrcId, rmf_ProgramInfo_t *pInfo);

/**
 * @brief This function gets the program information of the specified Virtual Channel Number from the SI cache.
 *
 * The program information includes frequency, modulation, program number and the service handle.
 *
 * @param[in] vcn The virtual channel number.
 * @param[out] pInfo A pointer to the Program information structure.
 * @param[in] pSourceId A pointer to the Program information structure.
 *
 * @return Upon successful retrieval of the information, RMF_SUCCESS will be returned.
 */
rmf_Error rmf_GetProgramInfoByVCN( uint32_t vcn, rmf_ProgramInfo_t *pInfo);

/**
 * @brief This function will return the Virtual Channel Number associated with the given Source ID.
 *
 * @param[in] decimalSrcId The Input Source Id
 * @param[out] pVirtualChannelNumber A pointer to keep the virtual channel number associated for the program number.
 *
 * @return Upon successful completion, RMF_SUCCESS shall be returned otherwise appropriate error code
 * will be returned.
 */
rmf_Error rmf_GetVirtualChannelNumber( uint32_t decimalSrcId, uint16_t *pVirtualChannelNumber);


void rmf_podmgrDumpDsgStats();
void rmf_podmgrStartHomingDownload(uint8_t ltsId, int32_t status);
#ifdef USE_IPDIRECT_UNICAST
void rmf_podmgrStartInbandTune(uint8_t ltsId, int32_t status);
#endif

/**
 * @brief This function will return the Manufacturer ID from POD Database by calling the corresponding
 * OS implementation or by sending a request to the IPC event manager.
 *
 * @param[out] pManufacturerId A pointer to hold the Manufacturer Id.
 *
 * @return Upon successful completion, RMF_SUCCESS shall be returned otherwise appropriate error code
 * will be returned.
 */
rmf_Error rmf_podmgrGetManufacturerId (uint16_t *pManufacturerId);

/**
 * @brief This function will return the Authorization status of the input Entitlement category.
 *
 * @param[in] eidType Entitlement Type (May be one of VOD, DVR, MDVR, HD etc.)
 * @param[out] pAuthorized A pointer to output the result authorization status.
 *
 * @return Upon successful completion, RMF_SUCCESS shall be returned otherwise appropriate error code
 * will be returned.
 */
rmf_Error rmf_podmgrGetEIDAuthStatus( PODEntitlementAuthType eidType, rmf_osal_Bool *pAuthorized );

rmf_Error rmf_podmgrGetDSGIP(uint8_t *pDSGIP);

/**
 * @brief This function is used to get the maximum number of elementary streams supported by the low level implementation.
 *
 * @param pMaxElmStr[out] A pointer to output the Number of elementary streams.
 *
 * @return Upon successful completion, RMF_SUCCESS will be returned otherwise RMF_OSAL_ENODATA in case of any error.
 */
rmf_Error rmf_podmgrGetMaxDecElemStream( uint16_t *pMaxElmStr );

/**
 * @brief This will invoke a shutdown call on the POD and will de-register event handlers with releasing
 * all resources allocated.
 *
 * @return None.
 */
void rmf_podmgrUnInit();

/**
 * @brief Sends an event to notify the SAS (Subscriber Authorization System) Termination due to a crash or any other reason.
 *
 * @return None.
 */
rmf_Error rmf_podmgrNotifySASTerm();

/** @} */

//#ifdef __cplusplus
//}
//#endif
#endif
