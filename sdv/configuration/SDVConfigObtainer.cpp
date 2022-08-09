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
 * ============================================================================
 * @file SDVConfigObtainer.cpp *
 * @brief Implementation of SDVConfigObtainer
 * @Author Jason Bedard jasbedar@cisco.com
 * @Date Aug 28 2014
 */
#include "SDVConfigObtainer.h"
#include "canhClient.h"
#include "podServer.h"
#include "rmf_pod.h"
#include <ctime>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>

#include "rdk_debug.h"
#include "rmf_osal_util.h"
#include "rmf_oobsimgr.h"

#include <sstream>
#include <fstream>

static volatile uint32_t pod_server_cmd_port_no=0;
static volatile uint32_t canh_server_port_no=0;

#define COMMAND_GET_DAC_ID 36
#define COMMAND_GET_CHANNEL_MAP_ID 35

static SDVConfigObtainer * instance = NULL;
/*
 *=============================================================================
 *=============================================================================
 *========================= private structures implementation==================
 *=============================================================================
 *=============================================================================
 */
SDVConfigObtainer::QueuedDownloadEvent::QueuedDownloadEvent(rmf_osal_TimeMillis delay_ms, SDVDownloadThreadEventCallback callback, SDV_CONFIGUTATION_FILE_TYPE arg){
    _timeToFire = getNowTimeInMillis() + delay_ms;
    _callback = callback;
    _arg = arg;
}

void SDVConfigObtainer::SDV_FILE_DATA::dump(){
    char buff[200];
    uint8_t soffset =0;
    memset(buff, '\0', sizeof(buff));
    unsigned char *pc = (unsigned char*)memory;
    for (int i = 0; i < size; i++) {
        if ((i % 16) == 0) {
            if (i != 0){
                RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVConfigObtainer] [SDV_FILE_DATA]  %s\n", buff);
                memset(buff, '\0', sizeof(buff));
                soffset = 0;
            }
        }
        soffset += sprintf (&buff[soffset]," %02x", pc[i]);
    }
    if(size % 16 != 0){
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVConfigObtainer] [SDV_FILE_DATA]  %s\n", buff);
    }
}
bool SDVConfigObtainer::SDV_FILE_DATA::operator== (sdv_file_data& compare) const{
    if(size != compare.size){
        return false;
    }
    for(int i=0;i!=size;++i){
        if(memory[i] != compare.memory[i]){
            return false;
        }
    }
    return true;
}

bool SDVConfigObtainer::QueuedItemComparator::operator()(const SDVConfigObtainer::QueuedDownloadEvent  a, const SDVConfigObtainer::QueuedDownloadEvent  b) const{
    return (a._timeToFire > b._timeToFire);
}

SDVConfigObtainer::SDVFileObserver::SDVFileObserver(SDV_CONFIGUTATION_FILE_TYPE filetype, void * ptrObj, SDVConfigMonitor_t callback){
    _filetype = filetype;
    _ptrObj = ptrObj;
    _callback = callback;
    lastNotifiedData = NULL;
}

/*
 *=============================================================================
 *=============================================================================
 *========================= SDVConfigObtainer implementation==================
 *=============================================================================
 *=============================================================================
 */

/**
 * @addtogroup sdvapi
 * @{
 * @fn SDVConfigObtainer::getInstance()
 * @brief This API is used to get the singleton instance of SDVConfigObtainer. It creates and initialises the
 * SDVConfigObtainer instance if not already created.
 *
 * @return instance Returns pointer to the SDVConfigObtainer instance.
 */
SDVConfigObtainer * SDVConfigObtainer::getInstance(){
    if(instance == NULL){
        instance = new SDVConfigObtainer();
    }
    return instance;
}

