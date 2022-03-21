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
 * @file rmfsdvsource.cpp
 * @brief SDV functionality is used since many customers are watching the same programs,
 * statistical efficiencies result from offering a virtual channel map having more programs
 * than the physical bandwidth is capable of (over-subscription).
 */

#include <algorithm>
#include "rmfsdvsource.h"
#include "rmfsdvsrcpriv.h"
#include "SDVConfigObtainer.h"
#include "SDVDiscoveryImpl.h"

#include <netinet/in.h>
#include "sysMgr.h"
#define SOURCEID_PRIFIX_STRING "ocap://0x"
#define TUNEPARAMS_PRIFIX_STRING "tune://"

rmf_osal_Mutex RMFSDVSrc::m_sdvSwitchedListMutex;
bool RMFSDVSrc::m_isEnabled = FALSE;
SDVForceTuneEventHandler * RMFSDVSrc::forceTuneHandler = NULL;

uint16_t * RMFSDVSrc::m_sdvChanneMap = NULL;
uint16_t  RMFSDVSrc::m_sdvChanneMapSize = 0;


/* ************ RMFSDVSrc function definitions ******************** */

/**
 * @breif It is used to initialize the RMFSDVSrc platform functionality.
 * Mutex is created for protecting access to switched list.
 * Set configuration observer is for handling force tune events. If file is already available
 * observer will be immediatly notified.
 * Only a single observer can be set per file type.
 * It starts the Configutation obtainer thread and begins download/retry and refresh operations.
 * It starts the SDVDiscovery service to read its configuration file and monitor it for changes.
 * Registers itself as an IARM event handler for events indicating MCP data is needed by the SDVAgent
 * Registers itself as the listener for MCP Section filtering events.
 *
 * @return RMFResult
 * @retval RMF_RESULT_SUCCESS If successfully initialized the sdvsrc platform functionality.
 * @retval RMF_RESULT_INTERNAL_ERROR It indicates that all SDV Sources cannot register itself
 * for Force Tune Events when tuning to a SDV Channel.
 */
RMFResult RMFSDVSrc::init_platform(){
    m_isEnabled = FALSE;

    // rmfStreamer must be connected to IARM bus in order for SDV Agent to enable RMF SDV
    int isIarmConnected = 0;
    if (IARM_Bus_IsConnected("rmfStreamer", &isIarmConnected) !=  IARM_RESULT_SUCCESS) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.QAMSRC", "[RMFSDVSrc] %s() IARM_Bus_IsConnected failed\n", __FUNCTION__);
    }
    if (isIarmConnected == 1) {
        if (IARM_Bus_RegisterCall(IARM_BUS_RMFSDV_ATTEMPT_ENABLE, (IARM_BusCall_t)enableSdv) != IARM_RESULT_SUCCESS) {
            RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.QAMSRC", "[RMFSDVSrc] %s() failed to register IARM_BUS Call\n", __FUNCTION__);
            return RMF_RESULT_INTERNAL_ERROR;
        }
    } else {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.QAMSRC", "[RMFSDVSrc] %s() rmfStreamer not connected to IARM Bus, SDV cannot be enabled\n", __FUNCTION__);
        return RMF_RESULT_INTERNAL_ERROR;
    }

    return RMF_RESULT_SUCCESS;
}

bool RMFSDVSrc::isSdvEnabled() {
    return m_isEnabled;
}


static bool sdvReadyNotified = false;
static bool sdvFailSafeTimerStarted = false;
static void notifySdvReady()
{
    if(!sdvReadyNotified) {
        RMFQAMSrc::notifyQAMReady();
        sdvReadyNotified = true;
    }
}

gboolean sdvReadyTimeoutCallback(gpointer user_data) {
    RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "[RMFSDVSrc] %s() SDV ready timer expired\n", __FUNCTION__);
    notifySdvReady();
    return false /*G_SOURCE_REMOVE*/;
}

