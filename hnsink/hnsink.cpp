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
 * @file hnsink.cpp
 */

#include "hnsink.h"
#include "rmfprivate.h"
#include "rdk_debug.h"
#include "rmf_osal_thread.h"
#include "rmf_osal_sync.h"
#include "rmf_osal_util.h"
#include <semaphore.h>

#include <cassert>
#include <gst/gst.h>
#ifdef USE_DTCP
#include "dtcpmgr.h"
#endif
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_arp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

#define DEFAULT_BUF_SIZE (128*1024)
#define STATUS_POLL_INTERVAL 1

typedef struct
{
	const gchar *padname;
	GstPad *target;
} dyn_link;

//-- HNSinkPrivate ---------------------------------------------------

/**
 * @class HNSinkPrivate
 * @brief This class is used to define the operations required for HN Sink elements by using G-Streamer plugins.
 * This will allow the application to get the data from RMF source or RMF filter element and stream
 * the data to the client box using standard HTTP protocol. This utilizes the G-Streamer encrypt plugin to
 * encrypt the SPTS stream. The current version of RDK supports the DTCP-IP encryption technique for delivering
 * the content over network.
 * <p>
 * This class is responsible for establishing the sink elements, sets the IP address of the client box for
 * which SPTS stream to be transmitted, configure the DTCP parameter for protecting the content over network,
 * register events for monitoring the connection while streaming.
 * @ingroup hnsinkclass
 */
class HNSinkPrivate: public RMFMediaSinkPrivate
{
public:
	HNSinkPrivate(HNSink* parent);
	~HNSinkPrivate();
	void* createElement();

	// from RMFMediaSinkBase
	/*virtual*/ RMFResult setSource(IRMFMediaSource* source);
	RMFResult setSocket(gint);
	RMFResult setRemoteIp(long remote_ip);
	RMFResult setSourceType(char* source_type);
	RMFResult setSourceId(char* source_id);
	virtual void handleCCIChange(char);
	void resetDTCP();
	void setup_dynamic_link (GstElement * element, const gchar * padname, GstPad * target, GstElement * bin);
	void registerEventQ(rmf_osal_eventqueue_handle_t eventqueue);
	void unRegisterEventQ(rmf_osal_eventqueue_handle_t eventqueue);
	rmf_osal_eventmanager_handle_t getEventManager();
	uint32_t getListenerCount();
	/* virtual */RMFResult getRMFError(int error_code);
	void getHttpSessionParams(int *socket, char *ip, unsigned int uiSizeOf_ip, char **url);
	uint64_t getStreamedBytes();
	uint64_t getLastSendTime();
	bool getSendStatus();
	bool mStreamingDone;
	uint64_t mTotalBytesStreamed;
	uint64_t m_bitRate;
	uint32_t m_bitRateCalcInterval;
	uint64_t m_lastSendTime;
	rmf_osal_Cond sink_delete_cond;
	rmf_osal_Cond strm_status_thrd_complete;

private:
	HNSink *m_parent;
	rmf_osal_eventmanager_handle_t eventmanager;
	uint32_t listeners;
	rmf_osal_ThreadId mThid;
	GstElement* m_httpsink;
	GstElement* m_dtcpenc;
	GstElement* m_tee;
	GstElement* m_filesink;
};

/**
 * @fn HNSinkPrivate::HNSinkPrivate(HNSink* parent)
 * @brief A default constructor is used to create an event manager for handling the
 * HN Sink events. This will initialize the member variables, create new condition synchronization objects and
 * read the configuration "FEATURE.HNSINK.BITRATE_INTERVAL" for calculating the video bitrate interval.
 * If the configuration file could not find, the default value 5 seconds will be set.
 *
 * @return None
 */
HNSinkPrivate::HNSinkPrivate(HNSink* parent) :	 RMFMediaSinkPrivate(parent)
{
	rmf_Error ret;
	m_parent = parent;

	eventmanager=0;
	ret = rmf_osal_eventmanager_create ( &eventmanager );
	if (RMF_SUCCESS != ret)
	{
		RDK_LOG(RDK_LOG_FATAL, "LOG.RDK.HN", "%s:Event manager create failed - %d \n", __FUNCTION__, ret);
	}
	listeners = 0;

	m_httpsink =  NULL;
	m_dtcpenc = NULL;
	mStreamingDone = false;
	m_bitRate = 0;
	mTotalBytesStreamed = 0;

	ret = rmf_osal_condNew( TRUE, FALSE, &sink_delete_cond);
	if (RMF_SUCCESS != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.HN", "%s - rmf_osal_condNew for sink_delete_cond failed.\n", __FUNCTION__);
		return;
	}

	ret = rmf_osal_condNew( TRUE, FALSE, &strm_status_thrd_complete);
	if (RMF_SUCCESS != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.HN", "%s - rmf_osal_condNew for strm_status_thrd_complete failed.\n", __FUNCTION__);
		return;
	}

	const char* bitRateInterval = rmf_osal_envGet( "FEATURE.HNSINK.BITRATE_INTERVAL" );
	if ( NULL !=  bitRateInterval )
	{
		m_bitRateCalcInterval = atoi(bitRateInterval);
	}
	else
	{
		m_bitRateCalcInterval = 5;
	}
}

/**
 * @fn HNSinkPrivate::~HNSinkPrivate()
 * @brief A default destructor is used to delete previously created event manager and
 * destroy the condition synchronization objects was created at the time of creating instance for HNSinkPrivate class.
 *
 * @return None
 */
HNSinkPrivate::~HNSinkPrivate()
{
	rmf_osal_eventmanager_delete( eventmanager );
	mStreamingDone = true;
	rmf_osal_condSet( sink_delete_cond );
	rmf_osal_condGet( strm_status_thrd_complete );
	rmf_osal_condDelete( sink_delete_cond );
	rmf_osal_condDelete( strm_status_thrd_complete );
}