SDVConfigObtainer::SDVConfigObtainer() {
    m_running = false;
    downloadFailureCount = 0;
    m_controllerId = 0;
    m_channelMapId = 0;
    m_tuneReadyCallback = NULL;
    m_IPNetworkIsUp = false;
    m_availableConfigFiles = 0;
    m_tuneReadyNotified = false;

    rmf_osal_condNew(FALSE, FALSE, &m_thread_wait_conditional);
    rmf_osal_mutexNew(&m_observerMutex);

    const char *server_port;
    const char *canh_port;

    server_port = rmf_osal_envGet( "POD_SERVER_CMD_PORT_NO" );
    if( NULL == server_port ){
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[SDVConfigObtainer] %s: NULL POINTER  received for name string\n", __FUNCTION__ );
        pod_server_cmd_port_no = POD_SERVER_CMD_DFLT_PORT_NO;
    } else{
        pod_server_cmd_port_no = atoi( server_port );
    }

    canh_port = rmf_osal_envGet( "CANH_SERVER_PORT_NO" );
    if( NULL == canh_port ){
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[SDVConfigObtainer] %s: NULL POINTER  received for name string\n",__FUNCTION__ );
        canh_server_port_no = CANH_SERVER_DFLT_PORT_NO;
    }else{
        canh_server_port_no = atoi( canh_port );
    }
    m_observers[SDVDiscovery] = new SDVFileObserver(SDVDiscovery, NULL, NULL);
    m_observers[SwitchedChannelList] = new SDVFileObserver(SwitchedChannelList, NULL, NULL);
    m_observers[SDVIpv6AddressMap] = new SDVFileObserver(SDVIpv6AddressMap, NULL, NULL);

    m_persistentTemplate[SDVDiscovery] = PERSISTANT_SDV_DISCOVERY_FILE_NAME;
    m_persistentTemplate[SwitchedChannelList] = PERSISTANT_SDV_SERVICE_LIST_FILE_NAME;
    m_persistentTemplate[SDVIpv6AddressMap] = PERSISTANT_SDV_IPV_ADDRESS_FILE_NAME;
}

SDVConfigObtainer::~SDVConfigObtainer() {
    rmf_osal_condDelete(m_thread_wait_conditional);
}

/**
 * @fn SDVConfigObtainer::start()
 * @brief This API sets the path for persisting SDV configuration files and creates a SDVDownloadThread
 * for execution of download related tasks. It also sets a flag to indicate that the SDVDownloadThread
 * is in active/running state.
 *
 * @return None
 */
void SDVConfigObtainer::start(){
    m_running = true;
    discoverPersistentPath();
    rmf_osal_threadCreate( downloadThread, this, RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE, &m_threadId, "SDVDownloadThread");
}

void SDVConfigObtainer::stop(){
    m_running = false;
    rmf_osal_condSet(m_thread_wait_conditional);     // awaken doLoop thread in case it was sleeping
}

/**
 * @fn SDVConfigObtainer::setObserver(SDV_CONFIGUTATION_FILE_TYPE filetype,void * ptrObj, SDVConfigMonitor_t callback)
 * @brief This API is used to set an observer for the specified file type by registering the callback function. If the
 * configuration file of specified filetype is already downloaded then it immediately notifies the observer by calling
 * the registered callback function.
 *
 * @param[in] filetype Indicates the SDV configuration file type the observer is interested in.
 * <ul>
 * <li> 0 indicates SDV discovery map file.
 * <li> 1 indicates SDV channel map file.
 * </ul>
 * @param[in] ptrObj Pointer to instance of observer. A NULL can be passed if its a singleton instance.
 * @param[in] callback Pointer to callback function.
 *
 * @return None
 * @}
 */
void SDVConfigObtainer::setObserver(SDV_CONFIGUTATION_FILE_TYPE filetype,void * ptrObj, SDVConfigMonitor_t callback){
    rmf_osal_mutexAcquire(m_observerMutex);

    SDVFileObserver * observer = m_observers[filetype];
    observer->_callback = callback;
    observer->_ptrObj = ptrObj;

    //If File has already been downloaded
    if( observer->lastNotifiedData != NULL){
        observer->_callback(observer->_ptrObj, (uint8_t *) observer->lastNotifiedData->memory, observer->lastNotifiedData->size);
    }
    rmf_osal_mutexRelease(m_observerMutex);
}

void SDVConfigObtainer::registerSDVTuneReadyCallback(SDVReadyCallback_t callback){
	m_tuneReadyCallback = callback;
}

/* SDV is ready to go when IP connectivity is through and SDVDiscovery and SwitchedChannelList configuration files are available.*/
void SDVConfigObtainer::checkAndNotifyTuneReadiness(SDV_CONFIGUTATION_FILE_TYPE downloadedFiletype){
    static const uint8_t configFileTarget = (0x01 << SDVDiscovery) | (0x01 << SwitchedChannelList);
    if(m_tuneReadyNotified){
        return;
    }
    m_availableConfigFiles |= (0x01 << downloadedFiletype);

    if(m_IPNetworkIsUp && (configFileTarget == (m_availableConfigFiles & configFileTarget))){
        if(m_tuneReadyCallback){
            RDK_LOG( RDK_LOG_ERROR,  "LOG.RDK.QAMSRC","[SDVConfigObtainer] reporting tune ready.\n" );
            m_tuneReadyCallback();
            m_tuneReadyNotified = true;
        }
    }
}
SDVConfigObtainer::sdvTemplateAcquisitionResult_t SDVConfigObtainer::acquireUrlTemplates() {
    m_templates[SDVDiscovery] = "";
    m_templates[SwitchedChannelList] = "";
    m_templates[SDVIpv6AddressMap] = "";
    
    sdvTemplateAcquisitionResult_t result = updateURLTemplatesFromFileSystem();
    if(RESULT_TEMPLATE_ACQUISITION_SUCCESS == result){
        return result;
    }else {
        return updateURLTemplatesFromAuthService();
    }
}