/* If SDV fails to issue QAM ready for any reason, recorder and any other entities waiting for it will be
 * held up permanently. SDV can fail to issue QAM ready for many reasons. It may be unable to contact auth-service
 * or unable to download the configuration files (discovery, switched service list etc). Under these conditions,
 * we will unblock anybody waiting for green light from SDV within N seconds.*/
static const unsigned int MAX_SDV_READY_WAIT_TIME_SEC = 180; //3 minute max wait.
static void startFailSafeTimer()
{
    if(!sdvFailSafeTimerStarted) { 
        g_timeout_add_seconds(MAX_SDV_READY_WAIT_TIME_SEC, &sdvReadyTimeoutCallback, NULL);
        sdvFailSafeTimerStarted = true;
    }
}


IARM_Result_t RMFSDVSrc::enableSdv(IARM_RMFSDV_ENABLE_PAYLOAD *payload) {
    SDVConfigObtainer *configObtainer = SDVConfigObtainer::getInstance();

    // If SDV has not been enabled
    if (!m_isEnabled) {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "[RMFSDVSrc] %s() attempt to enable RMF SDV\n", __FUNCTION__);
        // If URL templates successfully acquired, start the RMF SDV subcomponents
        SDVConfigObtainer::sdvTemplateAcquisitionResult_t result = configObtainer->acquireUrlTemplates();
        if (SDVConfigObtainer::RESULT_TEMPLATE_ACQUISITION_SUCCESS == result) {
            m_isEnabled = TRUE;
            rmf_osal_mutexNew(&m_sdvSwitchedListMutex);
            SDVDiscovery::getInstance()->start();

            configObtainer->setObserver(SDVConfigObtainer::SwitchedChannelList, NULL, sdvSwitchedListUpdated);
            configObtainer->registerSDVTuneReadyCallback(&notifySdvReady);
            startFailSafeTimer();
            configObtainer->start();

            forceTuneHandler = new SDVForceTuneEventHandler();
            if(RMF_RESULT_SUCCESS != forceTuneHandler->start()) {
                RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.QAMSRC", "[RMFSDVSrc] %s() failed to start SDVForceTuneEventHandler\n", __FUNCTION__);
            }

            payload->result = IARM_RMFSDV_ENABLE_PAYLOAD::ENABLE_SUCCESS;
            RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "[RMFSDVSrc] %s() RMF SDV is now enabled\n", __FUNCTION__);
            
        } else if(SDVConfigObtainer::RESULT_TEMPLATE_ACQUISITION_COMM_FAILURE == result) {
            payload->result = IARM_RMFSDV_ENABLE_PAYLOAD::NO_RESPONSE;
            RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "[RMFSDVSrc] %s() temporarily unable to enable SDV due to comm issues\n", __FUNCTION__);
            startFailSafeTimer();
        // Else URL templates do not exist and SDV to remain disabled
        } else {
            payload->result = IARM_RMFSDV_ENABLE_PAYLOAD::ENABLE_REJECT;
            RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "[RMFSDVSrc] %s() RMF SDV cannot be enabled\n", __FUNCTION__);
            notifySdvReady(); 
        }
    // Else SDV has already been enabled
    } else {
        payload->result = IARM_RMFSDV_ENABLE_PAYLOAD::ENABLE_SUCCESS;
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "[RMFSDVSrc] %s() RMF SDV already enabled\n", __FUNCTION__);
    }
    return IARM_RESULT_SUCCESS;
}

/** 
 * @brief It is used to uninitialize the RMFSDVSrc platform functionality.
 * It stops the SDVForceTuneHandler by clearing the channel map and unregisters for IARM Bus Events.
 *
 * @return RMFResult
 * @retval RMF_RESULT_SUCCESS If successfully uninitialized the sdvsrc platform functionality.
 * @retval  RMF_RESULT_INTERNAL_ERROR It indicates that all SDV Sources not unregisters itself for Force Tune Events when tuning to a SDV Channel.
 */