/**
 * @fn void HNSinkPrivate::getHttpSessionParams(int *socket, char *ip, char **url)
 * @brief This function is used to get the HTTP session parameter for remote host.
 *
 * @param[out] socket a socket descriptor for which communication to be established between two hosts
 * @param[out] ip IP address of the remote host, example 192.168.30.188
 * @param[out] url url for the content to be stored. example http://"192.168.30.192"/vldms/tuner?ocap_locator=0x1234
 *
 * @return None
 */
void HNSinkPrivate::getHttpSessionParams(int *socket, char *ip, unsigned int uiSizeOf_ip, char **url)
{
	long remote_ip;
	remote_ip = m_parent->m_hnsink_prop.remote_ip;
	snprintf( ip, uiSizeOf_ip, "%d.%d.%d.%d", (remote_ip & 0xff000000) >> 24,
        	    (remote_ip & 0x00ff0000) >> 16, (remote_ip & 0x0000ff00) >> 8, 
            	remote_ip & 0x000000ff);
    g_object_get(m_httpsink, "http-obj", socket, NULL);
	*url = m_parent->m_hnsink_prop.source_id;
}

/**
 * @fn uint64_t HNSinkPrivate::getStreamedBytes()
 * @brief The function is used to get the how many bytes of data has been sent to the remote host.
 * This uses the function g_object_get() for getting the property of an object from gst-plugins.
 *
 * @return Returns a non zero value indicating that how many bytes of data has been transmited.
 */
uint64_t HNSinkPrivate::getStreamedBytes()
{
	uint64_t bytesSent = 0;
    g_object_get(m_httpsink, "sent_data_size", &bytesSent, NULL);
	return bytesSent;
}

/**
 * @fn uint64_t HNSinkPrivate::getLastSendTime()
 * @brief The function is used to get the time stamp of last send() function call.
 * This uses the function g_object_get() for getting the property of an object from gst-plugins.
 *
 * @return Returns the time stamp of last send() call.
 */
uint64_t HNSinkPrivate::getLastSendTime()
{
	uint64_t lastSendTime = 0;
	g_object_get(m_httpsink, "send_data_time", &lastSendTime, NULL);
	return lastSendTime;
}

/**
 * @fn uint64_t HNSinkPrivate::getSendStatus()
 * @brief The function is used to get the current send() function call status.
 * This uses the function g_object_get() for getting the property of an object from gst-plugins.
 *
 * @return Returns the current send() call status.
 */
bool HNSinkPrivate::getSendStatus()
{
	gboolean sendStatus = FALSE;
	g_object_get(m_httpsink, "send_status", &sendStatus, NULL);
	return (bool) sendStatus;
}

/**
 * @fn char* bytesToKMGBytesStr(uint64_t szBytes, char *normStr)
 * @brief This function is used to put the bytes into normStr in a string format. For example,
 * <ul>
 * <li> If the value of szBytes is 1560000000, then the string will be containing "1.56 G"
 * <li> If the value of szBytes is 1500600, then the string will be containing "1.50 M"
 * <li> If the value of szBytes is 4562, then the string will be containing "4.56 K"
 * <li> If the value of szBytes is 436, then the string will be containing "463"
 * </ul>
 *
 * @param[in] szBytes A value for which formated string need to be considered
 * @param[out] normStr A output string will be containing formated value to identify Giga, Mega, Kelo bytes
 * @param[in] uiNormStrSize sizeof "normStr".
 *
 * @return Returns the normStr
 */
char* bytesToKMGBytesStr(uint64_t szBytes, char *normStr, long unsigned int uiNormStrSize)
{
    double kilobytes = 0;
    double megabytes = 0;
    double gigabytes = 0;

    if (szBytes >= 1000000000)  //Giga
    {
        gigabytes = szBytes / 1000000000.0;
        snprintf(normStr, uiNormStrSize, "%.2f G", gigabytes);

    }
    else if (szBytes >= 1000000)  //Mega
    {
        megabytes = szBytes / 1000000.0;
        snprintf(normStr, uiNormStrSize, "%.2f M", megabytes);
    }
    else if (szBytes >= 1000)  //Kilo
    {
        kilobytes = szBytes / 1000.0;
        snprintf(normStr, uiNormStrSize, "%.2f K", kilobytes);
    }
    else
    {
        snprintf(normStr, uiNormStrSize, "%lld", szBytes);
    }

    return normStr;
}

/**
 * @fn void streamStatusThread (void* arg)
 * @brief A thread function is used to get how much data is sent to the remote host by using the function getStreamedBytes().
 * This will update the bitrate and streaming elapsed time periodically after few seconds interval and keep them updated to
 * the log file for monitoring. The thread will be terminated when the streaming gets over.
 *
 * @param[in] arg An object pointer for which need to be checked for
 *
 * @return None
 */
