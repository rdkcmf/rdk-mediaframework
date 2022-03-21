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
 * @file rmfvodsource.cpp
 * @brief RMF VOD source which tunes and starts the playback Video On Demand.
 */
#include <rmfvodsource.h>
#include "rmfqamsrcpriv.h"
#include "T2PClient.h"
#include "T2PServer.h"
#include <jansson.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "Autodiscovery.h"

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

#define VOD_CLIENT_IP "127.0.0.1"
#define VOD_CLIENT_PORT 11000
#define VOD_SERVER_ID "VOD_SERVER_ID"
#define RMF_VODSRC_ERRORSTR_LENGTH  256

static int64_t gVodServerId = -1;
static rmf_Error getVodServerId( uint32_t *pVodServerID );
static unsigned int g_pmt_optimization_level = RMFQAMSrcImpl::PMT_HANDLING_UNOPTIMIZED;

/**
 * @class RMFVODSrcImpl
 * @brief This class is used for processing the VOD request and playback the VOD using trick mode.
 * This inherit the properties from RMFQAMSrcImpl to make use of QAM Source functionalities
 * to tune the frequency and playback the SPTS. This class is responsible for managing several
 * functionalities such as open the VOD url, parse the uri, tune to the requested frequency for
 * receiving the VOD data as SPTS stream, playback the VOD from given play position, pause the playback,
 * set the speed of playback, handling the events such as end of VOD stream reached, error events, and so on.
 * @ingroup vodsourceclass
 */
class RMFVODSrcImpl:public RMFQAMSrcImpl
{
	public:
		GstElement * src_element;
		void *getLowLevelElement();
		RMFVODSrcImpl( RMFMediaSourceBase * parent );
		~RMFVODSrcImpl();
		RMFResult getSourceInfo( const char *uri,
				rmf_ProgramInfo_t * pProgramInfo,
				uint32_t * pDecimalSrcId );		
		RMFResult processvodrequest( const char *vodurl,
				char *ocaplocator, int& responseCode, uint32_t size_ocaplocator, uint32_t& tsid );

		RMFResult open( const char *uri, char *mimetype );
		RMFResult close();
		RMFResult getSpeed( float &speed );
		RMFResult setSpeed( float speed );
		RMFResult getMediaTime( double &time );
		RMFResult setMediaTime( double time );
		RMFResult getMediaTimeAndSpeed( double &time, float &speed, bool &adStatus );
		RMFResult addEventHandler( IRMFMediaEvents* events );
		RMFResult play();
		RMFResult pause();
		RMFResult stop();

		void onEOSVOD();
		void requestINBProgramInfo();
		void notifyError(RMFResult err, const char* msg);
		void notifyStatusChange(unsigned long status);
		void setWaitForClose();

	private:
		AutoDiscovery* m_adInstance;
		float 	m_speed;
		float   m_prevSpeed;
		double 	m_position;
		char 	m_session_id[256];
		char 	m_voduri[1024];
		char 	m_mimetype[256];
		bool 	m_playingState;
		int 	m_tuneDelay;
		int	m_serverStatus;
		int	m_lastError;
		bool m_isBaseQamSourceOpen;
		int m_lastErrorForTearDownResponse;
		bool	m_streamingState;
		bool m_isEosIssued;
		bool    m_doWaitForCloseResponse;
		virtual unsigned int getPMTOptimizationPolicy() const;
};

/**
 * @addtogroup vodsourceapi
 * @{
 * @fn void RMFVODSrc::EOSEventCallback(void* objRef)
 * @brief A callback routine for handling the end of stream in VOD Source.
 * This functions is used by AutoDiscovery::notifyEndOfStream().
 *
 * @param[in] objRef object of class RMFVODSrcImpl
 *
 * @return None
 * @}
 */
void RMFVODSrc::EOSEventCallback(void* objRef)
{
	class RMFVODSrcImpl* vodSrcImpl = (RMFVODSrcImpl*)objRef;
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", " %s():%d\n",		
		__FUNCTION__, __LINE__ );
	vodSrcImpl->onEOSVOD();
}

/**
 * @fn RMFResult RMFVODSrc::init_platform(  )
 * @brief This function is used to initialize the platform specific functionalities for VOD Source.
 *
 * This is currently not implemented.
 *
 * @return None
 */
RMFResult RMFVODSrc::init_platform(  )
{
	const char *pmtOptimization = rmf_osal_envGet("VODSRC.PMT.OPTIMIZATION.LEVEL");
	if (NULL != pmtOptimization)
	{
		g_pmt_optimization_level = (unsigned int) strtol(pmtOptimization, NULL, 10);
		if(RMFQAMSrcImpl::PMT_HANDLING_MAX <=  g_pmt_optimization_level)
		{
			g_pmt_optimization_level = RMFQAMSrcImpl::PMT_HANDLING_UNOPTIMIZED;
		}
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "%s - PMT optimization level is %d.", __FUNCTION__, g_pmt_optimization_level);
	}
	return RMF_RESULT_SUCCESS;
}

/**
 * @fn RMFResult RMFVODSrc::uninit_platform(    )
 * @brief This function is used to un-initialize the platform specific functionalities for VOD Source.
 *
 * This is currently not implemented.
 *
 * @retval RMF_RESULT_SUCCESS Returns success always.
 */
RMFResult RMFVODSrc::uninit_platform(	)
{
	return RMF_RESULT_SUCCESS;
}

/**
 * @addtogroup vodsourceapi
 * @{
 * @fn RMFVODSrc::RMFVODSrc(  )
 * @brief It is a default constructor used to log a message in "LOG.RDK.VODSRC".
 *
 * @return None
 */
RMFVODSrc::RMFVODSrc(  )
{
	RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.VODSRC", " %s():%d\n",
		__FUNCTION__, __LINE__ );
}

/**
 * @fn RMFVODSrc::~RMFVODSrc(  )
 * @brief It is a default destructor used to log a message in ""LOG.RDK.VODSRC""
 *
 * @return None
 */
