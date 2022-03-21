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

#include "rmfbase.h"
#include "rmfprivate.h"
#include "rmf_inbsimgr.h"
#include "rmfqamsrc.h"
#ifdef TEST_WITH_PODMGR
#include "rmf_pod.h"
#else
#include "rmf_oobsimanagerstub.h"
#endif

#ifdef CAMGR_PRESENT
#include <camgr.h>
#include <camgr_payload.h>
#include <camgr-proxy.h>
#endif

#include "rmf_osal_types.h"
#include <list>

#ifndef RMFQUAMSRCPRIV_H_
#define RMFQUAMSRCPRIV_H_

struct rmf_QAMParams
{
	uint16_t program_no;
	uint32_t frequency;
	uint32_t modulation;
	uint32_t symbol_rate;
};

class timekeeper
{
	public:
	typedef enum
	{
		TUNE_START = 0,
		TUNE_END,
		PAT_ACQ,
		PMT_ACQ,
		CA_START,
		CA_COMPLETE,
		STAGES_MAX
	}timekeeper_stages_t;
	private:
	rmf_osal_TimeMillis m_timestamps[STAGES_MAX];


	public:
	timekeeper();
	void reset();
	void log_time(timekeeper_stages_t stage);
	void print_stats(const char * identifier);
};

/* ************ RMFQAMSrcImpl CLass ******************** */
class RMFQAMSrcImpl : /*public QAMThread ,*/ public RMFMediaSourcePrivate
{
	public:
		typedef enum
			{
				QAM_STATE_IDLE = 0,
				QAM_STATE_OPEN,
				QAM_STATE_PAUSED,
				QAM_STATE_PLAYING
			}qamstate_t;
		
		typedef enum
		{
			PMT_HANDLING_UNOPTIMIZED = 0,
			PMT_OPTIMIZE_CA_OPS,
			PMT_OPTIMIZE_CA_AND_PMT_OPS,
			PMT_HANDLING_MAX
		} qamsrcPmtOptimizationPolicy_t;
	
		std::string saved_uri;
		uint32_t m_pat_timeout_offset;
		bool m_validateTsId;
		uint32_t m_tsIdForValidation;

	public:
		RMFQAMSrcImpl();
		RMFQAMSrcImpl(RMFMediaSourceBase* parent);
		~RMFQAMSrcImpl();
		RMFResult init();
		RMFResult term();

		// Content Acquisition
		RMFResult open(const char* uri, char* mimetype);
		void setTsIdForValidation(uint32_t tsid);
		RMFResult close();

		RMFResult play();
		RMFResult stop();
		RMFResult pause(); 
		void tunerStatusListnerImpl(GstElement * gstqamsrc, gint status);
		RMFResult getSpeed(float& speed);
		GstElement* getGstElement(){ return src_element; }

		void simonitorThread();
		virtual void notifyError(RMFResult err, const char* msg);
		virtual void notifyStatusChange( unsigned long status);
		virtual void notifyCaStatus(const void* data_ptr, unsigned int data_size);
		virtual void notifySection(const void* data_ptr, unsigned int data_size);

		RMFResult getTSID( uint32_t &tsId);
		RMFResult getLTSID( unsigned char &ltsId);
		void getURI(std::string &uri);
#ifdef TEST_WITH_PODMGR
		RMFResult getCCIInfo( char& cciInfo);
		rmf_PodmgrDecryptSessionHandle getPodSessionHandle();
		uint32_t getQamContext();
		void setPodSessionHandle( rmf_PodmgrDecryptSessionHandle podSessionHandle);
		RMFResult waitForCCI (char& cci); 
		void sendCCIChangedEvent();
		void notifyAuthorizationStatus( rmf_osal_Bool auth);
#endif

#ifdef CAMGR_PRESENT
		void ServiceListener(camgr_ctx_t ctx, camgr_svc_t svc, camgr_dtype_t data_type, void* data_ptr, uint32_t data_size, void* user_data);
#endif