/*
 *=============================================================================
 *=============================================================================
 *===========================Wild Card Obtaining Methods=======================
 *=============================================================================
 *=============================================================================
 */

rmf_Error SDVConfigObtainer::getDeviceIds(uint16_t * controllerID, uint16_t * channelMapId, uint16_t * vctId){
    int32_t result_recv;
    uint16_t pod_controllerId = 0;
    uint16_t pod_channelMapId = 0;
    uint16_t pod_vctId = 0;

    IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_CHANNEL_MAPID );
    rmf_Error error = runPODCommand(ipcBuf);
    if(RMF_SUCCESS != error){
        return error;
    }
    result_recv |= ipcBuf.collectData( &pod_channelMapId, sizeof( pod_channelMapId ) );

    ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_DACID );
    error = runPODCommand(ipcBuf);
    if(RMF_SUCCESS != error){
        return error;
    }
    result_recv |= ipcBuf.collectData( &pod_controllerId, sizeof( pod_controllerId ) );

    ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_VCT_ID );
    error = runPODCommand(ipcBuf);
    if(RMF_SUCCESS != error){
        return error;
    }
    result_recv |= ipcBuf.collectData( &pod_vctId, sizeof( pod_controllerId ) );

    if(result_recv < 0){
        return RMF_OSAL_ENODATA;
    }else{
        *controllerID = pod_controllerId;
        *channelMapId = pod_channelMapId;
        *vctId = pod_vctId;
        return RMF_SUCCESS;
    }
}

rmf_Error SDVConfigObtainer::runPODCommand(IPCBuf& ipcBuf){
    rmf_Error isReady = rmf_podmgrIsReady();
    if(RMF_SUCCESS != isReady){
        return RMF_OSAL_ENODATA;
    }

    uint32_t res = ipcBuf.sendCmd( pod_server_cmd_port_no  );
    if ( 0 != res ){
        RDK_LOG( RDK_LOG_ERROR,  "LOG.RDK.QAMSRC","[SDVConfigObtainer] POD Comm failure in %s:%d: ret = %d\n", __FUNCTION__, __LINE__, res );
        return RMF_OSAL_ENODATA;
    }
    uint32_t response;
    res = ipcBuf.getResponse( &response );
    if( res <= 0){
        RDK_LOG( RDK_LOG_ERROR,  "LOG.RDK.QAMSRC","[SDVConfigObtainer] POD Comm get response failed in %s:%d ret = %d\n",__FUNCTION__, __LINE__, res );
        return RMF_OSAL_ENODATA;
    }
    return RMF_SUCCESS;
}

/*
 *=============================================================================
 *=============================================================================
 *=========================== DOWNLOAD THREAD METHODS==========================
 *=============================================================================
 *=============================================================================
 */
void SDVConfigObtainer::downloadThread(void * obj){
    //Thread safe since access to queue is only done within queue thread
    QueuedDownloadEvent  event = QueuedDownloadEvent( 0l,  updateWildcardsEventAction, UnknownFile );
    instance->_queuedEvents.push(event);
    rmf_osal_TimeMillis now = getNowTimeInMillis();
    while(instance->m_running){

        QueuedDownloadEvent  event = instance->_queuedEvents.top();
        RDK_LOG( RDK_LOG_DEBUG,  "LOG.RDK.QAMSRC","[SDVConfigObtainer] %s: Pop event from queue, now time=%llu timeToFire=%llu\n", __FUNCTION__, now, event._timeToFire);
        instance->_queuedEvents.pop();

        if(now < event._timeToFire){
            uint32_t remaining = event._timeToFire - now;
            RDK_LOG( RDK_LOG_DEBUG,  "LOG.RDK.QAMSRC","[SDVConfigObtainer] %s: Wait %d for event to fire\n", __FUNCTION__, remaining);
            rmf_osal_condWaitFor(instance->m_thread_wait_conditional, remaining);
        }

        RDK_LOG( RDK_LOG_DEBUG,  "LOG.RDK.QAMSRC","[SDVConfigObtainer] %s: Invoke action for event\n", __FUNCTION__);
        uint32_t next_delay = event._callback(event._arg);

        now = getNowTimeInMillis();
        event._timeToFire = (next_delay + now);
        RDK_LOG( RDK_LOG_DEBUG,  "LOG.RDK.QAMSRC","[SDVConfigObtainer] %s: Push event to queue with delay=%d timeToFire=%llu\n", __FUNCTION__, next_delay, event._timeToFire);
        instance->_queuedEvents.push(event);    // sorts queue by timeToFire
    }
}

