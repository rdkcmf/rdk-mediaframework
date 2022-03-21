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

using namespace std;

#include "TunerReservationHelper.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include "rmf_osal_thread.h"
#include "rmf_osal_util.h"
#include "rdk_debug.h"
#include "rmf_osal_sync.h"
#include <map>
#include <string>
#include <string.h>
#include <uuid/uuid.h>
#include<string>
#include <list>
#include <vector>
#include <glib.h>
#include <unistd.h>
#include <stdio.h>
#if 1
#include "trm/Messages.h"
#include "trm/MessageProcessor.h"
#include "trm/Activity.h"
#include "trm/JsonEncoder.h"
#include "trm/JsonDecoder.h"
#endif
enum Type {
	REQUEST = 0x1234,
	RESPONSE = 0x1800,
	NOTIFICATION = 0x1400,
	UNKNOWN,
};

#define MAX_PAYLOAD_LEN 32767
#define DEFAULT_RESERVE_WINDOW 120000
//#define DEFAULT_TRH_URL "http://127.0.0.1:9987"
#define DEFAULT_TRH_URL "http://192.168.160.207:50050/trm"

#define MAX_RESPONSE_SIZE 2048
static const int64_t HOT_RECORDING_OFFSET_THRESHOLD = 66500;//66.5 seconds

static rmf_osal_Mutex g_mutex = 0;
static int trm_socket_fd = -1;

static const char* ip ="127.0.0.1";
static int  port = 9987;

static int is_connected = 0;
TRHcancelLiveCallback_t TunerReservationHelper::cancelLiveCallback = NULL;
static list<TunerReservationHelperImpl*> gTrhList;

std::string responseStr;

class TunerReservationHelperImpl
{
public:
	bool reserveTunerForRecord( string receiverId, string recordingId, const char* locator, 
				uint64_t startTime=0, uint64_t duration=0, bool synchronous = false );
	bool reserveTunerForLive( const char* locator, uint64_t startTime=0, 
				uint64_t duration=0 , bool synchronous = false);
	bool releaseTunerReservation();
	/*This function shall be called by the application once cancelRecording event is handled*/
	bool canceledRecording(string req_uuid);
	bool getAllTunerStates(std::string &output);
	static int getReserveWindowMS( );
	static void init();

	TunerReservationHelperImpl(TunerReservationEventListener* listener);
	~TunerReservationHelperImpl();
	const char* getId();
	const std::string& getToken();
	void setToken( const std::string& token);
	TunerReservationEventListener* getEventListener();
	void notifyResrvResponse(bool success);
	int notifyDisconnected();
	int notifyConnected();
private:
	TunerReservationHelperImpl* impl;
	static int reserveWindow;
	bool mRunning;
	TunerReservationEventListener* eventListener;
	static bool inited;
	char guid[64];
	std::string token;
	bool waitForResrvResponse();
	GCond* tunerStopCond;
	GMutex* tunerStopMutex;
	bool reservationSuccess;
	bool resrvResponseRecieved;
};


class RecorderMessageProcessor : public TRM::MessageProcessor 
{
public :
	RecorderMessageProcessor();
	void operator() (const TRM::ReserveTunerResponse &msg) ;
	void operator() (const TRM::CancelRecording &msg);
	void operator() (const TRM::NotifyTunerReservationRelease &msg);
	void operator() (const TRM::ReleaseTunerReservationResponse &msg);
	void operator() (const TRM::CancelLive &msg) ;
	void operator() (const TRM::GetAllTunerStatesResponse &msg);

//	void process(const ReleaseTunerReservationResponse &msg);
	TunerReservationHelperImpl* getTRH( const TRM::MessageBase &msg);
	TunerReservationHelperImpl* getTRH( const string &token);
};

static void notify_trm_connection_status_change(bool isConnected)
{
	list<TunerReservationHelperImpl*>::iterator i;
	rmf_osal_mutexAcquire( g_mutex);
	if(isConnected)
	{
		for(i=gTrhList.begin(); i != gTrhList.end(); ++i)
		{
			(*i)->notifyConnected();
		}
	}
	else
	{
		for(i=gTrhList.begin(); i != gTrhList.end(); ++i)
		{
			(*i)->notifyDisconnected();
		}
	}
	rmf_osal_mutexRelease( g_mutex);
	return;
}

