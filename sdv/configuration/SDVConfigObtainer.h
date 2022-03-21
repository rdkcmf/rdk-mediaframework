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
 * @file SDVConfigObtainer.h
 *
 * @brief Implementation of Class for obtaining MSO Specific configuration information needed to tune to
 * switched channels.
 *
 * @Author Jason Bedard jasbedar@cisco.com
 * @Date Aug 28 2014
 */

/**
 * @defgroup sdv SDV (Switched Digital Video)
 * SDV that enables an MSO to dynamically switch video services on and off based on user activities.
 * This is achieved by providing only the services which the user more frequently uses and on demand
 * other services are provided. This will indirectly save the bandwidth and their by increase in
 * available services to the users.
 * @n @n
 * In RDK this is achieved through the client server architecture where the client will be executed
 * on the device and the server will on be on headend. The SDV Client agent will update the user
 * activities, tuner request, error details, etc to the SDV server.
 * @image html sdv.jpg
 *
 * @par SDV Agent
 * - This component is responsible for interacting with the SDV server, and providing information back 
 * to the media framework for tuning operations
 * - Interacts with Media Framework and SDV Discovery module over IARM
 *
 * @par Media Framework:
 * - Initialize SDV Discovery
 * - QAMSource open method to perform auto-discovery if needed
 * - New extended class SDVQAMSource that handles all the interactions with the SDV Client
 *   
 * @par RMFSDVManager
 * - Initializes SDVDiscovery and SDV Configuration Sub services within the media streamer memory space.
 *
 * @par SDV Discovery module
 * - Initialize SDV Agent modules
 * - Manages tune time MPC Discovery operations and provided configuration to SDVAgent over iARM.
 *   
 * @par SDV Configuration
 * - Loading/storing the configuration files (Discovery frequencies/ SDV Channel lineup)
 * 
 * @ingroup RMF
 *
 * @defgroup sdvapi SDV API list
 * Describe the details about Switched Digital Video (SDV) API specifications.
 * @ingroup sdv
 *
 * @defgroup sdvclass SDV Classes
 * @ingroup sdv
*/

#ifndef SDVCONFIGOBTAINER_H_
#define SDVCONFIGOBTAINER_H_

#include "rmf_osal_ipc.h"
#include "rmf_osal_thread.h"
#include "rmf_osal_sync.h"
#include "rmf_osal_time.h"
#include "curl/curl.h"

#include <queue>
#include <map>
#include <string>

#define SDV_TEMPLATE_FILE_PATH "/opt/sdvconfig.txt"
#define SDV_DISCOVERY_FILE_NAME "SDVAutodiscovery "
#define SDV_SERVICE_LIST_FILE_NAME "SDVSwitchedChannelMap "
#define SDV_ADDRESS_MAP_FILE_NAME "SDVIPAddressMap "

#define SDV_AUTHSERVICE_POST_URL "http://127.0.0.1:50050/authService/getBootstrapProperty"
#define SDV_DISCOVERY_FILE_POST_FIELD "sdvAutoDiscoveryURL"
#define SDV_SERVICE_LIST_POST_FIELD "sdvSwitchedListURL"
#define SDV_IPV_ADDRESS_POST_FIELD "sdvIPV6AddressMapURL"

#define PERSISTANT_SDV_FOLDER_NAME              "/sdv-persistent/"
#define PERSISTANT_LOCATION_CONFIG_FILE_PATH_   "/etc/include.properties"
#define PERSISTANT_LOCATION_CONFIG_FILE_FIELD   "PERSISTENT_PATH="
#define PERSISTANT_SDV_DISCOVERY_FILE_NAME      "SDVAutodiscovery_{controller_id}"
#define PERSISTANT_SDV_SERVICE_LIST_FILE_NAME   "SDVSwitchedChannelMap_{controller_id}_{channelmap_id}"
#define PERSISTANT_SDV_IPV_ADDRESS_FILE_NAME    "SDVIPVAddress_{controller_id}.xml"