RMFVODSrc::~RMFVODSrc(  )
{
	RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.VODSRC", " %s():%d\n",
		__FUNCTION__, __LINE__ );
}


RMFResult RMFVODSrc::init()
{
	int retVal = RMF_VODSRC_ERROR_SRM_ITV_AD_FAILURE;
	bool adCompleted = false;
	uint32_t wait = 0;

	if ( false == AutoDiscovery::getInstance()->getAutoDiscoveryStatus() )
	{
		if (AutoDiscovery::getInstance()->triggerAutoDiscovery() == RMF_RESULT_FAILURE)
		{
			return retVal;
		}
	}

	do
	{
		adCompleted =  AutoDiscovery::getInstance()->getAutoDiscoveryStatus();
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "%s: Auto Discovery is %s\n",
			__FUNCTION__, (adCompleted) ? "Completed":"not yet completed");

		if ( adCompleted || wait > 13 )
		{
			break;
		}
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "AutoDiscovery is inprogress\n" );
		sleep(2);
		wait++;
	} while ( !adCompleted );

	if ( !adCompleted )
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "Auto Discovery is not complete!\n" );
		return retVal;
	}

	return RMFMediaSourceBase::init();
}

void RMFVODSrc::setWaitForClose()
{
	RMFMediaSourcePrivate *vodImpl = getPrivateSourceImpl();
	((RMFVODSrcImpl*)vodImpl)->setWaitForClose();
}

/**
 * @fn RMFMediaSourcePrivate *RMFVODSrc::createPrivateSourceImpl(  )
 * @brief This function is used to create an instance of VOD Source implementation object for handling tuning and playback.
 *
 * @return Returns an object of class RMFVODSrcImpl which is dynamically create in it.
 */
RMFMediaSourcePrivate *RMFVODSrc::createPrivateSourceImpl(  )
{
	RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.VODSRC", " %s():%d\n",
		__FUNCTION__, __LINE__ );
	return new RMFVODSrcImpl( this );
}

/**
 * @fn RMFVODSrcImpl::RMFVODSrcImpl( RMFMediaSourceBase * parent ):RMFQAMSrcImpl( parent )
 * @brief It is a constructor for initializing the member variables for VOD Source implementation.
 * This will reset the member variables such as speed, position, voduri, session id, playing state and server status.
 * This function is used to trigger the auto discovery, sets the tune delay parameter.
 *
 * @param[in] parent an instance of RMFMediaSourceBase class
 *
 * @return None
 */
RMFVODSrcImpl::RMFVODSrcImpl( RMFMediaSourceBase * parent ):RMFQAMSrcImpl
    ( parent )
{
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", " %s():%d\n",
		__FUNCTION__, __LINE__ );

	m_speed = 1.0;
	m_prevSpeed = 1.0;
	m_position = 0.0;
	memset ( m_voduri, 0x00, sizeof(m_voduri) );
	memset ( m_session_id, 0x00, sizeof(m_session_id) );
	strncpy ( m_session_id, "deadbeef", sizeof (m_session_id) - 1 );
	m_playingState = false;
	m_serverStatus = 0;
	m_lastError = 0;
	m_isBaseQamSourceOpen = false;
	m_lastErrorForTearDownResponse = 0;
	m_streamingState = false;

	m_isEosIssued = false;
	m_doWaitForCloseResponse = false;
	m_adInstance = AutoDiscovery::getInstance();

	const char* tuneDelayFlag = rmf_osal_envGet("FEATURE.VODSOURCE.TUNE_DELAY");
	if ( NULL != tuneDelayFlag )
	{
		m_tuneDelay = 1000 * atoi( tuneDelayFlag );
	}
	else
	{
		m_tuneDelay = 0;
	}
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "%s:%d Tune delay: %d\n",
		__FUNCTION__, __LINE__, m_tuneDelay );
}

/**
 * @fn RMFVODSrcImpl::~RMFVODSrcImpl(  )
 * @brief A destructor to free the resources allocated for low level element implementation for RMFQAMSrc module.
 *
 * @return None
 */
RMFVODSrcImpl::~RMFVODSrcImpl(  )
{
	RMFQAMSrc::freeLowLevelElement( ( void * ) src_element );
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", " %s():%d \n",
	__FUNCTION__, __LINE__ );
}

/**
 * @fn void *RMFVODSrcImpl::getLowLevelElement(  )
 * @brief This function will get the low level elements from RMF QAM Source.
 *
 * @return None
 */
void *RMFVODSrcImpl::getLowLevelElement(  )
{
    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", " %s():%d \n", __FUNCTION__,
             __LINE__ );
    src_element = ( GstElement * ) RMFQAMSrc::getLowLevelElement(  );
    return src_element;
}

/**
 * @fn RMFVODSrcImpl::getSourceInfo( const char* ocaplocator, rmf_ProgramInfo_t* pInfo, uint32_t* pDecimalSrcId )
 * @brief Parse the ocap locator to get the frequency, program number and modulation mode.
 *
 * @param[in] ocaplocator ocap locator, example ocap://f=0xfreq.0xPrgNum.m=0xModulation
 * for which frequency, program number and modulation mode to be extracted from it.
 * @param[out] pInfo program info structure where frequency, modulation and program number to be stored.
 * @param[out] pDecimalSrcId VOD Source Id to be set as 0.
 *
 * @return RMFResult (int)
 * @retval RMF_RESULT_SUCCESS If it is successfully update the parameter in pInfo structure
 * @retval RMF_RESULT_FAILURE returns an error when null found in input parameter ocaplocator.
 */
RMFResult RMFVODSrcImpl::getSourceInfo( const char *ocaplocator,
                                        rmf_ProgramInfo_t * pInfo,
                                        uint32_t * pDecimalSrcId )
{
    if ( !ocaplocator )
    {
        return RMF_RESULT_FAILURE;
    }
    sscanf( ocaplocator, "ocap://f=0x%x.0x%x.m=0x%x",
            &pInfo->carrier_frequency, &pInfo->prog_num,
            &pInfo->modulation_mode );

    if ( NULL != pDecimalSrcId )
    {
        *pDecimalSrcId = 0;
    }

    return RMF_RESULT_SUCCESS;
}