static int connect_to_trm()
{
	int socket_fd ;
	int socket_error = 0;
	struct sockaddr_in trm_address;

	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TRH", "Enter %s():%d \n" , __FUNCTION__, __LINE__);
	memset(&trm_address, 0, sizeof(sockaddr_in));
	rmf_osal_mutexAcquire( g_mutex);

	if (is_connected == 0)
	{
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TRH", "%s:%d : Connecting to remote\n" , __FUNCTION__, __LINE__);
		
		trm_address.sin_family = AF_INET;
		trm_address.sin_addr.s_addr = inet_addr(ip);
		trm_address.sin_port = htons(port);
		if (trm_socket_fd == -1 )
		{
			socket_fd = socket(AF_INET, SOCK_STREAM, 0);
		}
		else
		{
			socket_fd = trm_socket_fd;
		}
		
		while(1)
		{
			int retry_count = 10;
			socket_error = connect(socket_fd, (struct sockaddr *) &trm_address, sizeof(struct sockaddr_in));
			if (socket_error == ECONNREFUSED  && retry_count > 0) 
			{
				RDK_LOG(RDK_LOG_WARN, "LOG.RDK.TRH", "%s:%d : TRM Server is not started...retry to connect\n" , __FUNCTION__, __LINE__);
				sleep(2);
				retry_count--;
			}
			else 
			{
			   break;
			}
		}

		if (socket_error == 0)
		{
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TRH", "%s:%d : Connected\n" , __FUNCTION__, __LINE__);

			int current_flags = fcntl(socket_fd, F_GETFL, 0);
			current_flags &= (~O_NONBLOCK);
			fcntl(socket_fd, F_SETFL, current_flags);
			trm_socket_fd = socket_fd;
			is_connected = 1;
			notify_trm_connection_status_change(TRUE);
		}
		else 
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.TRH", "%s:%d : socket_error %d, closing socket\n" , __FUNCTION__, __LINE__, socket_error);
			close(socket_fd);
			trm_socket_fd = -1;
		}
	}
	rmf_osal_mutexRelease( g_mutex);
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TRH", "%s:%d : returning %d\n",__FUNCTION__, __LINE__, socket_error);
	return socket_error;
}

