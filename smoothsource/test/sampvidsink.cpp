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
#include "sampvidsink.h"
#include <gst/gst.h>


// virtual
void* SampVidSink::createElement()
{
    g_print("SampVidSink::createSinkElement enter\n");
    GstElement *pBin, *pCaps, *pDecoder, *pSink;
    pBin = gst_bin_new("video_sink_bin");
    pCaps = gst_element_factory_make("capsfilter", "h264-caps_filter");
    pDecoder = gst_element_factory_make("ismd_h264_viddec", "h264-decoder");
    pSink = gst_element_factory_make("ismd_vidsink", "videosink");
    g_object_set(G_OBJECT(pCaps), "caps",
        gst_caps_new_simple("video/x-h264",
        "variant", G_TYPE_STRING, "itu",
        "framerate", GST_TYPE_FRACTION, 30, 1,
        "width", G_TYPE_INT, 1280,
        "height", G_TYPE_INT, 720,
        NULL), NULL);
    g_object_set (G_OBJECT(pSink), "gdl-plane", 7, NULL);
    gst_bin_add_many(
	    GST_BIN(pBin),
	    pCaps,
        pDecoder,
        pSink,
        NULL);
    gst_element_link_many(
        pCaps,
        pDecoder,
        pSink, 
	    NULL);

    // Add a ghostpad to the bin so it can proxy to the demuxer.
    GstPad* pad = gst_element_get_static_pad(pCaps, "sink");
    gst_element_add_pad(pBin, gst_ghost_pad_new("sink", pad));
    gst_object_unref(GST_OBJECT(pad));

    g_print("SampVidSink::createSinkElement exit\n");
    return pBin;
}


