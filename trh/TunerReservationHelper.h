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
#ifndef TRH_H_
#define TRH_H_

#include <stdint.h>
#include<string>

typedef int (*TRHcancelLiveCallback_t)(const std::string, const std::string);
class TunerReservationHelperImpl;


/*Application shall extend required methods to get events from TRH*/
class TunerReservationEventListener
{
public:
	virtual void reserveSuccess() {};
	virtual void reserveFailed() {};
	virtual void releaseReservationSuccess() {};
	virtual void releaseReservationFailed() {};
	virtual void tunerReleased() {};
	virtual void cancelRecording() {};
	virtual void notifyConnected() {}
	virtual void notifyDisconnected() {}
	virtual ~TunerReservationEventListener(){};
};

class TunerReservationHelper
{
public:
	bool reserveTunerForRecord(std::string receiverId, std::string recordingId, const char* locator, 
				uint64_t startTime=0, uint64_t duration=0, bool synchronous = false );
	bool reserveTunerForLive( const char* locator, uint64_t startTime=0, 
				uint64_t duration=0 , bool synchronous = false);
	bool releaseTunerReservation();
	/*This function shall be called by the application once cancelRecording event is handled*/
	bool canceledRecording();
	bool getAllTunerStates(std::string &output);
	static int getReserveWindowMS( );
	static void init();
	static int registerCancelLiveCallback(TRHcancelLiveCallback_t callback);
	static int executeCancelLiveCallback(const std::string reservation_token, const std::string service_locator);

	TunerReservationHelper(TunerReservationEventListener* listener);
	~TunerReservationHelper();
private:
	TunerReservationHelperImpl* impl;
	static TRHcancelLiveCallback_t cancelLiveCallback;
};

#endif