static bool url_request_post( const char *payload, int payload_length)
{
	unsigned char *buf = NULL;
	bool ret = false;
//	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TRH", "POSTing data to URL at %s", url);
	connect_to_trm();
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TRH", "Connection status to TRM  %d\n", is_connected);
	
	if (is_connected ) 
	{
		if (payload_length != 0) 
		{
			/* First prepend header */
			static int message_id = 0x1000;
			const int header_length = 16;
			buf = (unsigned char *) malloc(payload_length + header_length);
			int idx = 0;
			/* Magic Word */
			buf[idx++] = 'T';
			buf[idx++] = 'R';
			buf[idx++] = 'M';
			buf[idx++] = 'S';
			/* Type, set to UNKNOWN, as it is not used right now*/
			buf[idx++] = (UNKNOWN & 0xFF000000) >> 24;
			buf[idx++] = (UNKNOWN & 0x00FF0000) >> 16;
			buf[idx++] = (UNKNOWN & 0x0000FF00) >> 8;
			buf[idx++] = (UNKNOWN & 0x000000FF) >> 0;
			/* Message id */
			++message_id;
            const unsigned int recorder_connection_id = 0XFFFFFF00;
			buf[idx++] = (recorder_connection_id & 0xFF000000) >> 24;
			buf[idx++] = (recorder_connection_id & 0x00FF0000) >> 16;
			buf[idx++] = (recorder_connection_id & 0x0000FF00) >> 8;
			buf[idx++] = (recorder_connection_id & 0x000000FF) >> 0;
			/* Payload length */
			buf[idx++] = (payload_length & 0xFF000000) >> 24;
			buf[idx++] = (payload_length & 0x00FF0000) >> 16;
			buf[idx++] = (payload_length & 0x0000FF00) >> 8;
			buf[idx++] = (payload_length & 0x000000FF) >> 0;

			for (int i =0; i< payload_length; i++)
				buf[idx+i] = payload[i];
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TRH", "========REQUEST MSG================================================\n[");
			for (idx = 0; idx < (header_length); idx++) {
				printf( "%02x", buf[idx]);
			}
			for (; idx < (payload_length + header_length); idx++) {
				printf("%c", buf[idx]);
			}

			/* Write payload from fastcgi to TRM */
			int write_trm_count = write(trm_socket_fd, buf, payload_length + header_length);
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TRH", "Send to TRM  %d vs expected %d\n", write_trm_count, payload_length + header_length);
			free(buf);
			buf = NULL;

			if (write_trm_count == 0) 
			{
				is_connected = 0;
				RDK_LOG(RDK_LOG_WARN, "LOG.RDK.TRH", "%s:%d : write_trm_count 0\n", __FUNCTION__, __LINE__);
				/* retry connect after write failure*/
				notify_trm_connection_status_change(FALSE);
			}
			else
			{
				ret = true;
			}
		}
	}
	return ret;
}

void processBuffer( const char* buf, int len)
{
	if (buf != NULL)
	{
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TRH","Response %s\n", buf);
		responseStr = buf;
		std::vector<uint8_t> response;
		response.insert( response.begin(), buf, buf+len);
		RecorderMessageProcessor recProc;
		TRM::JsonDecoder jdecoder( recProc);
		jdecoder.decode( response);
	}
}

static void TunerReservationHelperThread (void* arg)
{
	int read_trm_count = 0;
	char *buf = NULL;
	const int header_length = 16;
	int idx = 0;
	unsigned int payload_length = 0;
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TRH", "Enter %s():%d \n" , __FUNCTION__, __LINE__);
	while (1)
	{
		connect_to_trm();
		if ( is_connected )
		{
			buf = (char *) malloc(header_length);
			if (buf == NULL)
			{
				RDK_LOG(RDK_LOG_WARN, "LOG.RDK.TRH", "%s:%d :  Malloc failed for %d bytes \n", __FUNCTION__, __LINE__, header_length);
				continue;
			}
			memset(buf, 0, header_length);
			/* Read Response from TRM, read header first, then payload */
			read_trm_count = read(trm_socket_fd, buf, header_length);
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TRH", "Read Header from TRM %d vs expected %d\n", read_trm_count, header_length);
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TRH", "=====RESPONSE HEADER===================================================\n[");

			for (idx = 0; idx < (header_length); idx++) {
				printf( "%02x", buf[idx]);
			}
			printf("\n");

			if (read_trm_count == header_length) 
			{
				int payload_length_offset = 12;
				payload_length =((((unsigned char)(buf[payload_length_offset+0])) << 24) |
								 (((unsigned char)(buf[payload_length_offset+1])) << 16) |
								 (((unsigned char)(buf[payload_length_offset+2])) << 8 ) |
								 (((unsigned char)(buf[payload_length_offset+3])) << 0 ));

				if((payload_length > 0) && (payload_length < MAX_PAYLOAD_LEN))
				{
					free( buf);
						buf = NULL;
					RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TRH", "TRM Response payloads is %d and header %d\n", payload_length, header_length);

					buf = (char *) malloc(payload_length+1);
					memset(buf, 0, payload_length+1);
					read_trm_count = read(trm_socket_fd, buf, payload_length);
					RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TRH", "Read Payload from TRM %d vs expected %d\n", read_trm_count, payload_length);

					if (read_trm_count != 0) 
					{
						buf[payload_length] = '\0';
						processBuffer(buf, read_trm_count);
						free(buf);
						buf = NULL;
					}
					else 
					{
						/* retry connect after payload-read failure*/
						is_connected = 0;
						free(buf);
						buf = NULL;
						RDK_LOG(RDK_LOG_WARN, "LOG.RDK.TRH", "%s:%d : read_trm_count 0\n", __FUNCTION__, __LINE__);
						notify_trm_connection_status_change(FALSE);
					}
				}
				else
				{
					/* retry connect after payload-read failure*/
					is_connected = 0;
					free(buf);
					buf = NULL;
					RDK_LOG(RDK_LOG_WARN, "LOG.RDK.TRH", "%s:%d : read_trm_count 0\n", __FUNCTION__, __LINE__);
					notify_trm_connection_status_change(FALSE);

				}
			}
			else 
			{
				RDK_LOG(RDK_LOG_WARN, "LOG.RDK.TRH", "%s:%d : read_trm_count %d\n", __FUNCTION__, __LINE__, read_trm_count);
				free(buf);
				buf = NULL;
				/* retry connect after header-read failure */
				is_connected = 0;
				notify_trm_connection_status_change(FALSE);
			}

		}
		else
		{
			RDK_LOG( RDK_LOG_WARN, "LOG.RDK.TRH", "%s - Not connected- Sleep and retry\n", __FUNCTION__);
			sleep(1);
		}
	}
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TRH", "Exit %s():%d \n" , __FUNCTION__, __LINE__);
}

int  TunerReservationHelperImpl::reserveWindow = 0;
bool  TunerReservationHelperImpl::inited = false;

void TunerReservationHelperImpl::init()
{
	rmf_osal_ThreadId threadId;
	const char* env;
	rmf_Error ret;
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TRH", "Enter %s():%d \n" , __FUNCTION__, __LINE__);

	if ( false == inited )
	{
		inited = true;
		ret = rmf_osal_mutexNew( &g_mutex);
		if (RMF_SUCCESS != ret )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.TRH", "%s - rmf_osal_mutexNew failed.\n", __FUNCTION__);
			return;
		}

		env  = rmf_osal_envGet("FEATURE.TRH.IP");
		if ( NULL == env )
		{
			RDK_LOG(RDK_LOG_WARN, "LOG.RDK.TRH", "%s():%d FEATURE.TRH.IP Not present in configfile, using default %s\n" , 
				__FUNCTION__, __LINE__, ip);
		}
		else
		{
			ip = env;
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TRH", "%s():%d IP from config file %s\n" , 
				__FUNCTION__, __LINE__, ip);
		}

		env = rmf_osal_envGet("FEATURE.TRH.PORT");
		if ( NULL != env )
		{
			port =  (unsigned)atoi(env);
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TRH", "%s():%d Port from config file %d\n" , 
				__FUNCTION__, __LINE__, port);
		}
		else
		{
			RDK_LOG(RDK_LOG_WARN, "LOG.RDK.TRH", "%s():%d FEATURE.TRH.PORT Not present in configfile, using default %d\n" , 
				__FUNCTION__, __LINE__, port);
		}

		env = rmf_osal_envGet("FEATURE.TRH.WINDOW");
		if ( NULL != env )
		{
			reserveWindow =  (unsigned)atoi(env);
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TRH", "%s():%d WINDOW from config file %d\n" , 
				__FUNCTION__, __LINE__, reserveWindow);
		}
		else
		{
			reserveWindow =  DEFAULT_RESERVE_WINDOW;
			RDK_LOG(RDK_LOG_WARN, "LOG.RDK.TRH", "%s():%d FEATURE.TRH.WINDOW Not present in configfile, using default %d\n" , 
				__FUNCTION__, __LINE__, reserveWindow);
		}
		
		setlinebuf(stderr);
		rmf_osal_threadCreate( TunerReservationHelperThread, NULL,
								RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE,
								&threadId,"TunerRes" );
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TRH", "Exit %s():%d \n" , __FUNCTION__, __LINE__);
	}
}