RMFResult RMFSDVSrc::uninit_platform(	){

    if(RMF_RESULT_SUCCESS != forceTuneHandler->stop()){
        return RMF_RESULT_INTERNAL_ERROR;
    }
    delete forceTuneHandler;

    rmf_osal_mutexAcquire(m_sdvSwitchedListMutex);
    if(m_sdvChanneMap != NULL){
        delete m_sdvChanneMap;
        m_sdvChanneMapSize = 0;
        m_sdvChanneMap = NULL;
    }
    rmf_osal_mutexRelease(m_sdvSwitchedListMutex);
    rmf_osal_mutexDelete(m_sdvSwitchedListMutex);

    return RMF_RESULT_SUCCESS;
}

/**
 * @brief Constructor to initializes and creates the instance of RMFSDVSrc.
 *
 * @return None.
 */
RMFSDVSrc::RMFSDVSrc(  ){
    RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", " %s():%d\n",__FUNCTION__, __LINE__ );
}

/**
 * @brief Destructor to deinitializes the instance of RMFSDVSrc.
 *
 * @return None.
 */
RMFSDVSrc::~RMFSDVSrc(  ){
    RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", " %s():%d\n", __FUNCTION__, __LINE__ );
}

void RMFSDVSrc::forceTuneEventHandler(void * ptrObj, uint32_t sourceId){
    ((RMFSDVSrcImpl*)ptrObj)->notifyError(RMF_QAMSRC_ERROR_TUNE_FAILED, NULL);
}

RMFMediaSourcePrivate *RMFSDVSrc::createPrivateSourceImpl(){
    RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", " %s():%d\n", __FUNCTION__, __LINE__ );
    return new RMFSDVSrcImpl( this );
}

bool RMFSDVSrc::isSwitched(const char * ocapLocator){
    char *tmp;
    uint16_t decimalSrcId = 0;
    tmp =  strstr ( (char* )ocapLocator, SOURCEID_PRIFIX_STRING );
    bool result = false;
    if ( NULL != tmp) {
        decimalSrcId = strtol( tmp+strlen(SOURCEID_PRIFIX_STRING), NULL, 16);
        rmf_osal_mutexAcquire(m_sdvSwitchedListMutex);
        for(int i=0;i!= m_sdvChanneMapSize;++i){
            if(m_sdvChanneMap[i] == decimalSrcId){
                result = true;
                RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "[RMFSDVSrcImpl] %s() locator %s is SDV\n", __FUNCTION__, ocapLocator);
                break;
            }
        }
        rmf_osal_mutexRelease(m_sdvSwitchedListMutex);
    }
    return result;
}

/**
 * @brief Static callback for when the SDV Switched list file changes.
 *
 * @param [in] ptrObj NULL.
 * @param [in] data Binary data of the SDV Channel list file.
 * @param [in] size Size of configutation file in bytes.
 *
 * @return rmf_Error
 * @return RMF_SUCCESS If file is well formed.
 * @retval RMF_OSAL_ENODATA If file is corrupted.
 */