#define REFRESH_BACKOFF_MIN 1800000  //!< Minimum amount of time to wait before refreshing a file [30 minutes]
#define REFRESH_BACKOFF_MAX 7200000  //!< Maximum amount of time to wait before refreshing a file [2 hours]

#define RETRY_BACK_OFF_CONNECTION_ERROR 10000  //!< Amount of time to wait before retry when download failed because non HTTP error [10 seconds]

//These will be effected by degrading backoff if multiple failures
#define RETRY_BACKOFF_MIN 20000   //!< Minimum amount of time to wait before retrying a file download [20 seconds]
#define RETRY_BACKOFF_MAX 120000  //!< Maximum amount of time to wait before retrying a file download [2 minutes]

#define BACKOFF_MULTIPLIER 20   //!< Base value for multiplier used for degrading backoff [ 20 percent]
#define BACKOFF_MULTIPLIER_MAX 200 //!< Max value for multiplier used for degrading backoff [ 200 percent]

#define DEVICE_ID_REFRESH_INTERVAL  60000  //!< Refresh interval time for device ID. It is set to 60 seconds.
#define DEVICE_ID_RETRY_INTERVAL    15000  //!< Retry interval time in milliseconds to get the device ID. It is set to 15 seconds.

/**
 * @class SDVConfigObtainer
 *
 * @brief Manages the downloading of SDV Configuration files and exposes
 * an observer API for sub modules to get the contents of specific files.
 *
 * Files will be downloaded based on URL template where wildcards in the URL will
 * be replaced with system variables. Supported wildcards are.
 *  {controllerId} : replaced with the controller(DAC?DNCS) ID
 * @ingroup sdvclass
 *
 */
class SDVConfigObtainer {
public:
    static SDVConfigObtainer * getInstance();

    /**
     * @enum SDV_CONFIGUTATION_FILE_TYPE
     * @brief Enumeration of SDV File types
     */
    typedef enum sdv_file_type{
        SDVDiscovery = 0,               	//!<Autodiscovery configuration file
        SwitchedChannelList,              	//!<Switched Channel list configuration file
        SDVIpv6AddressMap,					//!<Map of IPV4-IPV6 addresses to support IPV6 on MCMIC 2.1
        UnknownFile
    }SDV_CONFIGUTATION_FILE_TYPE;

    typedef enum 
    {
        RESULT_TEMPLATE_ACQUISITION_SUCCESS = 0,
        RESULT_TEMPLATE_ACQUISITION_FAILURE,
        RESULT_TEMPLATE_ACQUISITION_COMM_FAILURE
    }sdvTemplateAcquisitionResult_t;

    /** @brief File Download notification callback template
     * Template of callback function for SDV configuration file monitors.
     *
     * @param [in] ptrObj pointer to instance of observer or NULL if static
     * @param [in] data binary contents of configuration file
     * @param [in] size of configuration file in bytes
     *
     * @return RMF_SUCCESS if file is well formed. RMF_OSAL_ENODATA if file is corrupted
     */
    typedef rmf_Error (*SDVConfigMonitor_t) (void * ptrObj, uint8_t *data, uint32_t size);
    typedef void (*SDVReadyCallback_t)(void);

    /**
     * @struct SDV_FILE_DATA
     * @brief Representation of a chuck of memory which stores an SDV configuration file.
     *  ==  operator is overloaded to compare two different chucks of memory for equality.
     */
    typedef struct sdv_file_data{
        char *memory;
        size_t size;
        void dump();
        bool operator== (sdv_file_data& compare) const;
    }SDV_FILE_DATA;

    virtual ~SDVConfigObtainer();

    void start();

    /**
     * @fn stop
     *
     * @brief Stops all in progress downloads and prevent any future download retry or refresh operations
     */
    void stop();