/**
 * @fn RMFVODSrcImpl::processvodrequest( const char *vodurl,char *ocaplocator, int& responseCode, uint32_t size_ocaplocator, uint32_t& tsid)
 * @brief This function will process a VOD request for a supplied VOD url. It will compose a play request
 * message and send it to VOD client. After validating success response from the VOD Client,
 * it will parse the session Id and ocap locator from the response message. A typical ocap locator would look
 * like following ocap://f=0xfreq.0xPrgNum.m=0xModulation.
 *
 * @param[in] vodurl VOD request url
 * @param[out] ocaplocator a locator for which result will be stored
 * @param[out] responseCode response code such as RF not available, Tune frequency mismatch, TSID mismatch,
 * seek to a new position error, etc.
 * @param[in] size_ocaplocator size of ocap locator
 * @param[out] tsid Transport stream ID
 *
 * @return RMFResult (int)
 * @retval RMF_RESULT_SUCCESS If it successfully get the ocaplocator and responseCode from VOD Client.
 * @retval RMF_RESULT_FAILURE Invalid ocap locator, Unable to get response from VOD Client or waiting
 * for the AutoDiscovery to complete.
 */
RMFResult RMFVODSrcImpl::processvodrequest( const char *vodurl,
                                            char *ocaplocator, int& responseCode, uint32_t size_ocaplocator, uint32_t& tsid)
{
    const int msgBufSz = 1024;
    char t2p_resp_str[msgBufSz];
    char t2p_req_str[msgBufSz];
    int retVal = RMF_RESULT_FAILURE;
    uint32_t wait = 0;
    int position = ( int ) m_position;
    responseCode = RMF_VODSRC_ERROR_SRM_ERROR_016;

    if ( !vodurl || !ocaplocator )
    {
        return retVal;
    }

/*Request:       
{
	"vodPlayRequest" : {
		"vodUrl" : vodUrl,
		"position" : position,
		"rate" : rate
	}
}
*/
    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "SessionId = %s\n", m_session_id );

    // Invoke vod client  to get ocap locator for vod request
    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC",
             "processvodrequest speed is %f position is %lf\n", m_speed,
             m_position );
    snprintf( t2p_req_str, sizeof (t2p_req_str),
             "{ \"vodPlayRequest\" : { \"vodUrl\" : \"%s\", \"position\": %d, \"rate\": %f ,\"sessionId\": %s}}\n",
             vodurl, position, m_speed,m_session_id );

    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC",
             "Received service selection message for service %s\n", vodurl );
    RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.VODSRC",
             "processvodrequest()::%s length %d\n", t2p_req_str,
             strlen( t2p_req_str ) );
    T2PClient *t2pclient = new T2PClient(  );
    if ( t2pclient->openConn( VOD_CLIENT_IP, VOD_CLIENT_PORT ) == 0 )
    {
        int recvdMsgSz = 0;
        t2p_resp_str[0] = '\0';
        if ( ( recvdMsgSz =
               t2pclient->sendMsgSync( t2p_req_str, strlen( t2p_req_str ),
                                       t2p_resp_str, msgBufSz ) ) > 0 )
        {
            json_t *msg_root = NULL;
            json_error_t json_error;
            json_t *Response;
            json_t *status;
            json_t *locator;
            json_t *sessionId;
            json_t *tsidJson;
            t2p_resp_str[recvdMsgSz] = '\0';
            RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.VODSRC",
                     "processvodrequest(): Response : %s\n", t2p_resp_str );

            /*
             * Response:
             * {
             * "vodPlayResponse" : {
             * <ResponseStatus>,
             * "locator": Locator
             * "sessionId": session Id
             * }
             * }
             * 
             */

            msg_root = json_loads( t2p_resp_str, 0, &json_error );
            if ( !msg_root )
            {
                RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC",
                         "json_error: on line %d: %s\n", json_error.line,
                         json_error.text );
            }
            else
            {
                Response = json_object_get( msg_root, "vodPlayResponse" );
                if ( !Response )
                {
                    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC",
                             "json_error: Response object not found\n" );
                }
                else
                {
                    status = json_object_get( Response, "ResponseStatus" );
                    if ( !status )
                    {
                        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC",
                                 "json_error: status object not found\n" );
                    }
                    else
                    {
                        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC",
                                 "json_log: Status = %lld\n",
                                 json_integer_value( status ) );
                        if ( json_integer_value( status ) == 0 )
                        {
                            responseCode = 0;
                            sessionId =
                                json_object_get( Response, "sessionId" );
                            RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC",
                                     "json_log: SessionId = %s\n",
                                     json_string_value( sessionId ) );
                            strcpy( m_session_id,
                                    json_string_value( sessionId ) );
                            RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC",
                                     "json_log: SessionId = %s\n",
                                     m_session_id );
                            locator = json_object_get( Response, "locator" );
                            if ( !locator )
                            {
                                RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC",
                                         "json_error: statusMessage object not found\n" );
                            }
                            else
                            {
                                memset(ocaplocator, 0, size_ocaplocator);
                                strncpy( ocaplocator,
                                        json_string_value( locator ), size_ocaplocator - 1);
                                //ocap://f=0xfreq.0xPrgNum.m=0xModulation
                                RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC",
                                         "json_log:%s\n", ocaplocator );
                                tsidJson= json_object_get( Response, "tsid" );
                                tsid = json_integer_value( tsidJson);
                                retVal = RMF_RESULT_SUCCESS;
                            }
                        }
                        else
                        {
                            responseCode = json_integer_value( status );
                        }
                    }
                }
                // Release resources
                json_decref( msg_root );
            }
        }
        t2pclient->closeConn();
        delete t2pclient;
    }

    return retVal;
}