rmf_Error RMFSDVSrc::sdvSwitchedListUpdated(void * ptrObj, uint8_t *data, uint32_t size){
    RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "[RMFSDVSrcImpl] %s()\n", __FUNCTION__);
    if(size < 3){   //Make sure file is long eneogh to at least reach the sourceid count
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.QAMSRC", "[RMFSDVSrcImpl] %s() channel List file is to small = \n", __FUNCTION__);
        return RMF_OSAL_ENODATA;
    }
    uint16_t * ptr = (uint16_t *)&data[1];  //first byte is reserved, next two bytes are count
    uint16_t count = ntohs( ptr[0] );
    RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "[RMFSDVSrcImpl] %s() file size = %d count = %d\n", __FUNCTION__, size, count);
    if( (count *2) != size -3){   // Each sourceid is 2 bytes. Do not include first 3 bytes in calculation.
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.QAMSRC", "[RMFSDVSrcImpl] %s() channel List is not well formed = \n", __FUNCTION__);
        return RMF_OSAL_ENODATA;
    }

    rmf_osal_mutexAcquire(m_sdvSwitchedListMutex);

    if(m_sdvChanneMap != NULL){
        delete m_sdvChanneMap;
    }
    m_sdvChanneMap = (uint16_t *) malloc(sizeof(uint16_t) * count);
    for(int i=0;i!=count;++i){
        uint16_t sourceid = ntohs( ptr[i + 1] );
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "[RMFSDVSrcImpl] %s() \tsourceid= %d\n", __FUNCTION__, sourceid);
        m_sdvChanneMap[i] = sourceid;
    }
    m_sdvChanneMapSize = count;

    rmf_osal_mutexRelease(m_sdvSwitchedListMutex);
    return RMF_SUCCESS;
}


/* ************ RMFSDVSrcImpl functions definition ******************** */

RMFSDVSrcImpl::RMFSDVSrcImpl( RMFMediaSourceBase * parent ):RMFQAMSrcImpl
        ( parent ){
	sdvSectionFilter = NULL;
    m_isSwitched = false;
}

RMFSDVSrcImpl::~RMFSDVSrcImpl(){/** no-op */}

RMFResult RMFSDVSrcImpl::getSourceInfo(const char *ocaplocator, rmf_ProgramInfo_t *pInfo, uint32_t *pDecimalSrcId) {
    RMFResult ret;

    if (RMFSDVSrc::isSwitched(ocaplocator)) {
    	// If section filter exists, sdv session must already be open and therefore caller should already have pInfo
    	if (sdvSectionFilter != NULL) {
            RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "[RMFSDVSrcImpl] %s() ProgramInfo already acquired for SDV session\n", __FUNCTION__);
    		return RMF_RESULT_SUCCESS;
    	}

        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "[RMFSDVSrcImpl] %s() URI is switched service\n", __FUNCTION__);
        if(((SDVDiscoveryImpl*)SDVDiscovery::getInstance)->isMCPDataDiscovered() == RMF_SUCCESS){
            ret = getSdvSourceInfo(ocaplocator, pInfo, pDecimalSrcId);
        }else{
            RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.QAMSRC", "[RMFSDVSrcImpl] %s() MCP Data is not available \n", __FUNCTION__);
            ret = RMF_RESULT_INTERNAL_ERROR;
        }
    }
    else {
    	ret = RMFQAMSrcImpl::getSourceInfo(ocaplocator, pInfo, pDecimalSrcId);
    }
    return ret;
}

