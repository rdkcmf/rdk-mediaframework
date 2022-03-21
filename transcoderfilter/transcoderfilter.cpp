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
#include "transcoderfilter.h"
#include "rmfprivate.h"
#include <cmath>
#include <cassert>
#include <string.h>

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

#define FILTER_BIN getSource()
#define FILTER_IMPL static_cast<TranscoderFilterPrivate*>(getPrivateSourceImpl())

#define RETURN_UNLESS(x) RMF_RETURN_UNLESS(x, RMF_RESULT_INTERNAL_ERROR)

typedef struct
{
	const gchar *padname;
	GstPad *target;
} dyn_link;


class TranscoderFilterPrivate : public RMFMediaFilterPrivate
{
public:
    TranscoderFilterPrivate(TranscoderFilter* parent);
    ~TranscoderFilterPrivate();
    void* createElement();

    RMFResult init();
    RMFResult term();
	void setProperties(TranscoderProperties *prop);

private:
#ifndef XG5_GW
	GstElement* m_pencsource;
	GstElement* m_pmuxer;
	GstElement* m_pdemuxer;
	GstElement* m_pdecoder;
	GstElement* m_padecoder;
	GstElement* m_pprocessor;
	GstElement* m_pddqueue;
	GstElement* m_pdpqueue;
	GstElement* m_ppequeue;
	GstElement* m_pemqueue;
	GstElement* m_pdmqueue;
#else
	GstElement* m_ToTranscoderqueue;
	GstElement* m_FromTranscodermqueue;
	GstElement* m_transcoder;
#endif //XG5_GW
	void setup_dynamic_link (GstElement * element, const gchar * padname, GstPad * target, GstElement * bin);
};

TranscoderFilterPrivate::TranscoderFilterPrivate(TranscoderFilter* parent)
:   RMFMediaFilterPrivate(parent)
{
#ifndef XG5_GW
	m_pencsource = NULL;
	m_pmuxer = NULL;
	m_pdemuxer = NULL;
	m_pdecoder = NULL;
	m_padecoder = NULL;
	m_pprocessor = NULL;
	m_pddqueue = NULL;
	m_pdpqueue = NULL;
	m_ppequeue = NULL;
	m_pemqueue = NULL;
	m_pdmqueue = NULL;
#else
	m_ToTranscoderqueue = NULL;
	m_FromTranscodermqueue = NULL;
	m_transcoder = NULL;
#endif //XG5_GW
}

TranscoderFilterPrivate::~TranscoderFilterPrivate()
{
}

RMFResult TranscoderFilterPrivate::init()
{
   RMFResult result;

   result= RMFMediaFilterPrivate::init();

   return result;
}