void streamStatusThread (void* arg)
{
	HNSinkPrivate *hnsp = (HNSinkPrivate *)arg;
	uint64_t bytesSent;
	uint64_t bitRate;
	char ipStr[50] = "localhost";
	char byteStr[1024];
	char bitRateStr[64];
	int sockId = -1;
	char *url = NULL;
	int elapsedTime = 0;
	struct timeval time;
	uint64_t lastSendTime = 0;

	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.HN", "streamStatusThread Begin\n");	
	while (!hnsp->mStreamingDone)
	{
		bytesSent = hnsp->getStreamedBytes();
		if (bytesSent < hnsp->mTotalBytesStreamed)
			hnsp->mTotalBytesStreamed = 0;
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.HN", "sentdatasize = %lld, prevSent = %lld\n", bytesSent, hnsp->mTotalBytesStreamed);	
		elapsedTime++;
		bitRate = ((bytesSent - hnsp->mTotalBytesStreamed) * 8)/(elapsedTime*hnsp->m_bitRateCalcInterval);
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.HN", "bitRate = %lld\n", bitRate);

		if (abs((long long)(hnsp->m_bitRate - bitRate)) > hnsp->m_bitRate/2 || (elapsedTime*hnsp->m_bitRateCalcInterval) >= 30)
		{
			elapsedTime = 0;
			hnsp->m_bitRate = bitRate;
			hnsp->mTotalBytesStreamed = bytesSent;
			hnsp->getHttpSessionParams(&sockId,ipStr, sizeof(ipStr),&url);
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.HN", "ConnId [%d] Client IP [%s] uri [%s] Bytes sent [%s] Bit Rate [%sbps]\n",
				sockId,ipStr, url, bytesToKMGBytesStr(bytesSent, byteStr, sizeof(byteStr)), bytesToKMGBytesStr(bitRate, bitRateStr, sizeof(bitRateStr)));
		}

		lastSendTime = hnsp->getLastSendTime();
		gettimeofday(&time, NULL);
		if ((time.tv_sec - lastSendTime) > hnsp->m_bitRateCalcInterval && lastSendTime != hnsp->m_lastSendTime)
		{
			hnsp->m_lastSendTime = lastSendTime;
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.HN", "ConnId [%d] Client IP [%s] uri [%s] last send() time=%lld current time=%ld send status=%s\n",
				 sockId, ipStr, url ? url : "None", lastSendTime, time.tv_sec, hnsp->getSendStatus() ? "Blocked" : "Free");
		}

		rmf_osal_condWaitFor( hnsp->sink_delete_cond, hnsp->m_bitRateCalcInterval*1000 );
	}
	rmf_osal_condSet( hnsp->strm_status_thrd_complete );
}

/**
 * @fn void* HNSinkPrivate::createElement()
 * @brief This function is used to create an empty bin name "hnsink_bin_<IP address of the remote device>" for
 * HN Sink elements. This is used to create a light weight thread streamStatusThread() for monitoring the
 * amount of data transmitted to the client box.
 * <p>
 * If in case the dtcp is enabled for this device, these two gstreamer element "httpsink" and "dtcpenc" will
 * add to the bin and link them between each other. Add a ghostpad by using gst_element_get_static_pad() to
 * the bin so it can proxy to the demuxer. These are the following properties are set for "dtcpenc" plugin.
 * <ul>
 * <li> "cci" is used to set the CCI (Copy Control Information) value such as 0x00=Copy Freely,
 * 0x01=Copy No More, 0x02=Copy Once or 0x03=Copy Never.
 * <li> "remoteip" is used to set the IP address of remote host for which DTCP encrypted data to be sent.
 * <li> "keylabel" is used to set property for the DTCP key level
 * </ul>
 *
 * @return Returns a void pointer contains GstElement.
 */
void* HNSinkPrivate::createElement()
{
	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.HN", "HNSinkPrivate::createElement enter\n");
	char ipStr[50];
	char binName[100];
	long remote_ip = m_parent->m_hnsink_prop.remote_ip;
	snprintf( ipStr, sizeof(ipStr),"%d.%d.%d.%d", (remote_ip & 0xff000000) >> 24,
		(remote_ip & 0x00ff0000) >> 16, (remote_ip & 0x0000ff00) >> 8, 
            	remote_ip & 0x000000ff );
	snprintf( binName, sizeof(binName), "hnsink_bin_%s", ipStr ); 

	// Create a bin to contain the sink elements.
#ifdef RMF_FCC_ENABLED
	GstElement* bin = gst_element_factory_make("bin", NULL); // for client with same ip address
#else
	GstElement* bin = gst_element_factory_make("bin", binName); // a unique name will be generated
#endif
	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.HN", "creating elements\n");

	const char *dumpenv = rmf_osal_envGet("FEATURE.DUMP.SRC.OUTPUT");
	//This flag is no more in-use. To dump data on src side, use "MEDIASTREAMER.STREAMING.DATADUMP=1" in rmfconfig.ini file
	dumpenv = NULL;
	if(m_parent->m_hnsink_prop.dtcp_enabled)
	{
		RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.HN", "creating dtcp encrypt elements\n");
		m_dtcpenc = gst_element_factory_make("dtcpenc", "dtcp-encrypt");
		if (!m_dtcpenc)
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.HN", "Failed to instantiate dtcp encrypt\n");
			assert( m_dtcpenc );
		}

		RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.HN", "Setting CCI value = '%d' as cci property and remote_ip = '%s' to dtcpenc element\n", m_parent->m_hnsink_prop.dtcp_cci, ipStr);
		g_object_set (G_OBJECT (m_dtcpenc), "cci", m_parent->m_hnsink_prop.dtcp_cci, NULL);
		g_object_set(G_OBJECT (m_dtcpenc), "remoteip", ipStr, NULL);
		if(m_parent->m_hnsink_prop.dtcp_key_label)
			g_object_set (G_OBJECT (m_dtcpenc), "keylabel", m_parent->m_hnsink_prop.dtcp_key_label, NULL);

		RMF_registerGstElementDbgModule("dtcpenc",  "LOG.RDK.HN");	
#if 0  /*Dead Code As dumpenv = NULL. CID 16885*/
		if (NULL != dumpenv)
		{
			char location[100];
			snprintf(location, sizeof(location),"/opt/SRC_OUT_%s.ts", ipStr);
			m_tee = gst_element_factory_make("tee", "tee-element");
			m_filesink = gst_element_factory_make("filesink", "file-sink");
			g_object_set (G_OBJECT (m_filesink), "location", location, NULL);
		}
#endif
	}

	m_httpsink = gst_element_factory_make("httpsink", "http-sink");
	if (!m_httpsink)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.HN", "Failed to instantiate http sink\n");
		assert( m_httpsink );
	}
	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.HN", "setting socket(%d) and stream-type(%d) to httpsink element initially \n", m_parent->m_hnsink_prop.socketId, m_parent->m_hnsink_prop.use_chunked_transfer);
	g_object_set(m_httpsink, "http-obj", m_parent->m_hnsink_prop.socketId, NULL);
	g_object_set(m_httpsink, "stream-type", m_parent->m_hnsink_prop.use_chunked_transfer, NULL);
	RMF_registerGstElementDbgModule("httpsink",  "LOG.RDK.HN");

	GstPad *pad1 = NULL;
	if(m_parent->m_hnsink_prop.dtcp_enabled)
	{
#if 0 /*Dead code as dumpenv = NULL CID 16885*/
		if (NULL != dumpenv)
		{
			gst_bin_add_many(GST_BIN(bin), m_tee, m_filesink, m_dtcpenc,m_httpsink , NULL);
			assert(gst_element_link_pads( m_tee, "src1", m_dtcpenc, "sink") == true);
			assert(gst_element_link( m_dtcpenc, m_httpsink) == true);
			assert(gst_element_link_pads( m_tee, "src2", m_filesink, "sink") == true);

			// Add a ghostpad to the bin so it can proxy to the demuxer.
			pad1 = gst_element_get_static_pad(m_tee, "sink");
		}
		else
#endif
		{
			gst_bin_add_many(GST_BIN(bin), m_dtcpenc,m_httpsink , NULL);
			assert(gst_element_link_many( m_dtcpenc,m_httpsink, NULL) == true);

			// Add a ghostpad to the bin so it can proxy to the demuxer.
			pad1 = gst_element_get_static_pad(m_dtcpenc, "sink");
		}
	}
	else
	{
			gst_bin_add(GST_BIN(bin),m_httpsink );

			// Add a ghostpad to the bin so it can proxy to the demuxer.
			pad1 = gst_element_get_static_pad(m_httpsink, "sink");
	}

	gst_element_add_pad(bin, gst_ghost_pad_new("sink", pad1));
	gst_object_unref(GST_OBJECT(pad1));

	rmf_Error ret = RMF_SUCCESS;
	ret = rmf_osal_threadCreate( streamStatusThread, this, RMF_OSAL_THREAD_PRIOR_DFLT,
                        RMF_OSAL_THREAD_STACK_SIZE, &mThid,"StreamStatus" );

	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.HN", "HNSinkPrivate::createElement exit\n");
	return bin;
}

