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
 * @file rmfqamsrc.h
 * @brief RMF QAM Source header file.
 * This file contains the public interfaces or RMF QAM Source.
 */

/**
 * @defgroup QAM QAM Source
 * @ingroup RMF
 * @defgroup rmfqamsrc QAM Source Base
 * @ingroup QAM
 * @defgroup rmfqamsrcclass QAM Source Base Class
 * @ingroup rmfqamsrc
 * @defgroup rmfqamsrcMacros QAM Source Base Macros
 * @ingroup rmfqamsrc
 * @defgroup qamsourcegenericerror QAM Source Generic Error
 * @ingroup rmfqamsrcMacros
 * @defgroup qamsourceSITablerror QAM Source SI Table Error
 * @ingroup rmfqamsrcMacros
 * @defgroup rmfqamsrcdatastructures QAM Source Base Data Structures
 * @ingroup rmfqamsrc
 * @defgroup rmfqamsrcccistatus QAM Source CCI Status
 * @ingroup rmfqamsrcMacros
 * @defgroup rmfqamsrcetuneevent QAM Source Tune Event
 * @ingroup rmfqamsrcMacros
 */

/**
 * @defgroup QAM QAM Source
 * <b>QAM Source is responsible for the following operations. </b>
 * - Get tuning parameters and program number from OOB SI manager.
 * - Create inband SI manager instance and get program specific information.
 * - Create and configure qamtunersrc.
 * - Configure decryption using POD manager.
 * - Utilize G-Streamer pipeline to send CCI related events originated in POD manager.
 * - Utilizes G-Streamer plugins (qamtunersrc) for tuning and section filtering.
 * - The plugin "qamtunersrc" is a push based source element which is responsible for
 * managing tuners and demux, filter Pids required for SPTS stream and output SPTS stream as g-streamer buffer.
 * @ingroup RMF
 *
 * @defgroup qamsrcapi QAM Source - API List
 * Describe the details about QAM Source API specifications.
 * @ingroup QAM
 */

#ifndef RMFQAMSOURCE_H
#define RMFQAMSOURCE_H

#include <map>
#include <string>
#include "rmfbase.h"
#include "rmf_error.h"
#include "rmf_osal_event.h"
#include "rmf_osal_sync.h"

#define QAMSRC_GET_TUNER_ID_SUPPRTED 1

/**
 * @defgroup qamsourceerrorcodes QAM Source Error Codes
 * @brief The following errors are generated when the invalid parameter is used.
 * Events are generated even if realted to demux error and insufficeint memory is used.
 * @ingroup rmfqamsrc
 * @addtogroup qamsourceerrorcodes 
 * @{
 */
#define RMF_QAMSRC_ERROR_BASE 0x100 /**< QAM Src error code start */
#define RMF_QAMSRC_ERROR_EINVAL (RMF_QAMSRC_ERROR_BASE +5) /**< QAM Src error invalid parameter */
#define RMF_QAMSRC_ERROR_DEMUX (RMF_QAMSRC_ERROR_BASE +6) /**< QAM Src error related to demux */
#define RMF_QAMSRC_ERROR_NOMEM (RMF_QAMSRC_ERROR_BASE +7) /**< QAM Src error insufficient memory */

/** @}
 * @defgroup qamsourceerrorevents QAM Source Events
 * @brief The following error is generated when the tuner is not locked and SI table status when its received or updated.
 * @ingroup rmfqamsrc
 * @addtogroup qamsourceerrorevents 
 * @{
 */