    /**
     * @fn setCongurationObserver
     *
     * @brief Static callback for handling force tune events.
     *
     * If file is already available observer will be immediately notified.
     * Only a single observer can be set per file type.  To remove observer provide
     * a NULL callback function pointer.
     *
     * @param [in] filetype for which file the observer is interested in.
     * @param [in] ptrObj pointer to instance of observer or NULL is static
     * @param [in] callback pointer to callback function.
     */
    void setObserver(SDV_CONFIGUTATION_FILE_TYPE filetype,void * ptrObj, SDVConfigMonitor_t callback);

    /**
     * @fn acquireUrlTemplates
     *
     * @brief Attempts to acquire the configuration URL templates and store them to m_templates
     * @details Once templates have been acquired, they are not expected to change
     * @return true if URL templates successfully acquired; otherwise false
     */
    sdvTemplateAcquisitionResult_t acquireUrlTemplates();
    void registerSDVTuneReadyCallback(SDVReadyCallback_t callback);

private:

    /** @brief Download Queue Function template
     * Template of functions that get executed from within the Download Queue
     *
     * @param args optional arguments to provide to callback function when fired.
     * @return next dealy in millis to wait before re-firing event.
     */
    typedef uint32_t (*SDVDownloadThreadEventCallback)(SDV_CONFIGUTATION_FILE_TYPE arg);

    /**
     * @struct QueuedDownloadEvent
     * @brief Representation of an download thread eventevent which has been posted scheduled in the queue
     */
    struct QueuedDownloadEvent{
        rmf_osal_TimeMillis _timeToFire;            //!< absolute time event expires in milliseconds
        SDVDownloadThreadEventCallback _callback;   //!< callback function pointer
        SDV_CONFIGUTATION_FILE_TYPE _arg;           //!< optional argument to provide to callback

        /**
         * @constructor
         *
         * @param [in] timeout_ms dealy in ms before event should fire.  If event should fire immediately should be 0
         * @param [in] callback function pointer to callback that gets invoked when event is fired.
         * @param [in] args optional argument that is provided to callback function when fired.
         */
        QueuedDownloadEvent(rmf_osal_TimeMillis timeout_ms, SDVDownloadThreadEventCallback callback, SDV_CONFIGUTATION_FILE_TYPE  arg);

    };
    /**
     * @struct QueuedItemComparator
     * @brief  comparator for QueuedEvents on behalf of a priorty_queue
     *
     * Events will need to be sorted from low to high relative to how much
     * time is left before the event should fire.
     */
    struct QueuedItemComparator{
        bool operator()(const QueuedDownloadEvent  a, const QueuedDownloadEvent  b) const;
    };

    /**
     * @struct SDVFileObserver
     * @brief Representation of an observer for SDV file downloads.
     */
    struct SDVFileObserver{
        SDV_CONFIGUTATION_FILE_TYPE _filetype;  //!< The type of file observer is interested in
        void * _ptrObj;                         //!< pointer to instance of observer or NULL is static
        SDVConfigMonitor_t _callback;           //!< pointer to function that is invoked when file is downloaded.
        SDV_FILE_DATA * lastNotifiedData;       //!< reference to last downloaded version of file on the heap.

        /**
         * @constructor
         *
         * @param [in] The type of file observer is interested in
         * @param [in] ptrObj pointer to instance of observer or NULL is static
         * @param [in] callback pointer to function that is invoked when file is downloaded.
         */
        SDVFileObserver(SDV_CONFIGUTATION_FILE_TYPE filetype,void * ptrObj, SDVConfigMonitor_t callback);
    };

private:
    SDVConfigObtainer();

    /**
     * @fn getDeviceIds()
     *
     * @brief Get device specific locality information from POD
     *
     * Operation is atomic and output variable will not be set if any failure is encountered.
     *
     *@param [out] controllerID pointer that will have its value set to the controllerId
     *@param [out] channelMapId pointer that will have its value set to the channel map id
     *@param
     *@retrun RMF_SUCCESS if device IDs where obtained
     */
    rmf_Error getDeviceIds(uint16_t * controllerID, uint16_t * channelMapId, uint16_t * vctID);