uint32_t SDVConfigObtainer::downloadFileAction(SDV_CONFIGUTATION_FILE_TYPE fileType){
    RDK_LOG( RDK_LOG_INFO,  "LOG.RDK.QAMSRC","[SDVConfigObtainer] %s: file Type = %d\n", __FUNCTION__, fileType );

    SDV_FILE_DATA * chunk = new SDV_FILE_DATA();
    chunk->memory = (char*)malloc(sizeof(char));  /* will be grown as needed by writeFileToMemoryCallback */
    chunk->size = 0;    /* no data at this point */

    std::string sURL = instance->m_templates[fileType];

    if(sURL.empty()){
        RDK_LOG( RDK_LOG_INFO,  "LOG.RDK.QAMSRC","[SDVConfigObtainer] %s:  URL for file type %d is NULL.  Will skip file \n", __FUNCTION__, fileType );
        return REFRESH_BACKOFF_MAX;
    }

    replaceWildcards(sURL);
    RDK_LOG( RDK_LOG_INFO,  "LOG.RDK.QAMSRC","[SDVConfigObtainer] %s: complete URL = %s\n", __FUNCTION__, sURL.c_str() );
    CURLcode result = readFileHTTP(sURL, chunk);
    if(result == CURLE_OK){
        if(instance->notifyObserver(fileType, chunk, false) == RMF_SUCCESS ){
            instance->downloadFailureCount = 0;
            return instance->getRandomDelay(REFRESH_BACKOFF_MIN, REFRESH_BACKOFF_MAX);
        }else{
            instance->downloadFailureCount++;
            return instance->getRandomDelay(RETRY_BACKOFF_MIN, RETRY_BACKOFF_MAX);
        }
    }else if(instance->readPersistentFile(fileType, chunk) == RMF_SUCCESS){
        instance->notifyObserver(fileType, chunk, true);
    }else{
        delete chunk;
    }

    if(result == CURLE_COULDNT_RESOLVE_HOST || result == CURLE_COULDNT_CONNECT){
        return RETRY_BACK_OFF_CONNECTION_ERROR;
    }else{
        instance->downloadFailureCount++;
        return instance->getRandomDelay(RETRY_BACKOFF_MIN, RETRY_BACKOFF_MAX);
    }
}

uint32_t SDVConfigObtainer::updateWildcardsEventAction(SDV_CONFIGUTATION_FILE_TYPE  arg){
    uint16_t controllerId;
    uint16_t channelMapId;
    uint16_t vctid;

    rmf_Error err = instance->getDeviceIds(&controllerId, &channelMapId, &vctid);
    //If we cannot get the device id values schedule a retry
    if(RMF_SUCCESS != err){
        RDK_LOG( RDK_LOG_ERROR,  "LOG.RDK.QAMSRC","[SDVConfigObtainer] %s() getDeviceIds failed\n",__FUNCTION__);
        return DEVICE_ID_RETRY_INTERVAL;
    }
    //The device ID has changed update our value and reschedule any pending file downloads action to take place immediatly
    if( controllerId != instance->m_controllerId || channelMapId != instance->m_channelMapId || vctid != instance->m_vctId){
        RDK_LOG( RDK_LOG_INFO,  "LOG.RDK.QAMSRC","[SDVConfigObtainer] %s() detected change to wild card values controllerId = %d channelMapId = %d vctid = %d \n",
                __FUNCTION__,controllerId, channelMapId, vctid );

        while(!instance->_queuedEvents.empty()){//Remove any pending download events.
            instance->_queuedEvents.pop();
        }
        // Queue action to download configuration files
        instance->m_controllerId = controllerId;
        instance->m_channelMapId = channelMapId;
        instance->m_vctId = vctid;
        instance->_queuedEvents.push( QueuedDownloadEvent ( 0l, downloadFileAction, SDVDiscovery ));
        instance->_queuedEvents.push( QueuedDownloadEvent ( 0l, downloadFileAction, SwitchedChannelList ));
        instance->_queuedEvents.push( QueuedDownloadEvent ( 0l, downloadFileAction, SDVIpv6AddressMap ));
    } else{
        RDK_LOG( RDK_LOG_INFO,  "LOG.RDK.QAMSRC","[SDVConfigObtainer] %s() no changes detected\n", __FUNCTION__);

    }
    return DEVICE_ID_REFRESH_INTERVAL;
}