/**
 * @fn void HNSinkPrivate::handleCCIChange(char cciInfo)
 * @brief This function is used to handle change in the CCI (Copy Control Information) and set the CCI
 * value using set property for dtcp plugin.
 *
 * @param[in] cciInfo CCI value could be 0x00=Copy Freely, 0x01=Copy No More, 0x02=Copy Once or 0x03=Copy Never.
 *
 * @return None
 */
void HNSinkPrivate::handleCCIChange(char cciInfo)
{
	//set CCI on dtcp encrypt element...
	if(!m_dtcpenc)
		return;

	m_parent->m_hnsink_prop.dtcp_cci= cciInfo;
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.HN", "Changing CCI value = '%d' as cci property to dtcpenc element\n", m_parent->m_hnsink_prop.dtcp_cci);
	g_object_set (G_OBJECT (m_dtcpenc), "cci", m_parent->m_hnsink_prop.dtcp_cci, NULL);
  
	return;
}

/**
 * @fn void HNSinkPrivate::resetDTCP()
 * @brief This function is used to reset the DTCP session to clean up residue data. This will
 * use the g_object_set() by sending "reset" for "dtcpenc" element.
 *
 * @return None
 */
void HNSinkPrivate::resetDTCP()
{
	if(!m_dtcpenc)
		return;

	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.HN", "setting dtcpenc element reset property\n");
	g_object_set (G_OBJECT (m_dtcpenc), "reset", TRUE, NULL);
  
	return;
}

/**
 * @fn RMFResult HNSinkPrivate::setSocket(gint socket)
 * @brief This function is used to set a socket descriptor for "httpsink" element.
 *
 * @param[in] socket Socket descriptor.
 *
 * @retval RMF_RESULT_SUCCESS This is able to set the socket descriptor for httpsink element.
 * @retval RMF_RESULT_INVALID_ARGUMENT httpsink sink element is not created.
 */
RMFResult HNSinkPrivate::setSocket(gint socket)
{
	//setsocket
	if(!m_httpsink)
		return RMF_RESULT_INVALID_ARGUMENT;

	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.HN", "setting socket(%d) to httpsink element\n", socket);
	m_parent->m_hnsink_prop.socketId = socket;
	g_object_set(m_httpsink, "http-obj", m_parent->m_hnsink_prop.socketId, NULL);
  
	return RMF_RESULT_SUCCESS;
}

/**
 * @fn RMFResult HNSinkPrivate::setRemoteIp(long remote_ip)
 * @brief This function is used to set the remote IP address for DTCP encrypt element.
 * This will convert the remote_ip to a string and sets the string formated IP address (example, 192.168.20.129)
 * to DTCP encrypt element by uisng the function g_object_set()
 *
 * @param[in] remote_ip IP address of the remote host
 *
 * @retval RMF_RESULT_SUCCESS On success.
 * @retval RMF_RESULT_INVALID_ARGUMENT DTCP encrypt element is not yet created
 */
RMFResult HNSinkPrivate::setRemoteIp(long remote_ip)
{
	//set Remote IP address on dtcp encrypt element...
	if(!m_dtcpenc)
		return RMF_RESULT_INVALID_ARGUMENT;

	m_parent->m_hnsink_prop.remote_ip = remote_ip;

	char ipStr[50];
	snprintf( ipStr, sizeof(ipStr),"%d.%d.%d.%d", (remote_ip & 0xff000000) >> 24,
            (remote_ip & 0x00ff0000) >> 16, (remote_ip & 0x0000ff00) >> 8,
            remote_ip & 0x000000ff);

	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.HN", "setting remoteip(%s) to dtcpenc element\n", ipStr);
	g_object_set(G_OBJECT (m_dtcpenc), "remoteip", ipStr, NULL);
  
	return RMF_RESULT_SUCCESS;
}