TunerReservationHelperImpl::TunerReservationHelperImpl(TunerReservationEventListener* listener) :
eventListener(listener)
{
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TRH", "Enter %s:%d this = %p\n",__FUNCTION__,__LINE__,this);
	init();
	
	tunerStopCond = g_cond_new();
	tunerStopMutex = g_mutex_new ();
	rmf_osal_mutexAcquire( g_mutex);
	gTrhList.push_back(this);
	rmf_osal_mutexRelease( g_mutex);
}

TunerReservationHelperImpl::~TunerReservationHelperImpl()
{
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TRH", "Enter %s:%d this = %p\n",__FUNCTION__,__LINE__,this);
	mRunning = false;

	/*signal tunerStopCond, as deleting a conditional variable which still have threads waiting
	 on it have undefined behaviour*/

	g_mutex_lock ( tunerStopMutex );
	resrvResponseRecieved = true;
	reservationSuccess = false;
	g_cond_signal(tunerStopCond);
	g_mutex_unlock ( tunerStopMutex );

	rmf_osal_mutexAcquire( g_mutex);
	gTrhList.remove(this);
	rmf_osal_mutexRelease( g_mutex);
	g_cond_free(tunerStopCond);
	g_mutex_free( tunerStopMutex);
}

#define TRM_RESPONSE_TIMEOUT (65*1000*1000) /* 65 Seconds used in 1.3 as well */

bool TunerReservationHelperImpl::waitForResrvResponse()
{
	GTimeVal timeVal;
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TRH", "Enter %s:%d this = %p\n",__FUNCTION__,__LINE__,this);
	g_mutex_lock ( tunerStopMutex );
	while (false == resrvResponseRecieved )
	{
		g_get_current_time (&timeVal);
		g_time_val_add (&timeVal, TRM_RESPONSE_TIMEOUT);
		if ( false == g_cond_timed_wait (tunerStopCond,
			tunerStopMutex, &timeVal) )
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.TRH", "TRM response timedout %s:%d this = %p\n",
				__FUNCTION__,__LINE__,this);
			break;
		}
		else
		{
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TRH", "TRM response received %s:%d this = %p\n",
				__FUNCTION__,__LINE__,this);
		}
	}
	g_mutex_unlock (tunerStopMutex);
	return reservationSuccess;
}