SDVConfigObtainer::sdvTemplateAcquisitionResult_t SDVConfigObtainer::updateURLTemplatesFromAuthService(){
    bool discoveryUrlFound = FALSE;
    bool serviceListUrlFound = FALSE;
    bool notifyCommFailure = FALSE;
    SDV_FILE_DATA * chunk = new SDV_FILE_DATA();


    chunk->memory = (char*)malloc(sizeof(char));  /* will be grown as needed by writeFileToMemoryCallback */
    chunk->size = 0;    /* no data at this point */

    if (postToHTTP(chunk, SDV_AUTHSERVICE_POST_URL, SDV_DISCOVERY_FILE_POST_FIELD) == CURLE_OK) {
        if (chunk->size != 0) {
            m_templates[SDVDiscovery] = std::string(chunk->memory, chunk->size);
            discoveryUrlFound = TRUE;
            RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVConfigObtainer] %s() m_templates[SDVDiscovery] = \"%s\"\n", __FUNCTION__, m_templates[SDVDiscovery].c_str() );
        } else {
            RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVConfigObtainer] %s() no data found for field %s\n", __FUNCTION__, SDV_DISCOVERY_FILE_POST_FIELD);
        }
    } else {
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[SDVConfigObtainer] %s() postToHTTP failed for URL %s with field %s\n", __FUNCTION__, SDV_AUTHSERVICE_POST_URL, SDV_DISCOVERY_FILE_POST_FIELD);
        notifyCommFailure = TRUE;
    }
    free (chunk->memory);

    // No sense attempting to get service list URL if discovery URL was not found
    if (discoveryUrlFound) {
        chunk->memory = (char*)malloc(sizeof(char));  /* will be grown as needed by writeFileToMemoryCallback */
        chunk->size = 0;
        if (postToHTTP(chunk, SDV_AUTHSERVICE_POST_URL, SDV_SERVICE_LIST_POST_FIELD ) == CURLE_OK) {
            if (chunk->size != 0) {
                m_templates[SwitchedChannelList] = std::string(chunk->memory, chunk->size);
                serviceListUrlFound = TRUE;
                RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVConfigObtainer] %s() m_templates[SwitchedChannelList] = \"%s\"\n", __FUNCTION__, m_templates[SwitchedChannelList].c_str() );
            } else {
                RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVConfigObtainer] %s() no data found for field %s\n", __FUNCTION__, SDV_SERVICE_LIST_POST_FIELD);
            }
        } else {
            RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[SDVConfigObtainer] %s() postToHTTP failed for URL %s with field %s\n", __FUNCTION__, SDV_AUTHSERVICE_POST_URL, SDV_SERVICE_LIST_POST_FIELD);
            notifyCommFailure = TRUE;
        }
        free (chunk->memory);

        //Now update the template url for the SDV address map file
        chunk->memory = (char*)malloc(sizeof(char));  /* will be grown as needed by writeFileToMemoryCallback */
        chunk->size = 0;
        if (postToHTTP(chunk, SDV_AUTHSERVICE_POST_URL, SDV_IPV_ADDRESS_POST_FIELD ) == CURLE_OK) {
            if (chunk->size != 0) {
                m_templates[SDVIpv6AddressMap] = std::string(chunk->memory, chunk->size);
                RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVConfigObtainer] %s() m_templates[SDVIpv6AddressMap] = \"%s\"\n", __FUNCTION__, m_templates[SDVIpv6AddressMap].c_str() );
            } else {
                RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVConfigObtainer] %s() no data found for field %s\n", __FUNCTION__, SDV_IPV_ADDRESS_POST_FIELD);
            }
        } else {
            RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[SDVConfigObtainer] %s() postToHTTP failed for URL %s with field %s\n", __FUNCTION__, SDV_AUTHSERVICE_POST_URL, SDV_IPV_ADDRESS_POST_FIELD);
            notifyCommFailure = TRUE;
        }
        free (chunk->memory);
    }

    delete chunk;
    if (notifyCommFailure) {
        return RESULT_TEMPLATE_ACQUISITION_COMM_FAILURE;
    }
    else if (discoveryUrlFound && serviceListUrlFound) {	// No need to consider the SDVIpv6AddressMap file since it is optional
        return RESULT_TEMPLATE_ACQUISITION_SUCCESS;
    }
    else {
        return RESULT_TEMPLATE_ACQUISITION_FAILURE;
    }
}