/**
 * @fn RMFResult HNSinkPrivate::setSourceType(char* source_type)
 * @brief This function is used to set the source type of HNSink element using httpsink plugin's property "source-type".
 * The source type could be QAM, DVR, VOD, TSB and IPPV. The name of the source type used when logging first packet
 * from these source.
 *
 * @param[in] source_type The source type could be "QAM_SRC"
 *
 * @retval RMF_RESULT_SUCCESS On success.
 * @retval RMF_RESULT_INVALID_ARGUMENT http sink element is not yet created
 */
RMFResult HNSinkPrivate::setSourceType(char* source_type)
{
	if(!m_httpsink)
	{
		return RMF_RESULT_INVALID_ARGUMENT;
	}
	memset(m_parent->m_hnsink_prop.source_type, 0, sizeof(m_parent->m_hnsink_prop.source_type));
	strncpy(m_parent->m_hnsink_prop.source_type, source_type, sizeof(m_parent->m_hnsink_prop.source_type)-1);
	g_object_set(m_httpsink, "source-type", m_parent->m_hnsink_prop.source_type, NULL);
	
	return RMF_RESULT_SUCCESS;
}

/**
 * @fn RMFResult HNSinkPrivate::setSourceId(char* source_id)
 * @brief This function is used to set the source Id of HNSink element using httpsink element. This sets the
 * plugin's property "source-id" to "httpsink" element and the source Id format could be ocap://0xXXXX,
 * dvr://local/xxxx#0, vod://<string>, etc.
 *
 * @param[in] source_id This is the source id from where to get the data
 *
 * @retval RMF_RESULT_SUCCESS On success.
 * @retval RMF_RESULT_INVALID_ARGUMENT http sink element is not yet created
 */
RMFResult HNSinkPrivate::setSourceId(char* source_id)
{
	if(!m_httpsink)
	{
		return RMF_RESULT_INVALID_ARGUMENT;
	}
	memset(m_parent->m_hnsink_prop.source_id, 0, sizeof(m_parent->m_hnsink_prop.source_id));
	strncpy(m_parent->m_hnsink_prop.source_id, source_id, sizeof(m_parent->m_hnsink_prop.source_id)-1);
	g_object_set(m_httpsink, "source-id", m_parent->m_hnsink_prop.source_id, NULL);
	
	return RMF_RESULT_SUCCESS;
}

/**
 * @fn RMFResult HNSinkPrivate::setSource(IRMFMediaSource* source)
 * @brief This function is used to remove the sink element which is currently being used and set the new sink element.
 * If in case the source element is exist, the function will update the cci value to the hnsink properties, otherwise 0 is set.
 *
 * @param[in] source An object of IRMFMediaSource class
 *
 * @retval RMF_RESULT_SUCCESS Successfully set the Source
 * @retval RMF_RESULT_NOT_INITIALIZED Sink element is not yet created
 * @retval RMF_RESULT_INTERNAL_ERROR Unable to remove the existing sink element from the source
 * if exist or unable to add the sink element.
 */
RMFResult HNSinkPrivate::setSource(IRMFMediaSource* source)
{
	if ( source )
	{
	   RMFResult result;
	   char cciInfo= 0;
	   result= source->getPrivateSourceImpl()->getCCIInfo( cciInfo );
	   m_parent->m_hnsink_prop.dtcp_cci= cciInfo;
	}
	else
	{
	   m_parent->m_hnsink_prop.dtcp_cci= 0;
	}
	return RMFMediaSinkPrivate::setSource(source);
}

/**
 * @fn void HNSinkPrivate::registerEventQ(rmf_osal_eventqueue_handle_t eventqueue)
 * @brief The function is used to register an event queue to the event manager for HN Sink.
 *
 * @param[in] eventqueue eventqueue handler of type uint32_t
 *
 * @return None
 */
void HNSinkPrivate::registerEventQ(rmf_osal_eventqueue_handle_t eventqueue)
{
	rmf_Error ret;
	ret = rmf_osal_eventmanager_register_handler(
			eventmanager,
			eventqueue,
			RMF_OSAL_EVENT_CATEGORY_HN);
	if (RMF_SUCCESS != ret)
	{
		RDK_LOG(RDK_LOG_FATAL, "LOG.RDK.HN", "%s:Register events failed - %d \n", __FUNCTION__, ret);
	}
	else
	{
		listeners++;
	}
}

/**
 * @fn void HNSinkPrivate::unRegisterEventQ(rmf_osal_eventqueue_handle_t eventqueue)
 * @brief The function is used to unregister an event queue to event manager for HN Sink.
 * This is to called if event queue do not want to receive any more events from the event manager.
 *
 * @param[in] eventqueue eventqueue handler of type uint32_t
 *
 * @return None
 */
void HNSinkPrivate::unRegisterEventQ(rmf_osal_eventqueue_handle_t eventqueue)
{
	rmf_Error ret;
	ret = rmf_osal_eventmanager_unregister_handler(
			eventmanager,
			eventqueue
			);
	if (RMF_SUCCESS != ret)
	{
		RDK_LOG(RDK_LOG_FATAL, "LOG.RDK.HN", "%s:Register events failed - %d \n", __FUNCTION__, ret);
	}
	else
	{
		listeners--;
	}
}

/**
 * @fn rmf_osal_eventmanager_handle_t HNSinkPrivate::getEventManager()
 * @brief This function is used to get the eventmanager Id which has created through this function rmf_osal_eventmanager_create().
 *
 * @return None
 */
rmf_osal_eventmanager_handle_t HNSinkPrivate::getEventManager()
{
	return eventmanager;
}

/**
 * @fn uint32_t HNSinkPrivate::getListenerCount()
 * @brief This function is used to return the total number of listeners are currently handle by HNSink.
 *
 * @return Returns a non-zero value indicating that the number of listeners are being handled by HN sink, returns 0 if no listeners are being present
 */