void TunerReservationHelperImpl::notifyResrvResponse(bool success)
{
		g_mutex_lock ( tunerStopMutex );
		resrvResponseRecieved = true;
		reservationSuccess = success;
		g_cond_signal ( tunerStopCond);
		g_mutex_unlock (tunerStopMutex);
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TRH", "Exit %s:%d this = %p\n",__FUNCTION__,__LINE__,this);
}

//startTime: start time of the reservation in milliseconds from the epoch.  
//duration: reservation period measured from the start in milliseconds. 
bool TunerReservationHelperImpl::reserveTunerForRecord( string receiverId, string recordingId, const char* locator, 
			uint64_t startTime, uint64_t duration, bool synchronous )
{
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TRH", "Enter %s():%d with receiverId[%s]\n" , __FUNCTION__, __LINE__, receiverId.c_str());
	bool ret = false;
	GTimeVal currentTime;
	int64_t now = 0;
	std::vector<uint8_t> out;
	uuid_t value;
	uuid_generate(value);
	uuid_unparse(value, guid);
	TRM::Activity activity(TRM::Activity::kRecord);
	//char recordingIdStr[10];
	//snprintf(recordingIdStr,10, "%d", recordingId);
	//activity.addDetail("recordingId", recordingIdStr);
	activity.addDetail("recordingId", recordingId);
	
	g_get_current_time (&currentTime);
	now = ((gint64) currentTime.tv_usec)/((gint64) 1000); //Convert usec to ms
	now+= (((gint64) currentTime.tv_sec) * ((gint64) 1000)); //Convert sec to ms

	if(HOT_RECORDING_OFFSET_THRESHOLD > ((int64_t)startTime - now))
	{
		activity.addDetail("hot", "true");
	}

        if(receiverId.empty()) {
            receiverId = "xg123";
        }
	
	TRM::TunerReservation resrv( receiverId, locator, startTime, duration, activity, token);
	TRM::ReserveTuner msg(guid, "recorder-2.0", resrv);
	JsonEncode(msg, out);
	out.push_back(0);
	int len = strlen((const char*)&out[0]);
	int retry_count = 10;
	resrvResponseRecieved = false;

	do
	{
		ret = url_request_post( (char *) &out[0], len);
		retry_count --;
	}
	while ((ret == false) && (retry_count >0));

	if ((ret == true) && ( synchronous == true ) )
	{
		ret = waitForResrvResponse();
	}

	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TRH", "Exit %s():%d \n" , __FUNCTION__, __LINE__);
	return ret;
}


//startTime: start time of the reservation in milliseconds from the epoch.  
//duration: reservation period measured from the start in milliseconds. 
bool TunerReservationHelperImpl::reserveTunerForLive( const char* locator, 
			uint64_t startTime, uint64_t duration, bool synchronous  )
{
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TRH", "Enter %s():%d \n" , __FUNCTION__, __LINE__);
	bool ret = false;
	std::vector<uint8_t> out;
	uuid_t value;
	uuid_generate(value);
	uuid_unparse(value, guid);
	TRM::Activity activity(TRM::Activity::kLive);

	TRM::TunerReservation resrv( "xg1", locator, startTime, duration, activity);
	TRM::ReserveTuner msg(guid, "xg1", resrv);
	JsonEncode(msg, out);
	out.push_back(0);
	int len = strlen((const char*)&out[0]);
	int retry_count = 10;
	resrvResponseRecieved = false;

	do
	{
		ret = url_request_post( (char *) &out[0], len);
		retry_count --;
	}while ((ret == false) && (retry_count >0));

	if ((ret == true) && ( synchronous == true ) )
	{
		ret = waitForResrvResponse();
	}

	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TRH", "Exit %s():%d \n" , __FUNCTION__, __LINE__);
	return ret;
}


//startTime: start time of the reservation in milliseconds from the epoch.  
//duration: reservation period measured from the start in milliseconds. 
bool TunerReservationHelperImpl::releaseTunerReservation( )
{
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TRH", "Enter %s():%d \n" , __FUNCTION__, __LINE__);
	bool ret = false;
	std::vector<uint8_t> out;
	uuid_t value;
	uuid_generate(value);
	uuid_unparse(value, guid);

	TRM::ReleaseTunerReservation msg( guid, "xg1", token);
	JsonEncode(msg, out);
	out.push_back(0);
	int len = strlen((const char*)&out[0]);
	int retry_count = 10;

	do
	{
		ret = url_request_post( (char *) &out[0], len);
		retry_count --;
	}while ((ret == false) && (retry_count >0));

	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TRH", "Exit %s():%d \n" , __FUNCTION__, __LINE__);
	return ret;
}