SDVConfigObtainer::sdvTemplateAcquisitionResult_t SDVConfigObtainer::updateURLTemplatesFromFileSystem(){
    ifstream infile;

    bool discoveryUrlFound = FALSE;
    bool serviceListUrlFound = FALSE;

    infile.open (SDV_TEMPLATE_FILE_PATH, ifstream::in);
    if(infile.is_open()){
       std::string line;
       std::string discoveryFileName = SDV_DISCOVERY_FILE_NAME;
       std::string serviceListFileName  = SDV_SERVICE_LIST_FILE_NAME;
       std::string addressMapFileName  = SDV_ADDRESS_MAP_FILE_NAME;

        while (infile.good() && getline(infile, line)){
            if(line.find(discoveryFileName) == 0){
                discoveryUrlFound  = TRUE;
                m_templates[SDVDiscovery] = line.substr(discoveryFileName.size(), line.size() - discoveryFileName.size());
                RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVConfigObtainer] %s():  m_templates[SDVDiscovery] = \"%s\"\n", __FUNCTION__, m_templates[SDVDiscovery].c_str() );
            }else  if(line.find(serviceListFileName) == 0){
                serviceListUrlFound = TRUE;
                m_templates[SwitchedChannelList] = line.substr(serviceListFileName.size(), line.size() - serviceListFileName.size());
                RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVConfigObtainer] %s():  m_templates[SwitchedChannelList] = \"%s\"\n", __FUNCTION__, m_templates[SwitchedChannelList].c_str() );
            }else  if(line.find(addressMapFileName) == 0){
                m_templates[SDVIpv6AddressMap] = line.substr(addressMapFileName.size(), line.size() - addressMapFileName.size());
                RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVConfigObtainer] %s():  m_templates[SDVIpv6AddressMap] = \"%s\"\n", __FUNCTION__, m_templates[SDVIpv6AddressMap].c_str() );
            }else{
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[SDVConfigObtainer] %s():  unknown configuration file\n", __FUNCTION__ );
            }
        }
        infile.close();
    }else{
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVConfigObtainer] %s(): Template file[%s] not found\n", __FUNCTION__, SDV_TEMPLATE_FILE_PATH );
    }

    //Considered success if these to files are found.  SDVIpv6AddressMap is an optional file.
    if (discoveryUrlFound && serviceListUrlFound) {	
        return RESULT_TEMPLATE_ACQUISITION_SUCCESS;
    }
    else {
        return RESULT_TEMPLATE_ACQUISITION_FAILURE;
    }
}


CURLcode SDVConfigObtainer::postToHTTP(SDV_FILE_DATA * chunk, const char * url, const char * field){
    CURL *curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_POST, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, writeFileToMemoryCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)chunk);
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, field);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    curl_easy_setopt(curl_handle, CURLOPT_FAILONERROR, true);

    CURLcode res = curl_easy_perform(curl_handle);
    if(res != CURLE_OK) {
        RDK_LOG( RDK_LOG_ERROR,  "LOG.RDK.QAMSRC","[SDVConfigObtainer] %s() curl_easy_perform() error: = %s\n",__FUNCTION__, curl_easy_strerror(res) );
    }
    else {
        RDK_LOG( RDK_LOG_INFO,  "LOG.RDK.QAMSRC","[SDVConfigObtainer] %s() %lu bytes retrieved\n",__FUNCTION__, (long)chunk->size );
        chunk->dump();
    }
    curl_easy_cleanup(curl_handle);
    return res;
}

void SDVConfigObtainer::replaceWildcards(std::string & str){
    instance->replace(str, "{controller_id}", instance->m_controllerId);
    instance->replace(str, "{channelmap_id}", instance->m_channelMapId);
    instance->replace(str, "{hub_id}", instance->m_vctId);
}

void SDVConfigObtainer::replace(std::string& str, const std::string& token, uint16_t value){
    char svalue[6];
    snprintf (svalue, sizeof(svalue), "%d", value);
    size_t start_pos = str.find(token);
    if(start_pos != std::string::npos){
        str.replace(start_pos, token.length(), svalue);
    }
}

