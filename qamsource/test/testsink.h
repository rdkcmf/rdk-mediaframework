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

#ifndef __TEST_SINK_H__
#define __TEST_SINK_H__

#include "rmfbase.h"
#include <string>
#include "rmf_osal_event.h"

typedef enum {
            TEST_BUFFER_AVAILABLE = 0,
            TEST_SESSION_TERMINATED
} test_event_type;

/**
 *  TEST sink.
 */
class TestSink: public RMFMediaSinkBase
{
public:

    // from RMFMediaSinkBase
    void* createElement();
    /*Register data / state events from the TEST Sink.*/
    void registerEventQ(rmf_osal_eventqueue_handle_t eventqueue);
    void unRegisterEventQ(rmf_osal_eventqueue_handle_t eventqueue);

protected:
    // from RMFMediaSinkBase
    /*virtual*/ RMFMediaSinkPrivate* createPrivateSinkImpl();
};

#endif // __TEST_SINK_H__