/**
 * @fn RMFResult RMFVODSrcImpl::open( const char *voduri, char *mimetype )
 * @brief This function will parse the voduri which is received from client machine,
 * process the VOD request and get the OCAP locator from VOD client.
 * The API will use the QAM Source and POD Manager properties for receiving the VOD data.
 *
 * It will try to extract the information from voduri such as session id,
 * play position, play rate of the video such as 4x, 8x, etc. All the necessary information put into
 * JSON format and send it to VOD client to get the OCAP locator from where VOD should
 * be tuned. It will call the function  processvodrequest() for parsing the client
 * request and communicate with VOD Client with the help of T2P Client module.
 * Check the server is up and running then API will use the QAM source RMFQAMSrcImpl::open() for PSI acquisition and
 * used to filter the PAT and PMT for which the VOD data to be filtered.
 *
 * @param[in] voduri requested uri from client box will be contained session Id, play position,
 * bitrate for which video need to be stream, etc.
 * @param[in] mimetype which is not currently being used/
 *
 * @return On success the function will return RMF_RESULT_SUCCESS, any other value will indicate that as an error.
 */
RMFResult RMFVODSrcImpl::open( const char *voduri, char *mimetype )
{
	( void ) mimetype;
	rmf_Error ret = RMF_RESULT_SUCCESS;
	char uri[1024];
	int responseCode;
	int patTimeoutOffset = 4000;
	uint32_t tsid;
	m_isEosIssued = false;
	AutoDiscovery::getInstance()->clearAdInsertionQueue();
	if ( processvodrequest( voduri, uri, responseCode, sizeof (uri), tsid ) != RMF_RESULT_SUCCESS )
	{
		m_serverStatus = responseCode;
		notifyError( (RMF_VODSRC_ERROR_BASE + responseCode), NULL );
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.VODSRC", "%s: Server error: SRM-%X\n",
				__FUNCTION__, responseCode);
	}
	m_serverStatus = responseCode;

	/* This is just work arround to know in Cisco platform, Experimenting
	 * are we able to play VOD after some delay to avoid the CA error
	 * Delay can be set in milliseconds
	 */
	usleep( m_tuneDelay );
	memset(m_voduri, 0, sizeof (m_voduri));
	strncpy( m_voduri, voduri, sizeof (m_voduri) - 1);

	const char* patTimeoutOffsetFlag = rmf_osal_envGet("FEATURE.VODSOURCE.PATTIMEOUTOFFSET");
	if ( NULL != patTimeoutOffsetFlag )
	{
		patTimeoutOffset = atoi( patTimeoutOffsetFlag );
	}	
	RMFQAMSrcImpl::setPatTimeoutOffset( patTimeoutOffset );
	RMFQAMSrcImpl::setTsIdForValidation(tsid);
	if ( m_serverStatus == 0x00 )
	{
		m_adInstance->registerVodSource( m_session_id, this );
		ret = RMFQAMSrcImpl::open( (const char*)uri, mimetype );
		if (RMF_RESULT_SUCCESS== ret )
		{
			m_isBaseQamSourceOpen = true;
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "QAMSrc::%s = %d\n", __FUNCTION__, ret );
		}
	}
	m_playingState = true;

	m_isEosIssued = false;
	return ret;
}

/**
 * @fn RMFResult RMFVODSrcImpl::close(  )
 * @brief This function will close the VOD by sending stop request to JAVA VOD Client. It will
 * verify the session id for which the connection is established and sends a "vodStopRequest" to
 * VOD Client for terminating the existing connection. It also calls RMFQAMSrcImpl::close() to
 * stop the qam source module.
 *
 * @return on success the function will return RMF_RESULT_SUCCESS, any other value will indicate that as error.
 */
RMFResult RMFVODSrcImpl::close(  )
{
	rmf_Error ret = RMF_RESULT_SUCCESS;
	const int msgBufSz = 1024;
	char t2p_resp_str[msgBufSz];
	char t2p_req_str[msgBufSz];

	if ( 0 == strcmp (m_session_id, "deadbeef") )
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "No need to stop session\n" );
		return ret;
	}

	if (m_lastErrorForTearDownResponse == 0)
	{
		snprintf( t2p_req_str, sizeof (t2p_req_str), "{ \"vodStopRequest\" : {\"sessionId\": %s}}\n",
				m_session_id );
	}
	else
	{
		RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC", "m_lastErrorForTearDownResponse= %s:%d\n" , __FUNCTION__, m_lastErrorForTearDownResponse);
		snprintf( t2p_req_str, sizeof(t2p_req_str), "{ \"vodStopRequest\" : {\"sessionId\": %s, \"streamerror\":%d}}\n",
				m_session_id, m_lastErrorForTearDownResponse );
	}

	if ( m_lastError != RMF_VODSRC_ERROR_SRM_ITV_SETTOP )
	{
		T2PClient *t2pclient = new T2PClient();
		if ( t2pclient->openConn( VOD_CLIENT_IP, VOD_CLIENT_PORT ) == 0 )
		{
			int recvdMsgSz = 0;
			t2p_resp_str[0] = '\0';
			int closeWait = (m_doWaitForCloseResponse == true) ? 2 : 1;
			if ( ( recvdMsgSz = t2pclient->sendMsgSync( t2p_req_str,
							strlen(t2p_req_str),
							t2p_resp_str, msgBufSz, closeWait ) ) > 0 )
			{
				t2p_resp_str[recvdMsgSz] = '\0';
				RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.VODSRC",
						"term(): Response : %s\n", t2p_resp_str );
			}
		}
		t2pclient->closeConn();
		delete t2pclient;
	}

	if ( true == m_isBaseQamSourceOpen)
	{
		ret = RMFQAMSrcImpl::close();
		m_isBaseQamSourceOpen = false;
	}
	m_adInstance->unregisterVodSource( m_session_id );

	return ret;
}

/**
 * @fn RMFResult RMFVODSrcImpl::getSpeed( float &speed )
 * @brief This function is used to get the current Playback speed set for VOD playback.
 *
 * @param[in] speed playback speed [1.0=normal playback, >1.0=FastForword, <1.0=Rewind]
 *
 * @retval RMF_RESULT_SUCCESS Returns success always.
 */
RMFResult RMFVODSrcImpl::getSpeed( float &speed )
{
	speed = m_speed;
	return RMF_RESULT_SUCCESS;
}