CURLcode SDVConfigObtainer::readFileHTTP(std::string &url, SDV_FILE_DATA * chunk ){
    RDK_LOG( RDK_LOG_INFO,  "LOG.RDK.QAMSRC","[SDVConfigObtainer] %s() url = %s\n",__FUNCTION__, url.c_str() );

    CURL *curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, writeFileToMemoryCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)chunk);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    curl_easy_setopt(curl_handle, CURLOPT_FAILONERROR, true);

    CURLcode res = curl_easy_perform(curl_handle);
    if(res != CURLE_OK) {
        RDK_LOG( RDK_LOG_ERROR,  "LOG.RDK.QAMSRC","[SDVConfigObtainer] %s() curl_easy_perform() error: = %s\n",__FUNCTION__, curl_easy_strerror(res) );
    }
    else {
        RDK_LOG( RDK_LOG_INFO,  "LOG.RDK.QAMSRC","[SDVConfigObtainer] %s() %lu bytes retrieved\n",__FUNCTION__, (long)chunk->size );
        chunk->dump();
    }
    curl_easy_cleanup(curl_handle);
    return res;
}

uint32_t SDVConfigObtainer::getRandomDelay(uint32_t min, uint32_t max){
    if(downloadFailureCount > 0){
        uint32_t modifer = std::min((uint32_t)(BACKOFF_MULTIPLIER * downloadFailureCount), (uint32_t)BACKOFF_MULTIPLIER_MAX);
        uint32_t modified_min = modified_min + ((min * modifer) /100);
        uint32_t modified_max = modified_max + ((max * modifer) /100);
    }
    uint32_t result =  ((rand() % (max - min)) + min);
    RDK_LOG( RDK_LOG_INFO,  "LOG.RDK.QAMSRC","[SDVConfigObtainer] %s()  result = % d\n" ,__FUNCTION__, result );

    return result;
}

void SDVConfigObtainer::discoverPersistentPath(){
    ifstream infile;

    infile.open (PERSISTANT_LOCATION_CONFIG_FILE_PATH_, ifstream::in);
    std::string line;
    uint32_t field_len = strlen(PERSISTANT_LOCATION_CONFIG_FILE_FIELD);
    m_persistentPath = "";
    if(infile.is_open()){
        while (infile.good() && getline(infile, line)){
            RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVConfigObtainer] %s(): line = \"%s\"\n", __FUNCTION__, line.c_str() );
            if(line.find(PERSISTANT_LOCATION_CONFIG_FILE_FIELD) == 0){
                m_persistentPath = line.substr(field_len, line.size() - field_len) + PERSISTANT_SDV_FOLDER_NAME;
                RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVConfigObtainer] %s():  m_persistentPath = %ss\n", __FUNCTION__, m_persistentPath.c_str() );
                break;
            }
        }
        infile.close();
    }else{
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[SDVConfigObtainer] %s(): could not open %s = \n", __FUNCTION__, PERSISTANT_LOCATION_CONFIG_FILE_PATH_ );
    }
}

void SDVConfigObtainer::writePersistentFile(SDV_CONFIGUTATION_FILE_TYPE filetype, SDV_FILE_DATA * data){
    if(m_persistentPath.size() == 0){
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[SDVConfigObtainer] %s(): persistent path not set. aborting write \n", __FUNCTION__ );
        return;
    }
    //Make sure folder exists or has been deleted
    struct stat st = {0};
    if(stat(m_persistentPath.c_str(), &st) == -1){
        if(mkdir(m_persistentPath.c_str(), 0700) != 0){
            RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[SDVConfigObtainer] %s(): could not create sdv config directory %s = \n", __FUNCTION__, m_persistentPath.c_str() );
            m_persistentPath="";
            return;
        }else{
            RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVConfigObtainer] %s(): created folder= %s \n", __FUNCTION__, m_persistentPath.c_str() );
        }
    }
    std::string name =m_persistentTemplate[filetype];
    replaceWildcards(name);
    std::string path = m_persistentPath + "/" + name;
    FILE * fp = fopen(path.c_str(), "w+");
    if(fp == NULL){
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[SDVConfigObtainer] %s(): could not create open file for writing: %s\n", __FUNCTION__, path.c_str() );
        return;
    }
    uint32_t count = fwrite( (void*)data->memory, sizeof(uint8_t), data->size, fp);
    fclose(fp);
    if(count != data->size){
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[SDVConfigObtainer] %s(): only %d out of %d bytes written.  File will be deleted \n", __FUNCTION__, count, data->size );
        remove(path.c_str());
    }else{
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVConfigObtainer] %s(): saved file  = %s \n", __FUNCTION__, path.c_str() );
    }
}