static unsigned int gNumRetryAllowances = 3;
RMFResult RMFSDVSrcImpl::open( const char *uri, char *mimetype ) {

    bool isSwitched = RMFSDVSrc::isSwitched(uri);
    if(isSwitched){
        SDVDiscovery::getInstance()->attemptDiscoveryIfNeeded(this);
    }

    RMFResult ret =  RMFQAMSrcImpl::open(uri, mimetype);

    if (ret == RMF_RESULT_SUCCESS && RMFSDVSrc::isSdvEnabled()) {
        // If switched or autodiscovery tune, start SectionFilter
    	if (isSwitched || isSdvDiscoveryUri(uri)) {
			RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "[RMFSDVSrcImpl] %s() start section filter for SDV or autodiscovery tune\n", __FUNCTION__);
			sdvSectionFilter = new SDVSectionFilter(m_sourceInfo.tunerId, m_sourceInfo.freq, m_sourceInfo.dmxHdl);
			SDVSectionFilterRegistrar::getInstance()->startSectionFilter(sdvSectionFilter);

			if (isSwitched) {
				SDVForceTuneEventHandler::SDV_FORCE_TUNE_HANDLER_t ftHandler = {this, &RMFSDVSrc::forceTuneEventHandler};
				RMFSDVSrc::forceTuneHandler->registerEventHandler(m_sourceInfo.tunerId, ftHandler);
			}
    	}
    	// Else must be linear tune, notify Agent so it can be reported to sdv server
    	else {
    		// Notify agent we are performing a linear tune
    		IARM_SDVAGENT_OPEN_SESSION_PAYLOAD payload;
    		payload.requestInfo.sourceId = decimalSrcId;
    		payload.requestInfo.tunerId = m_sourceInfo.tunerId;
    		payload.requestInfo.isSdvService = 0;
    		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[RMFSDVSrcImpl] %s() invoke IARM_BUS_SDVAGENT_API_OPEN_SESSION for linear tune\n", __FUNCTION__);

            do {
                IARM_Result_t iarmResult = IARM_Bus_Call(IARM_BUS_SDVAGENT_NAME, IARM_BUS_SDVAGENT_API_OPEN_SESSION, &payload, sizeof(IARM_SDVAGENT_OPEN_SESSION_PAYLOAD));
                if (IARM_RESULT_SUCCESS != iarmResult) {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[RMFSDVSrcImpl] %s() IARM_Bus_Call failed with %d\n", __FUNCTION__, iarmResult);
                    /* Limited retries per power cycle are allowed if the IARM call fails. It's possible SDV or networking infrastructure is not really ready right now.*/
                    if(0 < gNumRetryAllowances) {
                        gNumRetryAllowances--;
                    }
                }
                else {
                    break;
                }
            } while(0 < gNumRetryAllowances);
        }
    }
    m_isSwitched = isSwitched;
    return ret;
}

RMFResult RMFSDVSrcImpl::close(){
	if(sdvSectionFilter != NULL){	// filter not null indicates this was a switched service session
        SDVSectionFilterRegistrar::getInstance()->stopSectionFilter(sdvSectionFilter);
        delete sdvSectionFilter;
        sdvSectionFilter = NULL;
        
        if(decimalSrcId != 0){
            RMFSDVSrc::forceTuneHandler->unregisterEventHandler(m_sourceInfo.tunerId);

            IARM_SDVAGENT_CLOSE_SESSION_PAYLOAD params;
            params.sourceId = decimalSrcId;
            params.tunerId = m_sourceInfo.tunerId;

            // Agent only notified for closing of switched channel tunes
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[RMFSDVSrcImpl] %s() invoke IARM_BUS_SDVAGENT_API_CLOSE_SESSION\n", __FUNCTION__);
            IARM_Result_t iarmResult = IARM_Bus_Call(IARM_BUS_SDVAGENT_NAME, IARM_BUS_SDVAGENT_API_CLOSE_SESSION, &params, sizeof(IARM_SDVAGENT_CLOSE_SESSION_PAYLOAD));
            if(IARM_RESULT_SUCCESS != iarmResult){
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[RMFSDVSrcImpl] %s() Failed to close SDV Session, with IARM Error %d", __FUNCTION__, iarmResult);
            }
        }
    }
    m_isSwitched = false;
    return RMFQAMSrcImpl::close();
}