bool TunerReservationHelperImpl::canceledRecording(string req_uuid)
{
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TRH", "Enter %s():%d \n" , __FUNCTION__, __LINE__);
	bool ret = false;
	std::vector<uint8_t> out;
	uuid_t value;

	TRM::CancelRecordingResponse msg(req_uuid, TRM::ResponseStatus::kOk ,token, true);
	TRM::JsonEncode(msg, out);
	out.push_back(0);
	int len = strlen((const char*)&out[0]);
	int retry_count = 10;

	do
	{
		ret = url_request_post( (char *) &out[0], len);
		retry_count --;
	}while ((ret == false) && (retry_count >0));

	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TRH", "Exit %s():%d \n" , __FUNCTION__, __LINE__);
	return ret;
}

bool TunerReservationHelperImpl::getAllTunerStates(std::string &output)
{
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TRH", "Enter %s():%d \n" , __FUNCTION__, __LINE__);
	bool ret = false;
	std::vector<uint8_t> out;
	uuid_t value;
	uuid_generate(value);
	uuid_unparse(value, guid);

	TRM::GetAllTunerStates msg(guid, "");
	JsonEncode(msg, out);
	out.push_back(0);
	int len = strlen((const char*)&out[0]);
	int retry_count = 10;
	resrvResponseRecieved = false;
	reservationSuccess = false;
	output = "";

	do
	{
		ret = url_request_post( (char *) &out[0], len);
		retry_count --;
	} while ((ret == false) && (retry_count >0));

	if (ret == true)
	{
		ret = waitForResrvResponse();
	}

	if (ret == true)
	{
		output = responseStr;
	}

	return ret;
}

int TunerReservationHelperImpl::getReserveWindowMS( )
{
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TRH", "%s():%d \n" , __FUNCTION__, __LINE__);
	return reserveWindow;
}

TunerReservationEventListener* TunerReservationHelperImpl::getEventListener()
{
	return eventListener;
}

const char* TunerReservationHelperImpl::getId()
{
	return guid;
}

const string& TunerReservationHelperImpl::getToken()
{
	return token;
}

void TunerReservationHelperImpl::setToken( const string& token)
{
	this->token = token;
}

int TunerReservationHelperImpl::notifyConnected()
{
	RDK_LOG( RDK_LOG_DEBUG , "LOG.RDK.TRH", "%s. This: 0x%x\n", __FUNCTION__, this);
	if(eventListener)
	{
		eventListener->notifyConnected();
	}
	return 0;
}
int TunerReservationHelperImpl::notifyDisconnected()
{
	RDK_LOG( RDK_LOG_DEBUG , "LOG.RDK.TRH", "%s. This: 0x%x\n", __FUNCTION__, this);
	if(eventListener)
	{
		eventListener->notifyDisconnected();
	}
	return 0;
}



RecorderMessageProcessor::RecorderMessageProcessor() 
{
}

TunerReservationHelperImpl* RecorderMessageProcessor::getTRH( const TRM::MessageBase &msg)
{
	TunerReservationHelperImpl* trh = NULL;
	list<TunerReservationHelperImpl*>::iterator i;
	for(i=gTrhList.begin(); i != gTrhList.end(); ++i)
	{
		string id = msg.getUUID();
		trh = *i;
		string temp_id = trh->getId();
		if ( 0 == temp_id.compare(id))
		{
			break;
		}
		trh = NULL;
	}
	return trh;
}

TunerReservationHelperImpl* RecorderMessageProcessor::getTRH( const string &token)
{
	TunerReservationHelperImpl* trh = NULL;
	list<TunerReservationHelperImpl*>::iterator i;
	if(!token.empty())
	{
		for(i=gTrhList.begin(); i != gTrhList.end(); ++i)
		{
			trh = *i;
			string temp_token = trh->getToken();
			if ( 0 == temp_token.compare(token))
			{
				break;
			}
			trh = NULL;
		}
	}
	return trh;
}