/**
 * @fn RMFResult RMFVODSrcImpl::setSpeed( float speed )
 * @brief This function is used to set the playback speed for VOD Source.
 *
 * @param[in] speed playback speed [1.0=normal playback, >1.0=FastForword, <1.0=Rewind]
 *
 * @return RMF_RESULT_SUCCESS Returns success always.
 */
RMFResult RMFVODSrcImpl::setSpeed( float speed )
{
	m_prevSpeed = m_speed;
	m_speed = speed;
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC",
		"Received call for setRate rate=%f\n", speed );
	return RMF_RESULT_SUCCESS;
}

/**
 * @fn RMFResult RMFVODSrcImpl::play()
 * @brief This function is used to put the player into play mode. If the player is already in paused state the function 
 * will bring into the play mode.
 *
 * @return Returns RMF_RESULT_SUCCESS on success, any other value will indicate as failure.
 */
RMFResult RMFVODSrcImpl::play()
{
	RMFResult ret = RMF_RESULT_SUCCESS;

	if ( m_serverStatus == 0x00 )
	{
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.VODSRC", "%s: QAMSrc::play\n", __FUNCTION__ );
		ret = RMFQAMSrcImpl::play();
	}

	return ret;
}

/**
 * @fn RMFResult RMFVODSrcImpl::pause()
 * @brief This function is used to put the player in pause state.
 *
 * @return Returns RMF_RESULT_SUCCESS on success, any other value indicate as failure
 */
RMFResult RMFVODSrcImpl::pause()
{
	RMFResult ret = RMF_RESULT_SUCCESS;

	if ( m_serverStatus == 0x00 )
	{
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.VODSRC", "%s: QAMSrc::pause\n", __FUNCTION__ );
		m_isEosIssued = false;
		ret = RMFQAMSrcImpl::pause();
	}

	return ret;
}

/**
 * @fn RMFResult RMFVODSrcImpl::stop()
 * @brief This function is used to stop the VOD playback.
 *
 * @return Returns RMF_RESULT_SUCCESS on success, otherwise RMF error code.
 */
RMFResult RMFVODSrcImpl::stop()
{
	RMFResult ret = RMF_RESULT_SUCCESS;

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.VODSRC", "VODSrcImpl::%s\n", __FUNCTION__ );
	if ( m_serverStatus == 0x00 )
	{
		ret = RMFQAMSrcImpl::stop();
	}

	return ret;
}

/**
 * @fn RMFResult RMFVODSrcImpl::getMediaTime( double &time )
 * @brief This function is used to get the current playback time for which the VOD is playing. It will
 * use T2PClient to make the communication with VOD Client and receive the current play position.
 *
 * @param[out] time current playtime in terms of seconds.
 *
 * @return RMFResult (int )
 * @retval RMF_RESULT_SUCCESS got the current playback time of VOD
 * @retval RMF_RESULT_FAILURE returns an error if resource is not available
 */
RMFResult RMFVODSrcImpl::getMediaTime( double &time )
{
    const int msgBufSz = 1024;
    char t2p_resp_str[msgBufSz];
    char t2p_req_str[msgBufSz];

    RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.VODSRC", "Enter %s():%d \n",
             __FUNCTION__, __LINE__ );
    snprintf( t2p_req_str, sizeof (t2p_req_str),
             "{ \"vodGetPositionRequest\" : {\"sessionId\": %s}}\n",
             m_session_id );

    if ( m_adInstance->getAutoDiscoveryStatus() == false )
    {
        RDK_LOG( RDK_LOG_WARN, "LOG.RDK.VODSRC", "Autodiscovery not completed"
            " %s():%d \n", __FUNCTION__, __LINE__ );
        time = 0;
        return RMF_RESULT_FAILURE;
    }

    T2PClient *t2pclient = new T2PClient(  );
    if ( t2pclient->openConn( VOD_CLIENT_IP, VOD_CLIENT_PORT ) == 0 )
    {
        int recvdMsgSz = 0;
        t2p_resp_str[0] = '\0';
        if ( ( recvdMsgSz =
               t2pclient->sendMsgSync( t2p_req_str, strlen( t2p_req_str ),
                                       t2p_resp_str, msgBufSz ) ) > 0 )
        {
            json_t *msg_root = NULL;
            json_error_t json_error;
            json_t *Response;
            json_t *status;
            json_t *position;

            t2p_resp_str[recvdMsgSz] = '\0';
            RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.VODSRC",
                     "RMFVODSrcImpl::getMediaTime(): Response : %s\n",
                     t2p_resp_str );
            t2p_resp_str[recvdMsgSz] = '\0';
            RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.VODSRC",
                     "RMFVODSrcImpl::getMediaTime(): Response : %s\n",
                     t2p_resp_str );

            /*
             * Response:
             * {
             * "vodGetPositionResponse" : {
             * <ResponseStatus>,
             * "position": position
             * "sessionId": session Id
             * }
             * }
             * 
             */

            msg_root = json_loads( t2p_resp_str, 0, &json_error );
            if ( !msg_root )
            {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.VODSRC",
                         "json_error: on line %d: %s\n", json_error.line,
                         json_error.text );
            }
            else
            {
                Response =
                    json_object_get( msg_root, "vodGetPositionResponse" );
                if ( !Response )
                {
                    RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.VODSRC",
                             "json_error: Response object not found\n" );
                }
                else
                {
                    status = json_object_get( Response, "ResponseStatus" );
                    if ( !status )
                    {
                        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.VODSRC",
                                 "json_error: status object not found\n" );
                    }
                    else
                    {
                        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC",
                                 "json_log: Status = %lld\n",
                                 json_integer_value( status ) );
                        if ( json_integer_value( status ) == 0 )
                        {
                            position =
                                json_object_get( Response, "position" );
                            RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC",
                                     "json_log: position = %lld\n",
                                     json_integer_value( position ) );
                           // VOD Client returns position in int.
                            long long position_val = json_integer_value( position );
                            m_position = ( double ) position_val;
                        }
                    }
                }
                // Release resources
                json_decref( msg_root );
            }
        }
    }
    t2pclient->closeConn(  );
    delete t2pclient;

    time = m_position;
    return RMF_RESULT_SUCCESS;
}