void* TranscoderFilterPrivate::createElement()
{
    // Create an empty bin
    GstElement *bin= gst_bin_new("transcoder_filter_bin");
    assert(bin);

	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.HN", "creating transcode elements\n");
#ifndef XG5_GW
	m_pencsource   = gst_element_factory_make ("ismd_h264_videnc",       "encodersrc");
	assert( m_pencsource );
	m_pddqueue   = gst_element_factory_make ("queue",       "ddQueue");
	assert( m_pddqueue );
	m_pdpqueue   = gst_element_factory_make ("queue",       "dpQueue");
	assert( m_pdpqueue );
	m_ppequeue   = gst_element_factory_make ("queue",       "peQueue");
	assert( m_ppequeue );
	m_pemqueue   = gst_element_factory_make ("queue",       "emQueue");
	assert( m_pemqueue );
	m_pdmqueue   = gst_element_factory_make ("queue",       "dmQueue");
	assert( m_pdmqueue );
	m_pmuxer = gst_element_factory_make("ismd_ts_muxer", "ts-muxer");
	assert( m_pmuxer );
	m_pdemuxer = gst_element_factory_make ("flutsdemux",       "ts-demuxer");
	assert( m_pdemuxer );
	m_pdecoder = gst_element_factory_make ("ismd_mpeg2_viddec",      "video-decoder");
	assert( m_pdecoder );
	m_pprocessor = gst_element_factory_make ("ismd_vidpproc",      "video-processor");
	assert( m_pprocessor );
	m_padecoder = gst_element_factory_make ("ismd_audio_decoder",      "audio-decoder");
	assert( m_padecoder );

	//Element name properties
	g_object_set (G_OBJECT (m_pencsource), "name", "encoder", NULL);
	g_object_set (G_OBJECT (m_pmuxer), "name", "mux", NULL);
	//g_object_set (G_OBJECT (m_pdemuxer), "name", "demux", "pcr-difference", 10000, NULL);

	gst_bin_add_many(GST_BIN(bin), m_pencsource, m_pddqueue, m_pdpqueue, m_ppequeue, m_pemqueue, m_pdmqueue, m_pmuxer, m_pdemuxer, m_pdecoder,m_padecoder, m_pprocessor, NULL);

	gst_pad_link (gst_element_get_static_pad (m_pddqueue, "src"), gst_element_get_static_pad(m_pdecoder, "sink"));
	gst_pad_link (gst_element_get_static_pad (m_pdecoder, "src_1"), gst_element_get_static_pad(m_pdpqueue, "sink"));
	gst_pad_link (gst_element_get_static_pad (m_pdpqueue, "src"), gst_element_get_static_pad(m_pprocessor, "sink"));
	gst_pad_link (gst_element_get_static_pad (m_pprocessor, "src_1"), gst_element_get_static_pad(m_ppequeue, "sink"));
	gst_pad_link (gst_element_get_static_pad (m_ppequeue, "src"), gst_element_get_static_pad(m_pencsource, "sink_1"));
	gst_pad_link (gst_element_get_static_pad (m_pencsource, "src_1"), gst_element_get_static_pad(m_pemqueue, "sink"));
	gst_pad_link (gst_element_get_static_pad (m_pdmqueue, "src"), gst_element_get_static_pad(m_padecoder, "sink"));

	// video
	GstPadTemplate *muxer_sink_Vpad_template = gst_element_class_get_pad_template (GST_ELEMENT_GET_CLASS (m_pmuxer), "video_%d");
	GstPad *muxer_video_pad = gst_element_request_pad (m_pmuxer, muxer_sink_Vpad_template, NULL, NULL);
	gst_pad_link(gst_element_get_static_pad (m_pemqueue, "src"), muxer_video_pad /*gst_element_get_request_pad(muxer, "video_%d")*/);
	
	// Audio
	GstPadTemplate *muxer_sink_Apad_template = gst_element_class_get_pad_template (GST_ELEMENT_GET_CLASS (m_pmuxer), "audio_%d");
	GstPad *muxer_audio_pad = gst_element_request_pad (m_pmuxer, muxer_sink_Apad_template, NULL, NULL);
	gst_pad_link(gst_element_get_static_pad (m_padecoder, "src_1"), muxer_audio_pad/*gst_element_get_request_pad(muxer, "audio_00")*/);
	
	setup_dynamic_link (m_pdemuxer, "video"/*video_%04x*/, gst_element_get_static_pad (m_pddqueue, "sink"), bin);
	setup_dynamic_link (m_pdemuxer, "audio"/*audio_%04x*/, gst_element_get_static_pad (m_pdmqueue, "sink"), bin);


    // Add a src ghostpad to the bin.
    GstPad* pad = gst_element_get_static_pad(m_pmuxer, "src_1"); 
    gst_element_add_pad(bin, gst_ghost_pad_new("src", pad));
    gst_object_unref(GST_OBJECT(pad));

    // Add a sink ghostpad to the bin.
    pad = gst_element_get_static_pad(m_pdemuxer, "sink");
    gst_element_add_pad(bin, gst_ghost_pad_new("sink", pad));
    gst_object_unref(GST_OBJECT(pad));
#else
	m_ToTranscoderqueue   = gst_element_factory_make ("queue",       "m_ToTranscoderqueue");
	assert( m_ToTranscoderqueue );
	m_FromTranscodermqueue   = gst_element_factory_make ("queue",       "m_FromTranscodermqueue");
	assert( m_FromTranscodermqueue );
	m_transcoder   = gst_element_factory_make ("arris_xcode",       "m_transcoder");
	assert( m_transcoder );

	g_print("RMF_Filter : adding gst elements in Bin\n");
	gst_bin_add_many(GST_BIN(bin), m_ToTranscoderqueue, m_transcoder, m_FromTranscodermqueue, NULL);

	gst_element_link (m_ToTranscoderqueue, m_transcoder);
	gst_element_link (m_transcoder, m_FromTranscodermqueue);

	g_print("RMF_Filter : creating Ghost pad\n");
    // Add a src ghostpad to the bin.
    GstPad* pad = gst_element_get_static_pad(m_FromTranscodermqueue, "src"); 
    gst_element_add_pad(bin, gst_ghost_pad_new("src", pad));
    gst_object_unref(GST_OBJECT(pad));

    // Add a sink ghostpad to the bin.
    pad = gst_element_get_static_pad(m_ToTranscoderqueue, "sink");
    gst_element_add_pad(bin, gst_ghost_pad_new("sink", pad));
    gst_object_unref(GST_OBJECT(pad));
#endif //XG5_GW

    return bin;
}