void RecorderMessageProcessor::operator() (const TRM::ReserveTunerResponse &msg)
{
	TunerReservationHelperImpl* trh;
	TunerReservationEventListener* el;

	rmf_osal_mutexAcquire( g_mutex);
	trh =  getTRH(msg);
	if ( NULL == trh )
	{
		RDK_LOG( RDK_LOG_ERROR , "LOG.RDK.TRH", "%s -Matching TRH couldnot be found\n", __FUNCTION__);
	}
	else
	{
		bool success;
		TRM::TunerReservation resv = msg.getTunerReservation();
		trh->setToken( resv.getReservationToken());
		el = trh->getEventListener();
		TRM::ResponseStatus status  = msg.getStatus();
		if ( status == TRM::ResponseStatus::kOk )
		{
			RDK_LOG( RDK_LOG_INFO , "LOG.RDK.TRH", "%s -OK response detected\n", __FUNCTION__);
			if ( el )
			{
				el->reserveSuccess();
			}
			success = true;
		}
		else
		{
			int statusCode = status.getStatusCode();
			RDK_LOG( RDK_LOG_WARN , "LOG.RDK.TRH", "%s -Response not OK statusCode = %d\n", __FUNCTION__, statusCode);
			el->reserveFailed();
			success = false;
		}
		trh->notifyResrvResponse( success);
	}
	rmf_osal_mutexRelease( g_mutex);
}


void RecorderMessageProcessor::operator() (const TRM::CancelRecording &msg)
{
	TunerReservationHelperImpl* trh;
	TunerReservationEventListener* el;

	const string token = msg.getReservationToken();

	rmf_osal_mutexAcquire( g_mutex);
	trh =  getTRH(token);
	if ( NULL == trh )
	{
		RDK_LOG( RDK_LOG_ERROR , "LOG.RDK.TRH", "%s -Matching TRH couldnot be found\n", __FUNCTION__);
	}
	else
	{
		el = trh->getEventListener();
		if ( el )
		{
			trh->canceledRecording(msg.getUUID());
			el->cancelRecording();
		}
	}
	rmf_osal_mutexRelease( g_mutex);
}

void RecorderMessageProcessor::operator() (const TRM::CancelLive &msg)
{
	int ret;
	RDK_LOG( RDK_LOG_DEBUG , "LOG.RDK.TRH", "%s: processing cancel-live message for token %s, %s.\n",
		__FUNCTION__, msg.getReservationToken().c_str(), msg.getServiceLocator().c_str());
	TRM::Enum<TRM::ResponseStatus> statusCode = TRM::ResponseStatus::kOk;
	std::string statusString;
	ret = TunerReservationHelper::executeCancelLiveCallback(msg.getReservationToken(), msg.getServiceLocator());
	if(0 == ret)
	{
		statusString = "Cancelled live session.";
	}
	else
	{
		statusCode = TRM::ResponseStatus::kGeneralError;
		statusString = "Cancel-live request failed";
	}

	TRM::ResponseStatus status(statusCode, statusString);
	TRM::CancelLiveResponse rsp(msg.getUUID(), status,
		msg.getReservationToken(), msg.getServiceLocator(),
		(0 == ret));

	std::vector<uint8_t> out;
	TRM::JsonEncode(rsp, out);
	out.push_back( 0 );
        if ( url_request_post( (char *) &out[0], out.size()) != true )
        {
		RDK_LOG(RDK_LOG_WARN, "LOG.RDK.TRH", "%s:%d : url_request_post failed\n", __FUNCTION__, __LINE__);
        }
}



void RecorderMessageProcessor::operator() (const TRM::NotifyTunerReservationRelease &msg)
{
	TunerReservationHelperImpl* trh;
	TunerReservationEventListener* el;
	string token = msg.getReservationToken();

	rmf_osal_mutexAcquire( g_mutex);
	trh =  getTRH(token);
	if ( NULL == trh )
	{
		RDK_LOG( RDK_LOG_ERROR , "LOG.RDK.TRH", "%s::NotifyTunerReservationRelease -Matching TRH couldnot be found\n", __FUNCTION__);
	}
	else
	{
		string reason  = msg.getReason();

		RDK_LOG( RDK_LOG_INFO , "LOG.RDK.TRH", "NotifyTunerReservationRelease -reason:  %s\n",  reason.c_str());
		el = trh->getEventListener();
		if ( el )
		{
			el->tunerReleased();
		}
	}
	rmf_osal_mutexRelease( g_mutex);
}