/**
 * @fn RMFResult RMFVODSrcImpl::getMediaTimeAndSpeed( double &time, float &speed, bool &adStatus )
 * @brief This function is used to get the current playback time and speed for which the VOD is playing. It will
 * use T2PClient to make the communication with VOD Client and receive the current play position.
 *
 * @param[out] time current playtime in terms of seconds.
 * @param[out] speed current play speed (1.0=normal playback, <1.0=rewind, >1.0=FF).
 * @param[out] adStatus Autodiscovery status to be stored.
 *
 * @return RMFResult (int )
 * @retval RMF_RESULT_SUCCESS got the current playback time of VOD
 * @retval RMF_RESULT_FAILURE returns an error if resource is not available
 */
RMFResult RMFVODSrcImpl::getMediaTimeAndSpeed( double &time, float &speed, bool &adStatus )
{
	const int msgBufSz = 1024;
	char t2p_resp_str[msgBufSz];
	char t2p_req_str[msgBufSz];
	long long position_val, rate_val;

	snprintf( t2p_req_str, sizeof (t2p_req_str),
		"{ \"vodGetPositionAndRateRequest\" : {\"sessionId\": %s}}\n",
		m_session_id );

	if ( m_adInstance->getAutoDiscoveryStatus() == false )
	{
		RDK_LOG( RDK_LOG_WARN, "LOG.RDK.VODSRC", "Autodiscovery not completed"
				" %s():%d \n", __FUNCTION__, __LINE__ );
		time = 0;
		return RMF_RESULT_FAILURE;
	}

	T2PClient *t2pclient = new T2PClient();
	if ( t2pclient->openConn(VOD_CLIENT_IP, VOD_CLIENT_PORT) == 0 )
	{
		int recvdMsgSz = 0;
		t2p_resp_str[0] = '\0';
		if ( (recvdMsgSz = t2pclient->sendMsgSync(t2p_req_str, strlen(t2p_req_str),
			t2p_resp_str, msgBufSz)) > 0 )
		{
			json_t *msg_root = NULL;
			json_error_t json_error;
			json_t *Response;
			json_t *status;
			json_t *position;
			json_t *rate;
			json_t *ad;

			t2p_resp_str[recvdMsgSz] = '\0';
			RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.VODSRC", "%s : Response : %s\n",
				__FUNCTION__, t2p_resp_str );
			/*
			 * Response:
			 * {
			 * "vodGetPositionResponse" : {
			 * <ResponseStatus>,
			 * "position": position
			 * "rate" : rate
			 * "adStatus" : <started | completed>
			 * "sessionId": session Id
			 * }
			 * }
			 */

			msg_root = json_loads( t2p_resp_str, 0, &json_error );
			if ( !msg_root )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.VODSRC",
					"json_error: on line %d: %s\n", json_error.line,
					json_error.text );
			}
			else
			{
				Response =
					json_object_get( msg_root, "vodGetPositionAndRateResponse" );
				if ( !Response )
				{
					RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.VODSRC",
						"json_error: Response object not found\n" );
				}
				else
				{
					status = json_object_get( Response, "ResponseStatus" );
					if ( !status )
					{
						RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.VODSRC",
							"json_error: status object not found\n" );
					}
					else
					{
						RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC",
							"json_log: Status = %lld\n",
							json_integer_value(status) );
						if ( json_integer_value(status) == 0 )
						{
							position =
								json_object_get( Response, "position" );
							RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC",
								"json_log: position = %lld\n",
								json_integer_value(position) );
							position_val = json_integer_value( position );
							m_position = (double)position_val;

							rate = json_object_get( Response, "rate" );
							RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC",
								"json_log: rate = %d\n", json_integer_value(rate) );
							rate_val = json_integer_value( rate );
							speed = (float)rate_val;

							ad = json_object_get( Response, "adStatus" );
							RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC",
								"json_log: rate = %d\n", json_integer_value(ad) );
							adStatus = json_integer_value( ad );
						}
					}
				}
				json_decref( msg_root );
			}
		}
	}
	t2pclient->closeConn();
	delete t2pclient;

	time = m_position;

	return RMF_RESULT_SUCCESS;
}

/**
 * @fn RMFResult RMFVODSrcImpl::setMediaTime( double time )
 * @brief This function is used to do skip operation during playback. Time offset 0 means playback should
 * start from beginning of the file.
 *
 * @param[in] time skip time in second (from where to start playback time in seconds).
 *
 * @return RMFResult (int )
 * @retval RMF_RESULT_SUCCESS It was able to set the media time
 * @retval RMF_RESULT_NOT_INITIALIZED VOD is not yet initialized by VOD Client
 */
RMFResult RMFVODSrcImpl::setMediaTime( double time )
{
	rmf_Error ret = RMF_RESULT_SUCCESS;
	m_position = time;
	char uri[1024];
	int responseCode;

	if ( true == m_playingState )
	{
		AutoDiscovery::getInstance()->clearAdInsertionQueue();
		resetSptsTimer();
		uint32_t tsid;
		if ( processvodrequest( m_voduri, uri, responseCode, sizeof (uri), tsid ) != RMF_RESULT_SUCCESS )
		{
			if (responseCode == -2)
			{
				ret = RMF_RESULT_NO_PERMISSION;
				if (m_prevSpeed == 0.0)
				{
					m_speed = m_prevSpeed;
					RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.VODSRC",
						"FF not allowed moved back to paused\n");
				}
				AutoDiscovery::getInstance()->notifyAdInsertion( RMF_VOD_AD_INSERTED );
			}
			else
			{
				ret = responseCode;
				m_serverStatus = responseCode;
				notifyError( (RMF_VODSRC_ERROR_BASE + responseCode), NULL );
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.VODSRC", "%s: Server error: SRM-%X\n",
						__FUNCTION__, responseCode);
			}
		}
	}

	return ret;
}