		void setupDecryption();
		void handlePODRemoval();
		void handlePODInsertion();
		virtual rmf_osal_Bool readyForReUse(const char* uri = NULL);
		void handleCASystemReady();
		void setPODError( uint32_t value);
		virtual void* getLowLevelElement();
		RMFResult setEvents(IRMFMediaEvents* events);
		RMFResult addEventHandler(IRMFMediaEvents* events);
		static RMFResult init_platform();
		static void check_rf_presence();
		static RMFResult uninit_platform();
		static void disableCaching();
		static void* createLowLevelElement();
		static void freeLowLevelElement( void* element);
		static RMFResult addPlatformEventHandler(IRMFMediaEvents* events);
		static RMFResult removePlatformEventHandler(IRMFMediaEvents* events);
		static RMFResult sendPlatformStatusEvent( unsigned long event);
		static RMFResult isEIDAuthorised ( bool *pAuthStatus );
		static RMFResult getProgramInfo(const char* uri, rmf_ProgramInfo_t* pProgramInfo, uint32_t* pDecimalSrcId);
		virtual RMFResult getSourceInfo(const char* uri, rmf_ProgramInfo_t* pProgramInfo, uint32_t* pDecimalSrcId);
		void* createElement();
		uint32_t getSourceId();
		const rmf_MediaInfo* getInbandMediaInfo();
		static rmf_osal_Bool cachingEnabled;
		static rmf_osal_Bool enableTTS;
		static uint32_t numberOfTuners; 
		static rmf_osal_Mutex mutex ;
		static std::vector<IRMFMediaEvents*> m_event_handler;
		void setPatTimeoutOffset(uint32_t pat_timeout_offset);
		uint32_t getTunerId();
		RMFResult getVideoContentInfo(unsigned int iContent, struct VideoContentInfo * pVideoContentInfo);
		void notifySignalsDisconnected();
		void syncLossTimerExited();
		gboolean syncLossTimerCallback();
		RMFResult getStatus(qamsrc_status_t &status);
		RMFResult enableTdtTot(bool enable);
	private:
		/**************  Private functions ****************/
		RMFResult populateBin(GstElement* bin);
		static void setGhostPadTarget(GstElement* bin, GstPad* target, bool unref_target);
		unsigned int compareMediaInfo(rmf_MediaInfo *newMediaInfo);
		void processProgramInfo();
		virtual void requestINBProgramInfo();
		void clearINBProgramInfo();
		void stopSIMonitorThread();
		const char* getErrorString (RMFResult errorCode);
		void notifySIMonitorThread(uint32_t event_type);
		void updateOutputPMT();

		/**************  Private Variables ****************/
		rmf_ProgramInfo_t pInfo;
#ifdef TEST_WITH_PODMGR
		unsigned char m_cci;
		GCond* m_cciCond;
		GMutex* m_cciMutex;

		rmf_osal_Bool m_isAuthorized; 

		rmf_PodmgrDecryptSessionHandle sessionHandlePtr;
#endif

#ifdef CAMGR_PRESENT
		camgr_ctx_t camgr_ctx;
		camgr_svc_t camgr_svc;
		uint8_t *catTable;
		uint16_t catTableSize;
		uint16_t totalCatDescriptorSize;
		void processCatInfo();
#endif

		uint32_t maxQueueSize;
		gulong status_signal_handler_id;
		guint m_sync_loss_callback_id;
		rmf_osal_ThreadId siMonitorThreadId;
		sem_t* siDoneSem;
		rmf_osal_eventqueue_handle_t siEventQHandle;
		
		volatile rmf_osal_Bool pmtEventSent;
		volatile rmf_osal_Bool patEventSent;
		
		rmf_MediaInfo mediaInfo;
		rmf_osal_TimeMillis tuneInitatedTime;
		rmf_osal_TimeMillis psiAcqStartTime;
		qamstate_t state;
		rmf_osal_Mutex m_mutex;
		uint32_t tsId;
		rmf_osal_Cond pat_cond;
		rmf_osal_Cond m_signals_disconnected;
		rmf_osal_Cond m_sync_loss_timer_disconnected;
		
		volatile rmf_osal_Bool pat_available;
		rmf_osal_Bool is_clear_content;
		rmf_osal_Bool m_is_tuner_locked;
		rmf_osal_Bool m_pmt_optimization_active;

		uint32_t m_qamtuner_error;
		uint32_t m_pod_error;
		static rmf_osal_Bool supportTuningInNULLState;
		uint32_t m_context;
	protected:
		uint32_t decimalSrcId;
		uint32_t m_si_error;
		uint32_t m_error_bitmap;
		rmf_PsiSourceInfo m_sourceInfo;
		rmf_InbSiMgr *pInbandSI;
		rmf_QAMParams QAMParams;
		GstElement* src_element;
		timekeeper m_timekeeper;
		static RMFResult  get_querystr_val(string url, string q_str, string &value_str);
		virtual rmf_InbSiMgr* createInbandSiManager();
		inline virtual rmf_osal_Bool stopCAWhenPaused(void) const {return TRUE;}
		rmf_osal_Bool setErrorBits(uint32_t bitmask, rmf_osal_Bool value);
		void resetSptsTimer();
		virtual unsigned int getPMTOptimizationPolicy() const {return PMT_HANDLING_UNOPTIMIZED;}
};
#endif
