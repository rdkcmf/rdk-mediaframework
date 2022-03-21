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
#include <unistd.h>
#include "TunerReservationHelper.h"
#include "rdk_debug.h"
#include "rmf_osal_init.h"
#include <glib.h>


class TRHListenerImpl : public TunerReservationEventListener
{
public:
	void reserveSuccess();
	void reserveFailed();
	void tunerReleased();
	void cancelRecording();
	void releaseReservationSuccess();
	void releaseReservationFailed();
};

TunerReservationHelper *trhRecord = NULL;
TunerReservationHelper *trhLive = NULL;
TRHListenerImpl tRHListenerRecImpl, tRHListenerLiveImpl;

void TRHListenerImpl::reserveSuccess()
{
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TEST", "Enter %s:%d this = %p\n",__FUNCTION__,__LINE__,this);
	if (this == &tRHListenerLiveImpl)
	{
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TEST", "%s:%d this = %p Cancelling live reservation\n",__FUNCTION__,__LINE__,this);
		trhLive->releaseTunerReservation();
	}
}

void TRHListenerImpl::reserveFailed()
{
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TEST", "Enter %s:%d this = %p\n",__FUNCTION__,__LINE__,this);

}

void TRHListenerImpl::tunerReleased()
{
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TEST", "Enter %s:%d this = %p\n",__FUNCTION__,__LINE__,this);
}

void TRHListenerImpl::cancelRecording()
{
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TEST", "Enter %s:%d this = %p\n",__FUNCTION__,__LINE__,this);
}

void TRHListenerImpl::releaseReservationSuccess()
{
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TEST", "Enter %s:%d this = %p\n",__FUNCTION__,__LINE__,this);
}

void TRHListenerImpl::releaseReservationFailed()
{
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TEST", "Enter %s:%d this = %p\n",__FUNCTION__,__LINE__,this);
}


int main()
{
	GTimeVal currentTime;
	uint64_t startTime, duration;
	int recordingId = 15;
	g_thread_init(NULL);
	rmf_osal_init( NULL, NULL );
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TEST", "Requesting TunerReservation");
	trhRecord = new TunerReservationHelper(&tRHListenerRecImpl);
	g_get_current_time (&currentTime);
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TEST", "Current time is %llu \n", currentTime.tv_sec);
	startTime = (gint64)currentTime.tv_sec*(gint64)1000+(gint64)2000;
	duration = (gint64)15*1000;
	if (!trhRecord->reserveTunerForRecord(recordingId, "ocap://0xA0",  startTime, duration, true))
	{
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TEST", "Recording %d tuner reservation failed.", recordingId);
		return -1;
	}
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TEST", "reserveTunerForRecord Success");

	sleep(5);
	return(0);
	trhLive = new TunerReservationHelper(&tRHListenerLiveImpl);
	g_get_current_time (&currentTime);
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TEST", "Current time is %llu \n", currentTime.tv_sec);
	startTime = (gint64)currentTime.tv_sec*(gint64)1000+(gint64)2000;
	duration = (gint64)15*1000;
	if (!trhLive->reserveTunerForLive("ocap://0xA0",  startTime, duration, true))
	{
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TEST", "Live tuner reservation failed.", recordingId);
		return -1;
	}
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TEST", "reserveTunerForLive Success");
	while (1)
		sleep(1);
}