/**
 * @fn void RMFVODSrcImpl::requestINBProgramInfo()
 * @brief This function is for getting the inband program specific information.
 * The implementation of pInbandSI->RequestProgramInfo() checks whether the required program info is already available in SI Cache
 * and if available it update the modified data to the cache, otherwise it goes to acquire the section information and update
 * SI Cache.
 * @return None
 */
void RMFVODSrcImpl::requestINBProgramInfo()
{
	rmf_Error ret;
	
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[RMFVODSrcImpl::%s:%d] Enter\n", __FUNCTION__, __LINE__);

	if ( 0 != QAMParams.program_no)
	{
#ifndef DISABLE_INBAND_MGR
		ret = pInbandSI->RequestProgramInfo(QAMParams.program_no);
#endif
		if(RMF_SUCCESS != ret )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.VODSRC", "Error in RequestProgramInfo. Source ID: %s\n", saved_uri.c_str() );
			if ( RMF_INBSI_INVALID_PARAMETER == ret )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.VODSRC", "%s: PMT PID not available for program_number %d. Source ID: %s.\n", 
					__FUNCTION__, QAMParams.program_no, saved_uri.c_str());
//				m_si_error =RMF_QAMSRC_ERROR_PROGRAM_NUMBER_INVALID;
//				\(  RMF_QAMSRC_ERROR_PROGRAM_NUMBER_INVALID, NULL);
			}
		}

		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "RMFVODSrcImpl::%s:%d Requested ProgramInfo \n", __FUNCTION__, __LINE__);
	}
	else
	{
		RDK_LOG( RDK_LOG_WARN, "LOG.RDK.VODSRC", "%s:%d Not requesting ProgramInfo since pgm_no is 0 \n", __FUNCTION__, __LINE__);
	}
}

/**
 * @fn void RMFVODSrcImpl::notifyError(RMFResult err, const char* msg)
 * @brief VOD Source implementation function for onError which sends across all the error
 * messages except RMF_QAMSRC_EVENT_CANNOT_DESCRAMBLE_RESOURCES. Using this function we inform player
 * whether error has occurred.
 *
 * @param[in] err error code
 * @param[in] msg In current implementation message will be passed as NULL.
 *
 * @return None
 */
void RMFVODSrcImpl::notifyError(RMFResult err, const char* msg)
{
	char errorMsg[RMF_VODSRC_ERRORSTR_LENGTH] = {'\0'};

	RDK_LOG (RDK_LOG_INFO,"LOG.RDK.VODSRC", "Enter %s:%d\n" , __FUNCTION__, __LINE__);
	if (m_isEosIssued)
	{
		return;
	}
	// Ignore SPTS error
	else if (err == RMF_QAMSRC_ERROR_SPTS_TIMEOUT && !m_streamingState)
	{
		return;
	}
	// Ignore SPTS timeout during paused state.
	else if ((err == RMF_QAMSRC_ERROR_SPTS_TIMEOUT) && (int) (m_speed) == 0) {
		return;
	}
	switch (err)
	{
	case RMF_QAMSRC_EVENT_CANNOT_DESCRAMBLE_ENTITLEMENT:
	case RMF_QAMSRC_EVENT_CANNOT_DESCRAMBLE_RESOURCES:
		err = RMF_VODSRC_ERROR_SRM_ITV_VIDEO_LOST_QAM_OK;
		break;

	case RMF_QAMSRC_ERROR_PMT_NOT_AVAILABLE:
	case RMF_QAMSRC_ERROR_PAT_NOT_AVAILABLE:
		if (m_streamingState)
		{
			err = RMF_VODSRC_ERROR_SRM_ITV_VIDEO_LOST_QAM_OK;
		}
		else
		{
			err = RMF_VODSRC_ERROR_SRM_ITV_TUNING_ERROR;
		}
		break;

	case RMF_QAMSRC_ERROR_TSID_MISMATCH:
		err = RMF_VODSRC_ERROR_SRM_ITV_TUNE_TSID_MISMATCH;
		AutoDiscovery::getInstance()->resetAutoDiscoveryStatus();
		break;

	case RMF_QAMSRC_ERROR_TUNER_UNLOCK_TIMEOUT:
		if (m_streamingState)
		{
			err = RMF_VODSRC_ERROR_SRM_ITV_RF_NOT_AVAILABLE;
		}
		else
		{
			err = RMF_VODSRC_ERROR_SRM_ITV_NO_MPEG_DATA;
		}
		break;

	case RMF_QAMSRC_ERROR_TUNE_FAILED:
		err = RMF_VODSRC_ERROR_SRM_ITV_TUNING_ERROR;
		break;
	case RMF_QAMSRC_ERROR_SPTS_TIMEOUT:
		if (m_streamingState)
		{
			err = RMF_VODSRC_ERROR_SRM_ITV_VIDEO_LOST_QAM_OK;
		}
		break;
	case RMF_VODSRC_ERROR_SE_SESSION_RELEASE:
		memset ( m_session_id, 0x00, sizeof(m_session_id) );
		strncpy ( m_session_id, "deadbeef", sizeof (m_session_id) - 1 );
		if ( true == m_isBaseQamSourceOpen)
		{
			RMFQAMSrcImpl::close();
			m_isBaseQamSourceOpen = false;
		}
		m_adInstance->unregisterVodSource(m_session_id);
		break;
	default:
		break;
	}

	m_lastErrorForTearDownResponse = err;
	RDK_LOG (RDK_LOG_INFO, "LOG.RDK.VODSRC", "m_lastErrorForTearDownResponse=%d\n", m_lastErrorForTearDownResponse);
	m_lastError = err;
	snprintf (errorMsg, (RMF_VODSRC_ERRORSTR_LENGTH - 1), "SRM-%X", m_serverStatus);
	onError(err, errorMsg);

	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC", "Exit %s:%d\n" , __FUNCTION__, __LINE__);
}

/**
 * @fn void RMFVODSrcImpl::notifyStatusChange(unsigned long status)
 * @brief Notify the streaming status change.
 *
 * @param[in] status status code
 *
 * @return None
 */