    /**
     * @fn runPODCommand()
     *
     * @brief sends a command to the POD server.
     * @param [in] ipcBuf command to send to POD server
     *
     *@retrun RMF_SUCCESS if command was successfully executed
     */
    rmf_Error runPODCommand(IPCBuf & ipcBuf);

    /**
     * @fn downloadThread()
     *
     * @brief Main loop for execution of download related tasks.
     * Thread will continue running indefinitely waiting for the appropriate time
     * of the next task to fire.   for
     *
     * After a task is executed it will be rescheduled based on the delay returned
     * from the task's callback function.
     */
    static void downloadThread(void * obj);

    /**
     * @fn downloadDiscoveryFileAction()
     *
     * @brief Queued action for downloading the Discovry configuration file.
     *
     * @param [in] fileType which file to download
     *
     * @return delay in millis before the next attempt to download the
     *  configuration file
     */
    static uint32_t downloadFileAction(SDV_CONFIGUTATION_FILE_TYPE fileType);

    /**
     * @fn updateWildcardsEventAction()
     *
     * @brief Queued action for updating the know values for all wildcards
     *
     * @param [in] args not used
     *
     * @return delay in millis before the wildcards should be checked again.
     */
    static uint32_t updateWildcardsEventAction(SDV_CONFIGUTATION_FILE_TYPE arg);


    /**
     * @fn getRandomDelay()
     *
     * @brief Get a random backoff delay based on a min and max range
     * Takes into account the number of times a file download failure
     * has occurred so backoff times will degrade.
     *
     * @param [in] min the MIN back off time ( before degrading modification )
     * @param [in] max the MAX back off time ( before degrading modification )
     *
     * @return random backoff time in millis
     */
    uint32_t getRandomDelay(uint32_t min, uint32_t max);



    /**
     * @fn discoverPersistentPath()
     *
     * @brief Discovers the base path which should be used for persisting SDV configutation
     * files.
     */
    void discoverPersistentPath();

    /**
     * @fn writePersistentFile()
     *
     * @brief Attempts to write a given set of binary data to persistent storage.
     *
     * @param [in] filetype which configuation file the data is for
     * @param [in] data the binary data which should be written to persistent storage
     */
    void writePersistentFile(SDV_CONFIGUTATION_FILE_TYPE filetype, SDV_FILE_DATA * data);

    /**
     * @fn readPersistentFile()
     *
     * @brief Attempts to read a binary SDV configutation file from persistent storage.
     *
     * @param [in] filetype which configuation file the data is for
     * @param [in] data where the binary data of file where the file should be written to
     * in memory.
     *
     * @return RMF_SUCCESS if file was read successfully otherwise RMF_OSAL_NODATA
     */
    rmf_Error readPersistentFile(SDV_CONFIGUTATION_FILE_TYPE filetype, SDV_FILE_DATA * data);

    /**
     * @fn notifyObserver()
     *
     * @brief Notify an observer of a sdv configutation file that it has been downloaded.
     *
     * @param [in] filetype the type of file that was downloaded.
     * @param [in] the new contents of the file.
     * @param [in] true when file was retrieved from persistent storage 
     *
     * @return RMF_SUCCESS if no observer OR observer notified and indicated file is well formed
     * @return RMF_OSAL_ENODATA if file is indicated to be corrupted by observer
     */
    rmf_Error notifyObserver(SDV_CONFIGUTATION_FILE_TYPE filetype, SDV_FILE_DATA * data, bool isSavedCopy);