#define RMF_QAMSRC_ERROR_TUNER_NOT_LOCKED (RMF_QAMSRC_EVENT_BASE +1)     /**<  Tuner unlock event */
#define RMF_QAMSRC_ERROR_PAT_NOT_AVAILABLE (RMF_QAMSRC_EVENT_BASE +2)     /**<  PAT not recieved after timeout*/
#define RMF_QAMSRC_ERROR_PMT_NOT_AVAILABLE (RMF_QAMSRC_EVENT_BASE +3)     /**< QAM Src event  on PMT not recieved after timeout*/
#define RMF_QAMSRC_ERROR_UNRECOVERABLE_ERROR (RMF_QAMSRC_EVENT_BASE +4)     /**<  Unexpected failure */
#define RMF_QAMSRC_EVENT_CANNOT_DESCRAMBLE_ENTITLEMENT (RMF_QAMSRC_EVENT_BASE +5)     /**< Entitilement failure event*/
#define RMF_QAMSRC_EVENT_CANNOT_DESCRAMBLE_RESOURCES (RMF_QAMSRC_EVENT_BASE +6)     /**<  Descramble resource  failures */
#define RMF_QAMSRC_EVENT_MMI_PURCHASE_DIALOG (RMF_QAMSRC_EVENT_BASE +7)     /**< MMI interaction for a purchase dialog is required */
#define RMF_QAMSRC_EVENT_MMI_TECHNICAL_DIALOG  (RMF_QAMSRC_EVENT_BASE +8)     /**<  MMI interaction for a technical dialog is required  */
#define RMF_QAMSRC_EVENT_POD_REMOVED  (RMF_QAMSRC_EVENT_BASE +10)     /**< CableCard removed. */
#define RMF_QAMSRC_EVENT_POD_RESOURCE_LOST  (RMF_QAMSRC_EVENT_BASE +11)     /**< CableCard resources for this session have been lost to a higher priority session. */
#define RMF_QAMSRC_ERROR_TUNE_FAILED (RMF_QAMSRC_EVENT_BASE +12)     /**<  Tune request Failure*/
#define RMF_QAMSRC_ERROR_PROGRAM_NUMBER_INVALID (RMF_QAMSRC_EVENT_BASE +13)     /**<  Program number invalid */
#define RMF_QAMSRC_ERROR_PROGRAM_NUMBER_INVALID_ON_PAT_UPDATE (RMF_QAMSRC_EVENT_BASE +14)     /**< Program number invalid on a PAT update event */
#define RMF_QAMSRC_ERROR_TUNER_UNLOCK_TIMEOUT (RMF_QAMSRC_EVENT_BASE +15)	/**< Tuner didn't recover signal lock  within specified time. */
#define RMF_QAMSRC_ERROR_TSID_MISMATCH (RMF_QAMSRC_EVENT_BASE +16)     
#define RMF_QAMSRC_ERROR_SPTS_TIMEOUT (RMF_QAMSRC_EVENT_BASE + 17) /**< SPTS filters have run out of data. */
#define RMF_QAMSRC_ERROR_CHANNELMAP_LOOKUP_FAILURE (RMF_QAMSRC_EVENT_BASE + 18) /**< Could not find service in channel map*/
#define RMF_QAMSRC_ERROR_NO_TUNERS_AVAILABLE (RMF_QAMSRC_EVENT_BASE + 19) /**< No free tuners are available*/
#define RMF_QAMSRC_ERROR_NO_CA_SYSTEM_READY (RMF_QAMSRC_EVENT_BASE + 20) /**< No CA system ready to tune*/


/** @}
 * @defgroup qamsourcestatusevents QAM Source Events
 * @brief The following events are generated when tune is synced and streaming.
 * @ingroup rmfqamsrc
 * @addtogroup qamsourcestatusevents 
 * @{
 */
#define RMF_QAMSRC_EVENT_TUNE_SYNC (RMF_QAMSRC_EVENT_BASE +33)     /**<  Tuner becomes tuned and synced */
#define RMF_QAMSRC_EVENT_PAT_ACQUIRED (RMF_QAMSRC_EVENT_BASE +34)     /**<  PAT acquired*/
#define RMF_QAMSRC_EVENT_PMT_ACQUIRED (RMF_QAMSRC_EVENT_BASE +35)     /**< PMT acquired*/
#define RMF_QAMSRC_EVENT_STREAMING_STARTED (RMF_QAMSRC_EVENT_BASE +36)     /**< Data streaming started*/
#define RMF_QAMSRC_EVENT_EID_DISCONNECTED (RMF_QAMSRC_EVENT_BASE +37)     /**< EID disconnected*/
#define RMF_QAMSRC_EVENT_EID_CONNECTED (RMF_QAMSRC_EVENT_BASE +38)     /**< EID connected*/
#define RMF_QAMSRC_EVENT_POD_READY (RMF_QAMSRC_EVENT_BASE +39)     /**< POD is ready*/
#define RMF_QAMSRC_EVENT_ERRORS_ALL_CLEAR (RMF_QAMSRC_EVENT_BASE + 40) /**< All QAM errors have been recovered from.*/
#define RMF_QAMSRC_EVENT_CA_SYSTEM_READY (RMF_QAMSRC_EVENT_BASE + 41) /**< CA system is ready for use.*/
#define RMF_QAMSRC_EVENT_SPTS_OK (RMF_QAMSRC_EVENT_BASE + 42) /**< SPTS has recovered.*/ 
#define RMF_QAMSRC_EVENT_CAT_ACQUIRED (RMF_QAMSRC_EVENT_BASE +43)     /**<  CAT acquired*/
#define RMF_QAMSRC_EVENT_SECTION_ACQUIRED (RMF_QAMSRC_EVENT_BASE +44)     /**<  Any section acquired for the filters set*/
#define RMF_QAMSRC_EVENT_PAT_UPDATE (RMF_QAMSRC_EVENT_BASE +45)     /**<  PAT Update*/
#define RMF_QAMSRC_EVENT_CAT_UPDATE (RMF_QAMSRC_EVENT_BASE +46)     /**<  CAT Update*/
#define RMF_QAMSRC_EVENT_PMT_UPDATE (RMF_QAMSRC_EVENT_BASE +47)     /**<  PMT Update*/