uint32_t HNSinkPrivate::getListenerCount()
{
	return listeners;
}

/**
 * @fn RMFResult HNSinkPrivate::getRMFError(int error_code)
 * @brief This function is used to get the error code for HN Sink. The error code event could be connection closed
 * during communication, event for failing the DTCP authentication and key exchange.
 *
 * @param error_code The RMF error code
 *
 * @retval RMF_RESULT_FAILURE Unknown error generated by G-Streamer plugin
 * @retval RMF_HNSINK_EVENT_CONNECTION_CLOSED Events for connection closed during communication.
 * @retval RMF_HNSINK_EVENT_DTCP_AKE_FAILED DTCP authentication and key exchange error.
 */
RMFResult HNSinkPrivate::getRMFError(int error_code)
{
	RMFResult rmf_error = RMF_RESULT_FAILURE;

	switch(error_code)
	{
		case hnSinkError::httpsink_event_connection_closed:
				RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.HN", "%s : Received GST_HTTPSINK_EVENT_CONNECTION_CLOSED\n", __FUNCTION__);
				rmf_error = RMF_HNSINK_EVENT_CONNECTION_CLOSED;
			break;

		case hnSinkError::dtcpenc_event_ake_failed:
				RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.HN", "%s : Received GST_DTCPENC_EVENT_AKE_FAILED\n", __FUNCTION__);
				rmf_error = RMF_HNSINK_EVENT_DTCP_AKE_FAILED;
			break;

		default:
				RDK_LOG(RDK_LOG_INFO, "LOG.RDK.HN", "%s : Unknown event from GST plugin = %d \n", __FUNCTION__,error_code);
				rmf_error = RMF_RESULT_FAILURE;
			break;			
	}

	return rmf_error;
}

/*
* isVidiPathEnabled()
* If /opt/vidiPathEnabled does not exist, indicates VidiPath is NOT enabled.
* If /opt/vidiPathEnabled exists, VidiPath is enabled.
*/
#define VIDIPATH_FLAG "/opt/vidiPathEnabled"

static int isVidiPathEnabled()
{
    if (access(VIDIPATH_FLAG,F_OK) == 0)
        return 1; //vidipath enabled
    return 0;     //vidipath not enabled
}

/*
* cvpIfCheckForDtcpThread()
* Waits for CVP interface to acquire an IP address before starting DTCP server on the interface.
*/
static void cvpIfCheckForDtcpThread (void* arg)
{
    int dtcpPort= (intptr_t)(arg);
    char *cvp2interfaceName = (char *)rmf_osal_envGet("FEATURE.CVP2.MRDVR.MEDIASERVER.IFNAME");
    if ((cvp2interfaceName != NULL) && (0 != strcmp(cvp2interfaceName, "lo")))
    {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.HN","%s::DTCP CVP2 Start the Source at interface '%s' at port:%d\n", __FUNCTION__, cvp2interfaceName, dtcpPort);
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        
        if(sock != -1)
        {
            while(1)
            {
                struct ifreq ifr;
                memset(&ifr,0,sizeof(ifr));
                strncpy(ifr.ifr_name, cvp2interfaceName, IFNAMSIZ);
                ifr.ifr_name[IFNAMSIZ - 1] = 0;
                ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
                
                int result = 0;

                if ((result = ioctl(sock, SIOCGIFADDR, &ifr)) < 0)
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.HN","%s(): Cannot get IP addr for %s, ret %d.\n", __FUNCTION__,ifr.ifr_name, result);
                }
                else
                {
                    struct sockaddr_in localifaddr;
                    memset(&localifaddr,0,sizeof(localifaddr));
                    memcpy((char *)&localifaddr, (char *) &ifr.ifr_ifru.ifru_addr, sizeof(struct sockaddr));
                    
                    char szIpAddr[256] = "";
                    if(NULL == inet_ntop( AF_INET, &localifaddr.sin_addr, szIpAddr, sizeof(szIpAddr)))
                    {
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.HN","%s(): Error: inet_ntop returned NULL.\n", __FUNCTION__);
                    } 
                    else if(0 == strlen(szIpAddr))
                    {
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.HN","%s(): Error: inet_ntop returned '%s'.\n", __FUNCTION__, szIpAddr);
                    }
                    else if((0 == strcmp(szIpAddr, "127.0.0.2")) || (0 == strcmp(szIpAddr, "192.168.0.1")))
                    {
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.HN","%s(): ERROR: IP Address of '%s' is still the default value: '%s'.\n", __FUNCTION__, ifr.ifr_name, szIpAddr);
                    }
                    else
                    {
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.HN","%s(): IP Address of '%s' is '%s'.\n", __FUNCTION__, ifr.ifr_name, szIpAddr);
                        break; // Success
                    }
                }
                sleep(60); // Failure, so repeat.
            }
            close(sock);
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.HN","%s(): Could not create socket.\n", __FUNCTION__);
        }
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.HN","%s(): Starting CVP2 DTCP Server for '%s'.\n", __FUNCTION__, cvp2interfaceName);
        int32_t error_code = DTCPMgrStartSource(cvp2interfaceName, dtcpPort);
        if (0 > error_code)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.HN", "%s : Starting CVP2 DTCP Server failed...  retVal = %d", __FUNCTION__, error_code);
        }
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.HN","%s(): Error: Value of FEATURE.CVP2.MRDVR.MEDIASERVER.IFNAME is non-existing / invalid.\n", __FUNCTION__);
    }
}