    /**
     * @fn writeFileToMemoryCallback()
     *
     * @brief Callback function for libCurl to write contents of file to memory instead of
     * to disk.
     *
     * @param [in] contents pointer to contents of the file
     * @param [in] size the current size of user data structure.  Will always be 1
     * @param [in] nmemb size of the downloaded data
     * @param [in] userp pointer to structure to copy file.
     */
    static size_t writeFileToMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);

    /**
     * @fn readFileHTTP()
     *
     * @brief Read a SDV configuration file into memory
     *
     * @param [in] url template of url where configutation file should be downloaded
     * @param [in] chunk where to copy the downloaded file to.
     *
     * @return result from curl library of file download
     */
    static CURLcode readFileHTTP(std::string & url, SDV_FILE_DATA * chunk );

    /**
     * @fn replace()
     *
     * @brief utility method to replace a wildcard in a template url with a value
     *
     * @param [in] str template of url containing wildcards
     * @param [in] token wildcard token to replace
     * @param [in] value value of wildcard
     */
    static void replace(std::string & str, const std::string & token, uint16_t value);

    /**
     * @fn replace()
     *
     * @brief utility method to replace all wildcard in a template with
     * values
     *
     * @param [in] str template containing wildcards
     */
    static void replaceWildcards(std::string & str);

    /**
     * @fn getNowTimeInMillis()
     *
     * @brief get the current UTC time in millis
     *
     * @return the current UTC time in milli seconds
     */
    static rmf_osal_TimeMillis getNowTimeInMillis();

    /**
     * @fn updateURLTemplatesFromAuthService()
     *
     * @brief updates the URL Templates for SDV Configutation files Templates using the auth service API
     *
     * @return TRUE if templates could been determined using the authservice API
     * otherwise FALSE
     */
    sdvTemplateAcquisitionResult_t updateURLTemplatesFromAuthService();

    /**
     * @fn updateURLTemplatesFromAuthService()
     *
     * @briefupdates the URL Templates for SDV Configutation files using the
     *  SDV_TEMPLATE_FILE_PATH override file
     *
     * @return TRUE if templates SDV_TEMPLATE_FILE_PATH file was present
     */
    sdvTemplateAcquisitionResult_t updateURLTemplatesFromFileSystem();

    CURLcode postToHTTP(SDV_FILE_DATA * chunk, const char * url, const char * field);

    void checkAndNotifyTuneReadiness(SDV_CONFIGUTATION_FILE_TYPE downloadedFiletype);

private:
    bool m_running;                 //!<flag indicating download thread should continue running
    uint16_t m_controllerId;        //!< member reference to last known controller Identifier
    uint16_t m_channelMapId;        //!< member reference to last known channel map Identifier
    uint16_t m_vctId;               //!< member reference to last known vctid which is expected to equal hubid
    uint32_t downloadFailureCount;  //!< Counter of the number of times a download operation has failed.

    rmf_osal_ThreadId               m_threadId;                    //!< ID of the Download thread.
    rmf_osal_Cond                   m_thread_wait_conditional;     //!< Conditional used to wait on refresh/retry attempts in download thread
    rmf_osal_Mutex                  m_observerMutex;               //!< Mutex to protect access to observers list and state

    typedef std::priority_queue<QueuedDownloadEvent, std::vector<QueuedDownloadEvent>, QueuedItemComparator > SVD_FILE_EVENT_QUEUE_TYPE;
    SVD_FILE_EVENT_QUEUE_TYPE _queuedEvents;    //!< Prioritized list of pending action in the download queue
    SDVFileObserver * m_observers[3];           //!< Index of registered observers keyed on file type
    std::string m_templates[3];                 //!< Index of of URL templates keyed of file type
    std::string m_persistentTemplate[3];        //!< Index of of Filename templates keyed of file type

    std::string m_persistentPath;               //!< Path to folder where files will be persisted
    SDVReadyCallback_t m_tuneReadyCallback;
    bool m_IPNetworkIsUp;
    uint8_t m_availableConfigFiles;
    bool m_tuneReadyNotified;
};

#endif /* SDVCONFIGOBTAINER_H_ */