void RecorderMessageProcessor::operator() (const TRM::ReleaseTunerReservationResponse &msg)
{
	TunerReservationHelperImpl* trh;
	TunerReservationEventListener* el;
	bool isReleased;
	string token = msg.getReservationToken();

	rmf_osal_mutexAcquire( g_mutex);
	trh =  getTRH(token);
	if ( NULL == trh )
	{
		RDK_LOG( RDK_LOG_ERROR , "LOG.RDK.TRH", "NotifyTunerReservationRelease -Matching TRH couldnot be found\n", __FUNCTION__);
	}
	else
	{
		isReleased = msg.isReleased();
		el = trh->getEventListener();
		if ( true == isReleased )
		{
			RDK_LOG( RDK_LOG_INFO , "LOG.RDK.TRH", "ReleaseTunerReservationResponse -Tuner released\n", __FUNCTION__);
			if(el)
				el->releaseReservationSuccess();
		}
		else
		{
			RDK_LOG( RDK_LOG_WARN, "LOG.RDK.TRH", "ReleaseTunerReservationResponse -Tuner release failed\n", __FUNCTION__);
			if(el)
				el->releaseReservationFailed();
		}
	}
	rmf_osal_mutexRelease( g_mutex);
}

void RecorderMessageProcessor::operator() (const TRM::GetAllTunerStatesResponse &msg)
{
	TunerReservationHelperImpl* trh;

	rmf_osal_mutexAcquire( g_mutex);
	trh =  getTRH(msg);
	if ( NULL == trh )
	{
		RDK_LOG( RDK_LOG_ERROR , "LOG.RDK.TRH", "%s -Matching TRH couldnot be found\n", __FUNCTION__);
	}
	else
	{
		int statusCode = msg.getStatus().getStatusCode();

		if (TRM::ResponseStatus::kOk == statusCode)
		{
			RDK_LOG( RDK_LOG_INFO , "LOG.RDK.TRH", "%s -OK response detected\n", __FUNCTION__);
			trh->notifyResrvResponse(true);
		}
		else
		{
			RDK_LOG( RDK_LOG_INFO , "LOG.RDK.TRH", "%s -Response not OK statuscode = %d\n", __FUNCTION__, statusCode);
			trh->notifyResrvResponse(false);
		}
	}
	rmf_osal_mutexRelease( g_mutex);
}

void TunerReservationHelper::init()
{
	TunerReservationHelperImpl::init();
}

TunerReservationHelper::TunerReservationHelper(TunerReservationEventListener* listener) 
{
	impl = new TunerReservationHelperImpl(listener);
}

TunerReservationHelper::~TunerReservationHelper()
{
	delete impl;
}

//startTime: start time of the reservation in milliseconds from the epoch.  
//duration: reservation period measured from the start in milliseconds. 
bool TunerReservationHelper::reserveTunerForRecord( string receiverId, string recordingId, const char* locator, 
			uint64_t startTime, uint64_t duration, bool synchronous )
{
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TRH", "Enter %s():%d with receiverId[%s]\n" , __FUNCTION__, __LINE__, receiverId.c_str());
	return impl->reserveTunerForRecord( receiverId, recordingId.c_str(), locator, 
			startTime, duration, synchronous );
}


//startTime: start time of the reservation in milliseconds from the epoch.  
//duration: reservation period measured from the start in milliseconds. 
bool TunerReservationHelper::reserveTunerForLive( const char* locator, 
			uint64_t startTime, uint64_t duration, bool synchronous  )
{
	return impl->reserveTunerForLive( locator, startTime, duration );
}


//startTime: start time of the reservation in milliseconds from the epoch.  
//duration: reservation period measured from the start in milliseconds. 
bool TunerReservationHelper::releaseTunerReservation( )
{
	return impl->releaseTunerReservation();
}


bool TunerReservationHelper::canceledRecording()
{
//	return impl->canceledRecording();
	return true;
}

bool TunerReservationHelper::getAllTunerStates(std::string &output)
{
	return impl->getAllTunerStates(output);
}

int TunerReservationHelper::getReserveWindowMS( )
{
	return TunerReservationHelperImpl::getReserveWindowMS();
}

int TunerReservationHelper::registerCancelLiveCallback(TRHcancelLiveCallback_t callback)
{
	cancelLiveCallback = callback;
	return 0;
}

int TunerReservationHelper::executeCancelLiveCallback(const std::string reservation_token, const std::string service_locator)
{
	if(NULL == TunerReservationHelper::cancelLiveCallback)
	{				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.TRH", "%s: No callback registered.\n", __FUNCTION__);
		return -1;
	}
	return TunerReservationHelper::cancelLiveCallback(reservation_token, service_locator);
}
