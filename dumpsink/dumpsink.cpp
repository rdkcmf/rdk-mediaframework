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

#include "dumpsink.h"

#include <cstdio>
#include <glib.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <unistd.h>

static GstFlowReturn on_new_buffer(GstAppSink *sink, gpointer data)
{
	DumpSink* instance = (DumpSink*) data;
	GstBuffer* buffer = gst_app_sink_pull_preroll(sink);
	instance->handleBuffer(buffer);
	return GST_FLOW_OK;
}

DumpSink::DumpSink(char c)
:	m_char(c)
,   m_delay_ms(10000)
,   m_interval(1)
,   m_cnt(0)
{
}

void DumpSink::setChar(char c)
{
	m_char = c;
}

// virtual
void* DumpSink::createElement()
{
    GstElement* appsink = gst_element_factory_make("appsink", "dumpsink");
    if (!appsink)
    {
    	g_print("Failed to instantiate appsink\n");
    	return 0;
    }

	GstAppSinkCallbacks callbacks = { NULL, NULL, on_new_buffer, NULL, { NULL } };
	gst_app_sink_set_callbacks((GstAppSink*) appsink, &callbacks, this, NULL);
    g_object_set(G_OBJECT(appsink), "max-buffers", 1, NULL);
    g_object_set(G_OBJECT(appsink), "drop", TRUE, NULL);

	return appsink;
}

void DumpSink::handleBuffer(void* buf)
{
    if (m_interval == 1 || m_cnt++ % m_interval == 0) // print every m_interval's frame
        g_print("%c", buf ? m_char : 'X');

    if (m_delay_ms > 0)
        usleep(m_delay_ms);
}
