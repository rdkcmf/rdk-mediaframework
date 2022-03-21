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
 * @file hnsink.h
 */
/**
 * @defgroup HNSink HN Sink
 * <b>HN Sink is responsible for the following operations. </b>
 * <ul>
 * <li> HN Sink element gets data from Source elements and streams on network through HTTP.
 * <li> On request HN Sink can encrypt depending on DRM method used.
 * <li> In the case of a recorded stream, the HN Sink requests the DVR source to AES decrypt the stream for clear SPTS and
 * the stream will be DTCP encrypted before streaming out.
 * <li> DTCP encrypt is G-Streamer filter element which encrypts incoming data and pushes encrypted data to source pad.
 * </ul>
 * <p>
 * HN sink uses to gstreamer plugins "httpsink" and "dtcpenc". These two plugins are added to the bin element and
 * called as HN Sink element. This element uses TCP socket to transfer the data to other device in the network.
 * <p>
 * <b> Following "httpsink" plugins properties are used in HN Sink </b>
 * <ul>
 * <li> <b> http-obj : </b> Set this properties for socket id for communication
 * <li> <b> source-type : </b> Set this properties for selecting the source type such as QAM, VOD, IPPV, etc
 * <li> <b> source-id : </b> Set this properties for source id such as ocap://0xXXXX, dvr://local/xxxx#0, vod://<string>, etc.
 * <li> <b> sent_data_size : </b> Use this properties to get the information about how much data is sent to the remote devicel
 * </ul>
 *
 * <p>
 * <b> Following "dtcpenc" plugins properties are used in HN Sink </b>
 * <ul>
 * <li> <b> "cci" : </b> This is used to set the CCI (Copy Control Information) value such as
 * 0x00=Copy Freely, 0x01=Copy No More, 0x02=Copy Once or 0x03=Copy Never.
 * <li> <b> "remoteip" : </b> is used to set the IP address of remote host for which DTCP encrypted data to be sent.
 * <li> <b> "keylabel" : </b> is used to set property for the DTCP key level
 * </ul>
 * @ingroup RMF
 *
 * @defgroup hnsinkapi HN Sink API list
 * Describe the details about HN Sink API specifications.
 * @ingroup HNSink
 */

#ifndef __HN_SINK_H__
#define __HN_SINK_H__

#include "rmfbase.h"
#include <string>
#include "rmf_osal_event.h"

#define RMF_HNSINK_EVENT_CONNECTION_CLOSED (RMF_HNSINK_EVENT_BASE +1)
#define RMF_HNSINK_EVENT_DTCP_AKE_FAILED (RMF_HNSINK_EVENT_BASE + 16)

/**
 * @enum typedef enum
 * @brief It represents the valid values for HN event types such as HN bufer available and HN session terminated.
 */
typedef enum {
            HN_BUFFER_AVAILABLE = 0,
            HN_SESSION_TERMINATED
} hn_event_type;

namespace hnSinkError
{
 const int httpsink_event_connection_closed = 0x601;
 const int dtcpenc_event_ake_failed = 0x611;
};

/**
 * @struct HNSinkProperties
 * @brief This structure defines the parameter for HN Sink properties.
 */
typedef struct HNSinkProperties {
	// dtcp enable or disable
	int dtcp_enabled;
	int dtcp_cci; // get cci value from qam/dvr source and set
	long remote_ip; 

	//socket
	int socketId;
	
	// chuncked
	int use_chunked_transfer; //httpsink

	char url[1024];

	//Source type connected to this sink
	char source_type[32];

	char source_id[1024];

	unsigned int dtcp_key_label;

	// More properties : add here.
} HNSinkProperties_t;

/**
 * @defgroup hnsinkclass HN Sink Classes
 * @ingroup HNSink
 * @class HNsink
 * @brief This class contains public member functions for HN Sink.
 * @ingroup hnsinkclass
 */
class HNSink: public RMFMediaSinkBase
{
public:
    
    HNSinkProperties_t m_hnsink_prop;

    static RMFResult init_platform();
    static RMFResult uninit_platform();

    // from RMFMediaSinkBase
    void* createElement();
    /*Register data / state events from the HN Sink.*/
    void registerEventQ(rmf_osal_eventqueue_handle_t eventqueue);
    void unRegisterEventQ(rmf_osal_eventqueue_handle_t eventqueue);
    void setSocket(int socketId);
    void setRemoteIp(long remote_ip);
    void setUrl(char *url);
    char* getUrl();
    unsigned long getBitrate();
    void setSourceType(char* source_type);
    void setSourceId(char* source_id);
    void handleCCIChange(char cciInfo);
    void setHNSinkProperties(HNSinkProperties_t properties);
    unsigned long getMBStreamed();
protected:
    // from RMFMediaSinkBase
    /*virtual*/ RMFMediaSinkPrivate* createPrivateSinkImpl();
};

#endif // __HN_SINK_H__