/** @}
 * @defgroup qamsourceccistatus QAM Source CCI status values.
 * @brief The following events are generated when cci info is requested.
 * @ingroup rmfqamsrc
 * @addtogroup qamsourceccistatus 
 * @{
 */

#define RMF_QAMSRC_CCI_STATUS_NONE 0xFF
#define RMF_QAMSRC_CCI_STATUS_COPYING_IS_PERMITTED 0x0
#define RMF_QAMSRC_CCI_STATUS_NO_FURTHER_COPYING_IS_PERMITTED 0x01
#define RMF_QAMSRC_CCI_STATUS_ONE_GENERATION_COPYING_IS_PERMITTED 0x02
#define RMF_QAMSRC_CCI_STATUS_COPYING_IS_PROHIBITED 0x03
/** @} */

#ifdef USE_TRM
/**
 * @brief This represents the valid values for the rmfqam source pretune status.
 * @ingroup rmfqamsrc
 */
typedef enum
{
	RMF_QAMSRC_PRETUNE_REQUEST = 0,
	RMF_QAMSRC_PRETUNE_RELEASE,
	RMF_QAMSRC_PRETUNE_SHUTDOWN
}rmf_pretuneEvents_t;
#endif
#define QAMSRC_DEFAULT_TOKEN "default_token"
#define QAMSRC_UNRESERVED_TOKEN "unreserved_token"
/** @}
 * @brief Class extending RMFMediaSourceBase to implement QAM Source.
 * @ingroup rmfqamsrcclass
 */

typedef struct
{
	rmf_osal_Bool is_locked;
	rmf_osal_Bool pat_acquired;
	rmf_osal_Bool pmt_acquired;
}qamsrc_status_t;

class RMFQAMSrc : public RMFMediaSourceBase
{
	public:
		typedef enum {
			USAGE_TSB    = 1 << 0,
			USAGE_LIVE   = 1 << 1,
			USAGE_RECORD = 1 << 2,
			USAGE_AD     = 1 << 3,
			USAGE_PODMGR = 1 << 4,
			USAGE_LIVE_UNRESERVED = 1 << 5,
			USAGE_RECORD_UNRESERVED = 1 << 6,
			USAGE_PRETUNE = 1 << 7,
			USAGE_END_MARKER = 1 << 8
		} usage_t;
		RMFQAMSrc();
		~RMFQAMSrc();
		static RMFResult init_platform();
		static RMFResult uninit_platform();
		RMFResult init();
		RMFResult term();
		RMFResult open(const char* uri, char* mimetype);
		RMFResult close();
		RMFResult play(float& speed, double& time);
		RMFResult pause();
                RMFResult stop();
		RMFMediaSourcePrivate* createPrivateSourceImpl();
		void* createElement();
		RMFResult getTSID( uint32_t &tsId);
		RMFResult getLTSID( unsigned char &tsId);
		static RMFResult addPlatformEventHandler(IRMFMediaEvents* events);
		static RMFResult removePlatformEventHandler(IRMFMediaEvents* events);
		static RMFResult isEIDAuthorised ( bool *pAuthSatus  );
		static bool useFactoryMethods();
		static void* getLowLevelElement();
		static void freeLowLevelElement( void* element);
		static RMFQAMSrc* getQAMSourceInstance( const char* uri, usage_t requestedUsage = USAGE_LIVE, const std::string &token = QAMSRC_DEFAULT_TOKEN, RMFResult *errorCode = NULL);
		static void freeQAMSourceInstance( RMFQAMSrc* qamsrc, usage_t usage = USAGE_LIVE, const std::string &token = QAMSRC_DEFAULT_TOKEN);
		static void freeCachedQAMSourceInstances( );
		static void disableCaching();
		static RMFResult changeURI( const char* uri , RMFQAMSrc* old, RMFQAMSrc** updated, bool &newInstance ,
			usage_t usage = USAGE_LIVE, const std::string &token=QAMSRC_DEFAULT_TOKEN);
#ifdef USE_TRM
		static void performPretune( const char * uri );
#endif

		RMFResult enableTdtTot( bool enable);
		int32_t getTunerId(void);
	private :
#ifdef USE_TRM
		static rmf_osal_eventqueue_handle_t pretuneEventQHandle;
		static bool enablePretune;
#endif
		static bool livePreemptionEnabled;
		int factoryManaged;
		static bool useFactory;
		void setFactoryManaged( int fm);
		static RMFQAMSrc* getExistingInstance (const char * uri, bool addRef, int *references = NULL);
		static void removeFromList (void* obj);
		static RMFQAMSrc* preempt(const char *uri, usage_t requestedUsage, const std::string &token);
		static void adjustUsageForLowPriorityTokens(usage_t &usage, const std::string &token);
		void prepForPreemption(void);
#ifdef USE_TRM
		static void pretune_enabler_thread(void* arg);
#endif
	public:

		void attachUsage(usage_t usage, const std::string &token="UNKNOWN");
		void detachUsage(usage_t usage, const std::string &token="UNKNOWN");
		uint32_t getUsage(void);
        void dumpUsage(void);
        void flushUsages(void);
		RMFResult getStatus(qamsrc_status_t &status);
		RMFResult getMatchingToken(usage_t usage, std::string &token);
		typedef int (*QamsrcReleaseCallback_t)(const std::string token, const std::string uri);
		typedef void (*UpdateTunerActivityCallback_t)(const std::string &deviceId,const std::string &activityStatus, const std::string &lastActivityTimeStamp, const std::string &lastActivityAction);
		static void registerQamsrcReleaseCallback(QamsrcReleaseCallback_t callback); /*This API will be retired as soon as mediastreamer 
		switches to the below version of the API.*/
		static void registerQamsrcReleaseCallback(usage_t usage, QamsrcReleaseCallback_t callback);
                static UpdateTunerActivityCallback_t tunerActivityUpdateCallback;
                static void registerUpdateTunerActivityCallback(UpdateTunerActivityCallback_t callback);
		static uint32_t mapUsageToPriority(uint32_t usage);
		static RMFQAMSrc* findQAMSrcToPreempt(usage_t requestedUsage, uint32_t &retrievedUsage);
		static std::map<usage_t, QamsrcReleaseCallback_t> releaseCallbackList;
		static void notifyQAMReady();
	private:
		rmf_osal_Mutex m_mutex;
		std::map<std::string, uint32_t> usages;
		uint32_t unknownUsages[4];
};

#ifdef USE_TRM
class RMFPretuneEventHandler : public IRMFMediaEvents
{
public:
	RMFPretuneEventHandler( rmf_osal_eventqueue_handle_t  eventQueue, RMFQAMSrc* source);
	~RMFPretuneEventHandler();
	void status( const RMFStreamingStatus& status );
	void error(RMFResult err, const char* msg);
	void stopped();
private:
	rmf_osal_eventqueue_handle_t  eventQueue;
	RMFQAMSrc *m_source;
	void releasePretuneReference(void);
};

typedef struct 
{
	RMFPretuneEventHandler* handler;
	RMFQAMSrc* source;
}rmf_pretune_event_payload_t;
#endif

/**
 * @defgroup rmfqamsourcepod QAM Source POD Interfaces.
 * @brief Interfaces exposed to be used by POD module.
 * @ingroup rmfqamsrc
 * @addtogroup rmfqamsourcepod 
 * @{
 */

/**
 * @brief Demux/Transport Path Info structure
 */
typedef struct
{
	int   demux_handle;    /**< Demux handle used by RMFQAMSrc instance*/
	unsigned char ltsid;      /**< Local Transport Stream Id. To be used by the POD module. */
	int   hSecContextFor3DES;      /**< For Back door key loading for 3DES decryption (cablecard). */
	unsigned int  tsd_key_location;      /**< Valid values for loading clear keys are 128 - 153. */
}rmf_transport_path_info;

/** @}*/
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef TEST_WITH_PODMGR
	/* @{
	 */

	/**
	 * @brief Gets LTS Id corresponding to tunerId
	 *
	 * @param[in] tunerId  Tuner Id.
	 * @param[out] ltsid    LTS Id corresponding to tuner Id
	 *
	 * @return rmf_Error     defined in rmf_error.h
	 *
	 * @retval RMF_SUCCESS      No Error.
	 * @retval RMF_QAMSRC_ERROR_EINVAL   Invalid Parameter.
	 */
	rmf_Error rmf_qamsrc_getLtsid( int tunerId, unsigned char *ltsid );

	/**
	 * @brief Gets rmf_transport_path_info corresponding to ltsId
	 *
	 * @param[in] ltsid  LTS Id.
	 * @param[out] pTransportPathInfo    Outputs transport path info corresponding to ltsid
	 *
	 * @return rmf_Error     defined in rmf_error.h
	 *
	 * @retval RMF_SUCCESS      No Error.
	 * @retval RMF_QAMSRC_ERROR_EINVAL   Invalid Parameter.
	 */
	rmf_Error rmf_qamsrc_getTransportPathInfoForLTSID(int ltsid, rmf_transport_path_info *pTransportPathInfo);

	/** @}*/
#endif

#ifdef __cplusplus
}
#endif

//------------------------------
#endif // RMFQAMSOURCE_H