RMFResult RMFSDVSrcImpl::getSdvSourceInfo(const char *uri, rmf_ProgramInfo_t* pProgramInfo, uint32_t* pDecimalSrcId) {
	// Prepare IARM Bus message
	IARM_SDVAGENT_OPEN_SESSION_PAYLOAD payload;
	char *sSourceId = strstr((char *) uri, SOURCEID_PRIFIX_STRING);
	*pDecimalSrcId = strtol(sSourceId + strlen(SOURCEID_PRIFIX_STRING), NULL, 16);
	payload.requestInfo.sourceId = *pDecimalSrcId;
	payload.requestInfo.isSdvService = 1;
	g_object_get(src_element, "tunerid", &payload.requestInfo.tunerId, NULL);

	// Initialize status due to IARM_Bus_Call not returning error if sdvAgent is not running
	payload.responseInfo.status = AGENT_NOT_RESPONDING;

	// Invoke Agent over IARM to get tuning info
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[RMFSDVSrcImpl] %s() invoke IARM_BUS_SDVAGENT_API_OPEN_SESSION\n", __FUNCTION__);
    do {
        IARM_Result_t iarmResult = IARM_Bus_Call(IARM_BUS_SDVAGENT_NAME, IARM_BUS_SDVAGENT_API_OPEN_SESSION, &payload, sizeof(IARM_SDVAGENT_OPEN_SESSION_PAYLOAD));
        if (IARM_RESULT_SUCCESS != iarmResult) {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[RMFSDVSrcImpl] %s() IARM_Bus_Call failed with %d\n", __FUNCTION__, iarmResult);
            /* Limited retries per power cycle are allowed if the IARM call fails. It's possible SDV or networking infrastructure is not really ready right now.*/
            if(0 < gNumRetryAllowances) {
                gNumRetryAllowances--;
            }
        }
        else {
            break;
        }
    } while(0 < gNumRetryAllowances);

	// Handle failure replies
	if (payload.responseInfo.status == PROGRAM_SELECT_ERROR_serviceNotSDV) {
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[RMFSDVSrcImpl] %s() Request Program is not an SDV channel\n", __FUNCTION__);
		return RMF_RESULT_INVALID_ARGUMENT;
	} else if (payload.responseInfo.status == AGENT_NOT_RESPONDING) {
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[RMFSDVSrcImpl] %s() Agent failed to respond to IARM call\n", __FUNCTION__);
		return RMF_RESULT_INTERNAL_ERROR;
	} else if (payload.responseInfo.status != PROGRAM_SELECT_CONFIRM_rspOK) {
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[RMFSDVSrcImpl] %s() Failed to get SDV Program info with error %d\n", __FUNCTION__, payload.responseInfo.status);
		return RMF_RESULT_INTERNAL_ERROR;
	}

	// Provide output data to caller
	*pDecimalSrcId = payload.requestInfo.sourceId;
	pProgramInfo->carrier_frequency = payload.responseInfo.carrierFreq;
	pProgramInfo->modulation_mode = getSDVModulationMode(payload.responseInfo.modulationFormat);
	pProgramInfo->prog_num = payload.responseInfo.programNum;
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[RMFSDVSrcImpl] %s() IARM response freq=%d progNum=%d\n", __FUNCTION__, pProgramInfo->carrier_frequency, pProgramInfo->prog_num);

	return RMF_RESULT_SUCCESS;
}

rmf_ModulationMode RMFSDVSrcImpl::getSDVModulationMode(uint32_t modulation){
    rmf_ModulationMode modulation_mode;
    // Convert TWC-SDV_CCMIS modulation to RMF value
    switch(modulation){
    case 8:
        modulation_mode = RMF_MODULATION_QAM64;
        break;
    case 16:
        modulation_mode = RMF_MODULATION_QAM256;
        break;
    default:
        modulation_mode = RMF_MODULATION_UNKNOWN;
        break;
    }
    return modulation_mode;
}

bool RMFSDVSrcImpl::isSdvDiscoveryUri(const char * uri) {
	string val_str;
	return ((strstr(uri, TUNEPARAMS_PRIFIX_STRING) != NULL)  && (get_querystr_val(uri,"sdv", val_str ) == RMF_RESULT_SUCCESS));
}

rmf_osal_Bool RMFSDVSrcImpl::readyForReUse(const char* uri) {
    if(m_isSwitched) {
        /* SDV objects aren't supposed to be cached when they're not in use. RMFQAMSrc::getExistingInstance() will still allow 
         * multiple ACTIVE users to share the same object. */
        return FALSE;
    }
    return RMFQAMSrcImpl::readyForReUse(uri);
}
