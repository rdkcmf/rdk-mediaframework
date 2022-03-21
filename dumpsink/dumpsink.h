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

#ifndef DUMP_SINK_H
#define DUMP_SINK_H

#include "rmfbase.h"
#include <string>

/**
 * Prints a character whenever a buffer arrives.
 */
class DumpSink: public RMFMediaSinkBase
{
public:
	DumpSink(char c = '.');

    void setChar(char c);
    void setDelay(unsigned delay_ms) { m_delay_ms = delay_ms; }
    void setInterval(unsigned interval) { m_interval = interval; }

    // from RMFMediaSinkBase
    /*virtual*/ void* createElement();

    void handleBuffer(void* buf); // not really private, used internally

private:
	char m_char;
	unsigned m_delay_ms;
	unsigned m_interval;
	unsigned m_cnt;
};

#endif // DUMP_SINK_H