rmf_Error SDVConfigObtainer::readPersistentFile(SDV_CONFIGUTATION_FILE_TYPE filetype, SDV_FILE_DATA * data){
    if(m_persistentPath.size() == 0){
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[SDVConfigObtainer] %s(): no persistent path found. Aborting read\n", __FUNCTION__ );
        return RMF_OSAL_ENODATA;
    }

    std::string name =m_persistentTemplate[filetype];
    replaceWildcards(name);
    std::string path = m_persistentPath + name;

    FILE * fp = fopen(path.c_str(), "r");
    if(fp == NULL){
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[SDVConfigObtainer] %s(): no persistent SDV Configuration file found @ %s\n", __FUNCTION__,path.c_str() );
        return RMF_OSAL_ENODATA;
    }
    fseek(fp, 0L, SEEK_END);
    uint32_t fileSize = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVConfigObtainer] %s(): persistent file size = %d \n", __FUNCTION__,fileSize );

    data->memory = (char*) realloc(data->memory,sizeof(char) * fileSize);
    data->size = fileSize;
    int read_count = fread(data->memory, sizeof(char), data->size, fp);
    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVConfigObtainer] %s(): %d bytes read from persistent file\n", __FUNCTION__,read_count );

    fclose(fp);

    return fileSize == read_count? RMF_SUCCESS: RMF_OSAL_ENODATA;
}

size_t SDVConfigObtainer::writeFileToMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp){
    size_t realsize = size * nmemb;
    SDV_FILE_DATA *mem = (SDV_FILE_DATA *)userp;
    if(0 != realsize)
    {
        mem->memory = (char*) realloc(mem->memory,sizeof(char) * ( mem->size + realsize + 1));
        if(mem->memory == NULL) {
            RDK_LOG( RDK_LOG_ERROR,  "LOG.RDK.QAMSRC","[SDVConfigObtainer] %s() not eneogh memory to download file\n",__FUNCTION__ );
            return 0;
        }

        memcpy(&(mem->memory[mem->size]), contents, realsize);
        mem->size += realsize;
        mem->memory[mem->size] = 0;
    }
    return realsize;
}

rmf_Error SDVConfigObtainer::notifyObserver(SDV_CONFIGUTATION_FILE_TYPE filetype, SDV_FILE_DATA * data, bool isCachedCopy){
    rmf_osal_mutexAcquire(m_observerMutex);
    RDK_LOG( RDK_LOG_INFO,  "LOG.RDK.QAMSRC","[SDVConfigObtainer] %s() fileType = %d\n",__FUNCTION__ , filetype);

    rmf_Error result  = RMF_SUCCESS;
    if(!isCachedCopy){
        m_IPNetworkIsUp = true;
    }
    checkAndNotifyTuneReadiness(filetype);
    SDVFileObserver * observer = m_observers[filetype];
    if(observer == NULL){
        RDK_LOG( RDK_LOG_ERROR,  "LOG.RDK.QAMSRC","[SDVConfigObtainer] %s() No Observer to notify \n", __FUNCTION__);
        return result;
    }

    bool notify = true;
    if(observer->lastNotifiedData != NULL){
        if(*observer->lastNotifiedData == *data){
            notify = false;
            RDK_LOG( RDK_LOG_INFO,  "LOG.RDK.QAMSRC","[SDVConfigObtainer] %s() file has not changed.  Observer will not be notified\n",__FUNCTION__);
        }else{
            RDK_LOG( RDK_LOG_INFO,  "LOG.RDK.QAMSRC","[SDVConfigObtainer] %s() file has changed\n",__FUNCTION__);
            free(observer->lastNotifiedData->memory);
            delete observer->lastNotifiedData;
            observer->lastNotifiedData = data;
        }
    }else{
        RDK_LOG( RDK_LOG_INFO,  "LOG.RDK.QAMSRC","[SDVConfigObtainer] %s() file %d downloaded for first time\n",__FUNCTION__, filetype);
    }
    if(notify){
        writePersistentFile(filetype, data);
        observer->lastNotifiedData = data;
        result = observer->_callback(observer->_ptrObj, (uint8_t*)data->memory, data->size);
        RDK_LOG( RDK_LOG_INFO,  "LOG.RDK.QAMSRC","[SDVConfigObtainer] %s() observer notified and returned result \n",__FUNCTION__, result);
    }else{
        free(data->memory);
        delete data;
    }
    rmf_osal_mutexRelease(m_observerMutex);
    return result;
}

rmf_osal_TimeMillis SDVConfigObtainer::getNowTimeInMillis() {
    rmf_osal_TimeMillis time;
    rmf_osal_timeGetMillis(&time);
    return time;
}