void TranscoderFilterPrivate::setProperties(TranscoderProperties *prop)
{
#ifndef XG5_GW
	//Setting Properties
	g_object_set (G_OBJECT (m_pencsource), "max-bitrate", prop->max_bitrate, NULL);
	g_object_set (G_OBJECT (m_pencsource), "target-bitrate", prop->target_bitrate, NULL);
	g_object_set (G_OBJECT (m_padecoder), "audio-output-mode", 5, NULL);

	switch (prop->target_device)
	{
		//update resolution for devices
		case TRANSCODER_TARGET_IPHONE_16_9:
			g_object_set (G_OBJECT (m_pprocessor), "rectangle", "0,0,640,480", NULL);
			break;
		case TRANSCODER_TARGET_IPHONE_4_3:
			g_object_set (G_OBJECT (m_pprocessor), "rectangle", "0,0,640,480", NULL);
			break;
		case TRANSCODER_TARGET_IPAD_16_9:
			g_object_set (G_OBJECT (m_pprocessor), "rectangle", "0,0,420,380", NULL);
			break;
		case TRANSCODER_TARGET_IPAD_4_3:
			g_object_set (G_OBJECT (m_pprocessor), "rectangle", "0,0,420,380", NULL);
			break;
		case TRANSCODER_TARGET_1080p:
			g_object_set (G_OBJECT (m_pprocessor), "rectangle", "0,0,640,480", NULL);
			break;
		case TRANSCODER_TARGET_720p:
			g_object_set (G_OBJECT (m_pprocessor), "rectangle", "0,0,640,480", NULL);
			break;
		case TRANSCODER_TARGET_480p:
			g_object_set (G_OBJECT (m_pprocessor), "rectangle", "0,0,640,480", NULL);
			break;
		case TRANSCODER_TARGET_1080i:
			g_object_set (G_OBJECT (m_pprocessor), "rectangle", "0,0,640,480", NULL);
			break;
		case TRANSCODER_TARGET_480i:
			g_object_set (G_OBJECT (m_pprocessor), "rectangle", "0,0,640,480", NULL);
			break;
		default:
			break;
	}
	//g_object_set (G_OBJECT (m_pAAQueue), "max-size-buffers", DEFAULT_GST_QUEUE_MAX_SIZE_BUF, NULL);

	g_object_set (G_OBJECT (m_pddqueue), "max-size-buffers", 3, NULL); // demux to decode Q
	g_object_set (G_OBJECT (m_pdpqueue), "max-size-buffers", 3, NULL); // decode to vidpproc Q
	g_object_set (G_OBJECT (m_ppequeue), "max-size-buffers", 3, NULL); // vidpproc to encode Q
	g_object_set (G_OBJECT (m_pemqueue), "max-size-buffers", 3, NULL); // encode to mux Q
	g_object_set (G_OBJECT (m_pdmqueue), "max-size-buffers", 3, NULL); // demux to mux Q
#else
	g_print("In %s : setting arris properties\n", __FUNCTION__);
	g_object_set (G_OBJECT (m_transcoder), "ipaddress", "192.168.252.5", NULL);
	g_object_set (G_OBJECT (m_transcoder), "progressive", 0, NULL);
	g_object_set (G_OBJECT (m_ToTranscoderqueue), "max-size-buffers", 3, NULL); //  Q to transcoder
	g_object_set (G_OBJECT (m_FromTranscodermqueue), "max-size-buffers", 3, NULL); // transcoder to Q 
#endif //XG5_GW
	return;
}