RMFResult HNSink::init_platform()
{
       int32_t error_code;
       int dtcpPort= 5010;
       bool dtcpsupport_enabled = FALSE;
       
#ifdef USE_DTCP
       const char *dtcpenable = rmf_osal_envGet( "FEATURE.DTCP.SUPPORT" );

       if ((dtcpenable != NULL) && (0 == strcmp(dtcpenable, "TRUE")))
       {
           dtcpsupport_enabled = TRUE;
       }

       if(dtcpsupport_enabled)
       {
           RDK_LOG(RDK_LOG_INFO, "LOG.RDK.HN", "%s : DTCP/IP Support Enabled ", __FUNCTION__);
	   const char *env= rmf_osal_envGet( "FEATURE.MPEOS.HN.DTCP.PORT" );
	   if ( env )
	   {
		   int port= atoi(env);
		   if ( port != 0 )
		   {
			   dtcpPort= port;
		   }
	   }  

	   RDK_LOG(RDK_LOG_INFO, "LOG.RDK.HN", "%s : DTCP/IP Stack Initializing ", __FUNCTION__);
	   error_code = DTCPMgrInitialize();
	   if ( 0 > error_code )
	   {
		   RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.HN", "%s : DTCP/IP Stack Initialize failed... retVal = %d", __FUNCTION__, error_code);
		   return RMF_RESULT_FAILURE;
	   }	

	   int dtcpLoglevel=3;
	   env = rmf_osal_envGet("FEATURE.DTCP.LOGLEVEL");
	   if (NULL != env) 
	   {
		   dtcpLoglevel = atoi(env);
		   if((dtcpLoglevel < 1) || (dtcpLoglevel > 5)) 
		   {
			   RDK_LOG(RDK_LOG_INFO, "LOG.RDK.HN", "%s::DTCP MPEENV LOGLEVEL is Invalid:%d\n", __FUNCTION__, dtcpLoglevel);
			   dtcpLoglevel = 3;
		   }
	   }
	   RDK_LOG(RDK_LOG_INFO,"LOG.RDK.HN","%s::DTCP LOGLEVEL is:%d\n", __FUNCTION__, dtcpLoglevel);
	   error_code = DTCPMgrSetLogLevel(dtcpLoglevel);
	   if (0 > error_code) 
	   {
		   RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.HN","%s::DTCP Set LOGLEVEL:%d is failed:%d\n", __FUNCTION__, dtcpLoglevel, error_code);
	   }

	   char * tmpMediaServerIf = "lo";
	   RDK_LOG(RDK_LOG_INFO,"LOG.RDK.HN","%s::DTCP Start the Source at interface '%s' at port:%d\n", __FUNCTION__, tmpMediaServerIf, dtcpPort);
	   error_code = DTCPMgrStartSource(tmpMediaServerIf, dtcpPort);
	   if (0 > error_code)
	   {
	       RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.HN", "%s : Starting DTCP Server failed...  retVal = %d", __FUNCTION__, error_code);
	       return RMF_RESULT_FAILURE;
	   }
	   char *interfaceName = (char *)rmf_osal_envGet("FEATURE.MRDVR.MEDIASERVER.IFNAME");
	   if ((interfaceName != NULL) && (0 != strcmp(interfaceName, "lo")))
	   {
       	   RDK_LOG(RDK_LOG_INFO,"LOG.RDK.HN","%s::DTCP Start the Source at interface '%s' at port:%d\n", __FUNCTION__, interfaceName, dtcpPort);
       	   error_code = DTCPMgrStartSource(interfaceName, dtcpPort);
       	   if (0 > error_code)
       	   {
       	       RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.HN", "%s : Starting DTCP Server failed...  retVal = %d", __FUNCTION__, error_code);
       	       return RMF_RESULT_FAILURE;
       	   }
	   }
       }
       else
       {
           RDK_LOG(RDK_LOG_INFO, "LOG.RDK.HN", "%s : DTCP/IP Support Disabled ", __FUNCTION__);
       }
       if (isVidiPathEnabled())
       {
            rmf_Error ret = RMF_SUCCESS;
            rmf_osal_ThreadId threadId = 0;
            ret = rmf_osal_threadCreate( cvpIfCheckForDtcpThread, (void*)(dtcpPort), RMF_OSAL_THREAD_PRIOR_DFLT,
                                            RMF_OSAL_THREAD_STACK_SIZE, &threadId,"cvpIfCheckForDtcpThread" );
       }
#endif
       return RMF_RESULT_SUCCESS;
}

/**
 * @fn HNSink::uninit_platform()
 * @brief This API is used to uninitialize the HNSink platform functionality by stopping DTCP server.
 *
 * @return RMFResult
 * @retval RMF_RESULT_SUCCESS Successfully uninitialized the platform
 * @retval RMF_RESULT_FAILURE If DTCP server failed to stop.
 */
RMFResult HNSink::uninit_platform()
{
#ifdef USE_DTCP
       bool dtcpsupport_enabled = FALSE;

      const char *dtcpenable = rmf_osal_envGet( "FEATURE.DTCP.SUPPORT" );

       if ((dtcpenable != NULL) && (0 == strcmp(dtcpenable, "TRUE")))
       {
           dtcpsupport_enabled = TRUE;
       }

       if(dtcpsupport_enabled)
       {
           RDK_LOG(RDK_LOG_INFO, "LOG.RDK.HN", "%s : DTCP/IP Support Enabled.. Stopping DTCPMgr  ", __FUNCTION__);
           int32_t error_code = DTCPMgrStopSource();
	   
           if ( 0 > error_code )
           {
               RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.HN", "%s : DTCP Server Stop failed... retVal = %d", __FUNCTION__, error_code );
               return RMF_RESULT_FAILURE;
           }	
       }
       else
       {
           RDK_LOG(RDK_LOG_INFO, "LOG.RDK.HN", "%s : DTCP/IP Support Disabled ", __FUNCTION__);
       }
#endif
       return RMF_RESULT_SUCCESS;
}

/**
 * @fn HNSink::createElement()
 * @brief This API is used to create gstreamer plugins "httpsink" and "dtcpenc".
 *
 * These two plugins are linked together and added to the bin element and called as HN Sink element.
 * This element uses socket to transfer the data to the client devices.
 * "httpsink" plugin's properties 'http-obj' and 'stream-type' are set with socket-id and stream type.
 * Similarly "dtcpenc" plugin's poperties 'cci', 'remotrip' and 'keylabel are set according to copy control information, remoteip and dtcp key label.
 *
 * @return It returns a pointer which will be the instance of HNSink.
 * @}
 */