void RMFVODSrcImpl::notifyStatusChange(unsigned long status)
{
	if (RMF_QAMSRC_EVENT_STREAMING_STARTED == status)
	{
		m_streamingState = true;
	}
	RMFQAMSrcImpl::notifyStatusChange(status);
}

/**
 * @fn void RMFVODSrcImpl::onEOSVOD()
 * @brief A wrapper function to call implementation specific End Of Stream notifier.
 *
 * @retval RMF_RESULT_SUCCESS
 */
void RMFVODSrcImpl::onEOSVOD()
{
	m_isEosIssued = true;
	pause();
	onEOS();
}

void RMFVODSrcImpl::setWaitForClose()
{
	m_doWaitForCloseResponse = true;
}

/**
 * @fn RMFResult RMFVODSrcImpl::addEventHandler( IRMFMediaEvents* events )
 * @brief This function is used to add event handlers for VOD source events. The event could be Playing,
 * Paused, Stopped, Complete, Progress or Status. This is used to notifty the status of the server.
 *
 * @retval RMF_RESULT_SUCCESS 
 */
RMFResult RMFVODSrcImpl::addEventHandler( IRMFMediaEvents* events )
{
	RMFQAMSrcImpl::addEventHandler( events );
	if ( m_serverStatus )
	{
		notifyError( (RMF_VODSRC_ERROR_BASE + m_serverStatus), NULL );
	}

	return RMF_RESULT_SUCCESS;
}

unsigned int RMFVODSrcImpl::getPMTOptimizationPolicy() const
{
	return g_pmt_optimization_level;
}

/**
 * @fn rmf_Error getVodServerId( uint32_t *pVodServerID )
 * @brief This function is used to get the VOD server Id by sending an rmfProxyRequest message to VOD Client.
 * It should be able to get a non negative response value from VOD client and update the same in pVodServerID.
 *
 * @param[out] pVodServerID VOD server Id where result is stored
 *
 * @retval RMF_SUCCESS in case of success
 * @retval RMF_OSAL_ENODATA in case of failure
 */
rmf_Error getVodServerId( uint32_t *pVodServerID )
{
	const int32_t msgBufSz = 1024;
	char t2p_resp_str[ msgBufSz ];
	char t2p_req_str[ msgBufSz ];
	char fieldValue[ msgBufSz ];
	int32_t retVal = RMF_OSAL_ENODATA;
	int32_t recvdMsgSz = 0;

	json_t *msg_root = NULL;
	json_error_t json_error;
	json_t *Response;
	json_t *status;
	json_t *value;
	struct stat sb;

	if( !pVodServerID )
	{
		return retVal;
	}

	if( -1 != gVodServerId )
	{
		*pVodServerID = gVodServerId;
		return RMF_SUCCESS;
	}
	
	if ( stat( "/tmp/vodid_acquired", &sb ) == -1 ) 
	{
		return retVal;
	}

	// Invoke vod client  to get requested field
	snprintf( t2p_req_str, sizeof (t2p_req_str), "{ \"rmfProxyRequest\" : { \"fieldid\" : \"%s\"}} }\n", VOD_SERVER_ID );

	RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.POD",
		"%s::%s length %d\n", __FUNCTION__, t2p_req_str, strlen( t2p_req_str ) );
	
	T2PClient *t2pclient = new T2PClient( );
	if ( t2pclient->openConn( (char* ) VOD_CLIENT_IP, VOD_CLIENT_PORT ) != 0 )
	{
		return retVal;
	}
	
	t2p_resp_str[0] = '\0';
	if ( (recvdMsgSz =
		 t2pclient->sendMsgSync( t2p_req_str, strlen( t2p_req_str ), t2p_resp_str, msgBufSz ) ) 
		 	<= 0)
	{
		goto END_OF_VOD_SRC_ID;
	}
	
	t2p_resp_str[recvdMsgSz] = '\0';
	
	RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.POD",
		"%s: Response : %s\n", __FUNCTION__, t2p_resp_str );

	msg_root = json_loads( t2p_resp_str, 0, &json_error );
	if( !msg_root )
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
			"json_error: on line %d: %s\n", json_error.line, json_error.text );
		goto END_OF_VOD_SRC_ID;
	}

	Response = json_object_get( msg_root, "TriggerRMFProxyResponse" );
	if( !Response )
	{
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",
			"json_error: Response object not found\n");
		goto END_OF_VOD_SRC_ID;
	}

	status = json_object_get(Response, "ResponseStatus" );
	if( !status )
	{
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",
			"json_error: status object not found\n" );
		goto END_OF_VOD_SRC_ID;
	}

	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",
		"json_log: Status = %d\n", json_integer_value(status));
	
	if( json_integer_value( status ) == 0 )
	{
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",
			"json_log: looking for json_object: %s\n", VOD_SERVER_ID );
		value = json_object_get(Response, VOD_SERVER_ID);
		if( !value )
		{
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",
				"json_error: %s object not found\n", VOD_SERVER_ID );
		}
		else
		{
			memset( fieldValue, 0, sizeof (fieldValue) );
			strncpy( fieldValue , json_string_value( value ), sizeof (fieldValue) - 1 ) ;
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",
				"json_log: fieldValue:%s\n", fieldValue );
			
			sscanf( fieldValue, "%d", pVodServerID );
			*pVodServerID = gVodServerId;
			
			retVal = RMF_SUCCESS;
		}
	}
	                                // Release resources
	END_OF_VOD_SRC_ID:
	if( msg_root ) 
	{
		json_decref( msg_root );
	}

	t2pclient->closeConn( );
	delete t2pclient;

	return retVal;	
}

/**
 * @fn void eventNotifier( void* implObj, uint32_t e )
 * @brief This is used to notify an error event to VOD Server.
 *
 * @param[in] implObj object of type RMFVODSrcImpl
 * @param[in] e an error code
 *
 * @return None
 * @}
 */
void eventNotifier( void* implObj, uint32_t e )
{
	((RMFVODSrcImpl*)implObj)->notifyError(	(RMF_VODSRC_ERROR_BASE + e), NULL );
}