// Function added to get the audio sink in the pipeline
void dynamic_link (GstPadTemplate * templ, GstPad * newpad, gpointer data)
{
//	guint apid_def = 0;

	gchar *padname;
	dyn_link *connect = (dyn_link *) data;
	padname = gst_pad_get_name (newpad);
	
	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.HN", "dynamic_link: template=%x pad=%x data=%x padname = %s connect->padname = %s\n", templ, newpad, data, padname, connect->padname);
	if (connect->padname != NULL || g_strrstr(padname, connect->padname/*"video"*/)/*!strcmp (padname, connect->padname)*/)
	{
		RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.HN", "padname %s connect->padname %s\n", padname, connect->padname);
		if (!strcmp(connect->padname,"video"))
		{
			RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.HN", "linking for video\n");
			gst_pad_link (newpad, connect->target); //demuxer to decoder
		}
		else 
		{
			RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.HN", "linking for audio\n");
			gst_pad_link (newpad, connect->target);
			/* gchar apid_instring[5];
			sprintf(apid_instring,"%04x",apid_def);
			apid_instring[4] = '\0';
			
			if (apid_def)
			{
				if (g_strrstr(padname,apid_instring)) // if eng lang pid link
				{
					RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.HN", "linking for padname %s,apid_instring %s\n",padname,apid_instring);
					gst_pad_link (newpad, connect->target);
				}
			}
			else
			{
				RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.HN", "First audio linking for padname %s,apid_instring %s\n",padname,apid_instring);
			}
			*/
		}
	}
	if(padname)
		g_free (padname);
}

void TranscoderFilterPrivate::setup_dynamic_link (GstElement * element, const gchar * padname, GstPad * target, GstElement * bin)
{
	dyn_link *connect;
	
	connect = g_new0 (dyn_link, 1);
	connect->padname = g_strdup (padname);
	connect->target = target;
	g_signal_connect (G_OBJECT (element), "pad-added", G_CALLBACK (dynamic_link), connect);
}


RMFResult TranscoderFilterPrivate::term()
{
    RMFResult result;
    
    result = RMFMediaFilterPrivate::term();

    return result;
}


//-- Filter ------------------------------------------------------------------

TranscoderFilter::TranscoderFilter()
  : m_impl( 0 )
{
}

TranscoderFilter::~TranscoderFilter()
{
}

RMFResult TranscoderFilter::init()
{
    RMFResult result;
    
    result= RMFMediaFilterBase::init();

    return result;
}

RMFResult TranscoderFilter::term()
{
    RMFResult result;
    
    result= RMFMediaFilterBase::term();

    return result;
}

RMFMediaFilterPrivate* TranscoderFilter::createPrivateFilterImpl()
{
   return new TranscoderFilterPrivate(this);
}

void* TranscoderFilter::createElement()
{
    return FILTER_IMPL->createElement();
}

void TranscoderFilter::setProperties(TranscoderProperties *prop)
{
	return FILTER_IMPL->setProperties(prop);
}