void* HNSink::createElement()
{
	return static_cast<HNSinkPrivate*>(getPrivateSinkImpl())->createElement();
}


RMFMediaSinkPrivate* HNSink::createPrivateSinkImpl()
{
	return new HNSinkPrivate(this);
}

/**
 * @addtogroup hnsinkapi
 * @{
 * @fn HNSink::registerEventQ(rmf_osal_eventqueue_handle_t eventqueue)
 * @brief This API will register an event queue to event manager for a particular event category.
 *
 * @param[in] eventqueue Handle of eventqueue
 *
 * @return None
 */
void HNSink::registerEventQ(rmf_osal_eventqueue_handle_t eventqueue)
{
	static_cast<HNSinkPrivate*>(getPrivateSinkImpl())->registerEventQ(eventqueue);
}

/**
 * @fn HNSink::setHNSinkProperties(HNSinkProperties_t prop)
 * @brief This API is used to set the HNSink properties 'socketid', 'remoteip', 'dtcp_enabled', 'dtcp_cci', 'url', 'source type' and 'source id'.
 *
 * @param[in] prop HNSinkProperties
 *
 * @return None
 */
void HNSink::setHNSinkProperties(HNSinkProperties_t prop)
{
	memset(&m_hnsink_prop, 0, sizeof(m_hnsink_prop));
	memcpy(&m_hnsink_prop, &prop, sizeof(prop));
}

/**
 * @fn HNSink::setSocket(int socketId)
 * @brief This API is used to set socketId to httpsink plugin using the gstreamer property "http-obj".
 *
 * @param[in] socketId Unique socketId to emit data.
 *
 * @return None
 */
void HNSink::setSocket(int socketId)
{
	static_cast<HNSinkPrivate*>(getPrivateSinkImpl())->setSocket(socketId);
}

/**
 * @fn HNSink::setRemoteIp(long remote_ip)
 * @brief This API is used to set Remote IP address on dtcp encrypt element using gstreamer property "remote ip".
 *
 * @param[in] remote_ip Remote ip address
 *
 * @return None
 */
void HNSink::setRemoteIp(long remote_ip)
{
	static_cast<HNSinkPrivate*>(getPrivateSinkImpl())->setRemoteIp(remote_ip);
}

/**
 * @fn HNSink::setUrl(char* url)
 * @brief This API is used to set the url of HNSink property.
 *
 * @param[in] url Source url
 *
 * @return None
 */
void HNSink::setUrl(char *url)
{
	strncpy(m_hnsink_prop.url, url, sizeof(m_hnsink_prop.url));
	m_hnsink_prop.url[sizeof(m_hnsink_prop.url)-1] = 0;
}

/**
 * @fn HNSink::getUrl()
 * @brief This API is used to get the url of HNSink property.
 *
 * @return It returns the pointer to the URL.
 */
char* HNSink::getUrl()
{
	return m_hnsink_prop.url;
}

/**
 * @fn HNSink::getBitrate()
 * @brief This API is used to get the current active bitrate for the stream.
 *
 * @return It returns the current active bitrate.
 */
unsigned long HNSink::getBitrate()
{
	HNSinkPrivate* pSinkPrivate = ((static_cast<HNSinkPrivate*>(getPrivateSinkImpl())));
	if(NULL != pSinkPrivate)
	{
		return pSinkPrivate->m_bitRate;
	}
	return 0;
}

/**
 * @fn HNSink::getMBStreamed()
 * @brief This API is used to get the total MB streamed.
 *
 * @return It returns the total MB streamed.
 */
unsigned long HNSink::getMBStreamed()
{
	HNSinkPrivate* pSinkPrivate = ((static_cast<HNSinkPrivate*>(getPrivateSinkImpl())));
	if(NULL != pSinkPrivate)
	{
		return (pSinkPrivate->getStreamedBytes()/(1000*1000));
	}
	return 0;
}

/**
 * @fn HNSink::setSourceType(char* source_type)
 * @brief This API is used to set the source type of HNSink element using httpsink plugin's property "source-type".
 *
 * @param[in] source_type Type of source such ad qamsource, HNSource and DVRSource.
 *
 * @return None
 */
void HNSink::setSourceType(char* source_type)
{
	static_cast<HNSinkPrivate*>(getPrivateSinkImpl())->setSourceType(source_type);
}

/**
 * @fn HNSink::setSourceId(char* source_id)
 * @brief This API is used to set the source id of HNSink element using httpsink plugin's property "source-id".
 *
 * @param[in] source_id Source_id[ocap id] value to identify a particular service
 *
 * @return None
 */
void HNSink::setSourceId(char* source_id)
{
	static_cast<HNSinkPrivate*>(getPrivateSinkImpl())->setSourceId(source_id);
}

/**
 * @fn HNSink::handleCCIChange(char cciInfo)
 * @brief This API is used to set copy control information[CCI] on gstreamer plugin dtcp encrypt element using property "cci".
 *
 * @param[in] cciInfo Copy Control Information
 *
 * @return None
 */
void HNSink::handleCCIChange(char cciInfo)
{
	static_cast<HNSinkPrivate*>(getPrivateSinkImpl())->handleCCIChange(cciInfo);
}

/**
 * @fn HNSink::unRegisterEventQ(rmf_osal_eventqueue_handle_t eventqueue)
 * @brief This API is used to unregister an event queue from event manager.
 * This shall be called if event queue do not want to receive any more events from the event manager.
 *
 * @param[in] eventqueue Handle of eventqueue
 * 
 * @return None
 * @}
 */
void HNSink::unRegisterEventQ(rmf_osal_eventqueue_handle_t eventqueue)
{
	static_cast<HNSinkPrivate*>(getPrivateSinkImpl())->unRegisterEventQ(eventqueue);
}

