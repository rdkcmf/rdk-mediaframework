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
#include "mediaplayersink.h"
#include "rmfprivate.h"

#include <cassert>
#include <cstdio>
#include <gst/gst.h>

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

#define DEBUG_TRICKPLAY 0

#define VIDEO_PLANE 1 /* Default Plane to use */
#define GSTPLAYERSINKBIN_EVENT_HAVE_VIDEO 0x01
#define GSTPLAYERSINKBIN_EVENT_HAVE_AUDIO 0x02

long viddec_handle = -1;
//-- Callback -----------------------------------------------------------------

template <typename CB> class Callback
{
public:
    Callback() : m_cb(NULL), m_data(NULL)
    {
    }

    void set(CB cb, void* data)
    {
        m_cb = cb;
        m_data = data;
    }

    void operator()() // only works if the callback has no arguments besides 'data'
    {
        if (m_cb)
            m_cb(m_data);
    }

    operator bool()
    {
        return m_cb != NULL;
    }

    CB cb() const { return m_cb; }
    void* data() const { return m_data; }

private:
    CB m_cb;
    void* m_data;
};


//-- MediaPlayerSinkPrivate ---------------------------------------------------

class MediaPlayerSinkPrivate: public RMFMediaSinkPrivate
{
public:
    MediaPlayerSinkPrivate(MediaPlayerSink* parent);
    ~MediaPlayerSinkPrivate();
    void onSpeedChange(float new_speed);
    void* createElement();
    RMFResult setVideoRectangle(unsigned x, unsigned y, unsigned w, unsigned h, bool apply_now);
    void setMuted(bool muted);
    bool getMuted();
    RMFResult term();
    void setVolume(float volume);
    float getVolume();
    RMFResult getMediaTime(double& t);
    long getVideoDecoderHandle() const;

    // from RMFMediaSinkBase
    /*virtual*/ RMFResult setSource(IRMFMediaSource* source);

    void setHaveVideoCallback(MediaPlayerSink::callback_t cb, void* data);
    void setHaveAudioCallback(MediaPlayerSink::callback_t cb, void* data);

	/* virtual */RMFResult getRMFError(int error_code);
private:
    class Rect
    {
    public:
        Rect() : m_x(0), m_y(0), m_w(0), m_h(0) {}
        bool isSet() const { return m_w && m_h; }
        void set(unsigned x, unsigned y, unsigned w, unsigned h)
        {
            m_x = x;
            m_y = y;
            m_w = w;
            m_h = h;
        }
        unsigned m_x, m_y, m_w, m_h; // video rect
    };

    void configureTrickplay();
    void applyVideoRectangle();
    void setMutedPriv (bool muted);

    typedef Callback<MediaPlayerSink::callback_t> GenericCallback;

    GenericCallback m_haveVideoCallback;
    GenericCallback m_haveAudioCallback;

    Rect m_videoRect;
    bool m_haveAudio;
    bool m_haveVideo;
    bool m_contentBlocked;

    GstElement* m_playersinkbin;
    static void gstplayersinkbinCB(GstElement * playersinkbin, gint status,  MediaPlayerSinkPrivate* self);
	void eventplayersinkbinCB(GstElement * playersinkbin, gint status);
    // stream props
    unsigned long m_currentPTS;
};

MediaPlayerSinkPrivate::MediaPlayerSinkPrivate(MediaPlayerSink* parent)
:   RMFMediaSinkPrivate(parent)
,   m_haveAudio(false)
,   m_haveVideo(false)
,   m_contentBlocked (false)
,   m_playersinkbin(NULL)
,   m_currentPTS(0)
{
}

MediaPlayerSinkPrivate::~MediaPlayerSinkPrivate()
{
    viddec_handle = -1;
}

RMFResult MediaPlayerSinkPrivate::term()
{
    setMutedPriv (false);
    return RMFMediaSinkPrivate::term();
}

void MediaPlayerSinkPrivate::onSpeedChange(float new_speed)
{
    if ((new_speed == 1) && (!m_contentBlocked))
    {
        setMutedPriv(false);
    }
    else
    {
        setMutedPriv(true);
    }

    if (m_playersinkbin)
    {
        g_object_set(m_playersinkbin, "program-num", -1, NULL);
    }

}

void* MediaPlayerSinkPrivate::createElement()
{
    // init stream props
    m_currentPTS = 0;

    // Create a bin to contain the sink elements.
    GstElement* bin = gst_bin_new("player_bin");

    const char PLAYERSINKBIN[] = "playersinkbin";
    // Add a playersinkbin to the bin.
    m_playersinkbin = gst_element_factory_make(PLAYERSINKBIN, "player");
    if (!m_playersinkbin)
    {
        g_print("Failed to instantiate playersinkbin (%s)", PLAYERSINKBIN);
        RMF_ASSERT(m_playersinkbin);
        return NULL;
    }
    gst_bin_add(GST_BIN(bin), m_playersinkbin);
    g_object_set(m_playersinkbin, "plane", VIDEO_PLANE, NULL);
	g_print("Adding video and audio callbacks\n");	
	g_signal_connect(m_playersinkbin, "event-callback", G_CALLBACK(gstplayersinkbinCB), this);

    // Add a ghostpad to the bin so it can proxy to the demuxer.
    GstPad* pad = gst_element_get_static_pad(m_playersinkbin, "sink");
    gst_element_add_pad(bin, gst_ghost_pad_new("sink", pad));
    gst_object_unref(GST_OBJECT(pad));

    return bin;
}

RMFResult MediaPlayerSinkPrivate::setVideoRectangle(unsigned x, unsigned y, unsigned w, unsigned h, bool apply_now)
{
    if (!w || !h)
        return RMF_RESULT_INVALID_ARGUMENT;

    m_videoRect.set(x, y, w, h);
    if (apply_now)
        applyVideoRectangle();
    return RMF_RESULT_SUCCESS;
}

void MediaPlayerSinkPrivate::setMuted(bool muted)
{
    m_contentBlocked = muted;
    setMutedPriv (muted);
}

void MediaPlayerSinkPrivate::setMutedPriv (bool muted)
{
    if (!m_playersinkbin) return;
	g_object_set(m_playersinkbin, "audio-mute", muted, NULL);
}

bool MediaPlayerSinkPrivate::getMuted()
{
    if (!m_playersinkbin) return false;
    bool muted = false;
	g_object_get(m_playersinkbin, "audio-mute", &muted, NULL);
    return muted;
}

void MediaPlayerSinkPrivate::setVolume(float volume)
{
    if (!m_playersinkbin) return;
    g_object_set(m_playersinkbin, "volume", volume, NULL);
}

float MediaPlayerSinkPrivate::getVolume()
{
    if (!m_playersinkbin) return 1.0f;
    double volume = 1.0f;
    g_object_get(m_playersinkbin, "volume", &volume, NULL);
    return (float) volume;
}

RMFResult MediaPlayerSinkPrivate::getMediaTime(double& t)
{
    RMFResult result= RMF_RESULT_FAILURE;
    gboolean rc;
    gint64 cur_pos= 0.0;
    GstFormat cur_pos_fmt= GST_FORMAT_TIME;
    if ( m_haveVideo )
    {
        rc= gst_element_query_position(m_playersinkbin, &cur_pos_fmt, &cur_pos );
    }
    else
    {
        rc= gst_element_query_position(getSink(), &cur_pos_fmt, &cur_pos );
    }

    if ( rc )
    {
        t = GST_TIME_AS_SECONDS((float)cur_pos);
        t = std::max(0., t); // prevent negative offsets
    }
    
    return result;
}

#define G_VALUE_INIT {0,{{0}}}

long MediaPlayerSinkPrivate::getVideoDecoderHandle() const
{
    GValue gVideoDecoder = G_VALUE_INIT;
    g_value_init(&gVideoDecoder, G_TYPE_POINTER);

    if (m_playersinkbin)
	g_object_get_property(G_OBJECT(m_playersinkbin), "video-decode-handle", &gVideoDecoder);

    gpointer hVideoDecoderHwdl = g_value_get_pointer(&gVideoDecoder);
    return (long) hVideoDecoderHwdl;
}

// virtual
RMFResult MediaPlayerSinkPrivate::setSource(IRMFMediaSource* source)
{
    if (source)
        applyVideoRectangle();
    return RMFMediaSinkPrivate::setSource(source);
}

void MediaPlayerSinkPrivate::setHaveVideoCallback(MediaPlayerSink::callback_t cb, void* data)
{
    m_haveVideoCallback.set(cb, data);
}

void MediaPlayerSinkPrivate::setHaveAudioCallback(MediaPlayerSink::callback_t cb, void* data)
{
    m_haveAudioCallback.set(cb, data);
}


void MediaPlayerSinkPrivate::configureTrickplay()
{
    //new segment event for rewind
    float play_speed = 0;
    getSource()->getSpeed(play_speed);
    if (play_speed < 0)
    {
#if DEBUG_TRICKPLAY
        g_print("Rewind: sending new segment event\n");
#endif
        GstElement* pipeline = getSource()->getPrivateSourceImpl()->getPipeline();
		gst_element_set_state(pipeline, GST_STATE_PAUSED);
	// Sends new segment event on playersinkbin sink pad 
        g_object_set(m_playersinkbin, "play-speed", play_speed, NULL);
        gst_element_set_state (pipeline, GST_STATE_PLAYING);
    }

}

void MediaPlayerSinkPrivate::eventplayersinkbinCB(GstElement * playersinkbin, gint status)
{
    //g_print("MediaPlayerSinkPrivate::eventplayersinkbin.\n");
    switch (status)
    {
		case GSTPLAYERSINKBIN_EVENT_HAVE_VIDEO :
    		g_print("MediaPlayerSinkPrivate::eventplayersinkbin: got Video.\n");
			m_haveVideo = true;
			m_haveVideoCallback();
		break;
		case GSTPLAYERSINKBIN_EVENT_HAVE_AUDIO :
    		g_print("MediaPlayerSinkPrivate::eventplayersinkbin. got Audio\n");
			m_haveAudio = true;
			m_haveAudioCallback();
		break;
        default:
			g_print("++++++%s status = 0x%x (Unknown)\n",
				__FUNCTION__, status);
            break;
	}
}

// signals video / audio source pad added
// static
void MediaPlayerSinkPrivate::gstplayersinkbinCB(GstElement * playersinkbin, gint status,  MediaPlayerSinkPrivate* self)
{
    self->eventplayersinkbinCB(playersinkbin, status);
}

// virtual
RMFResult MediaPlayerSinkPrivate::getRMFError(int error_code)
{
    RMFResult rmf_error = RMF_RESULT_FAILURE;

    switch(error_code)
    {
        default:
                rmf_error = RMF_RESULT_FAILURE;
            break;
    }

    return rmf_error;
}

void MediaPlayerSinkPrivate::applyVideoRectangle()
{
    if (m_videoRect.isSet())
    {
        Rect& rect = m_videoRect;
        //g_print("Playing in rectangle: %dx%d at %dx%d\n", rect.m_w, rect.m_h, rect.m_x, rect.m_y);
        char rect_str[64];
        snprintf(rect_str, sizeof(rect_str), "%d,%d,%d,%d", rect.m_x, rect.m_y, rect.m_w, rect.m_h);
        g_object_set(m_playersinkbin, "rectangle", rect_str, NULL);
    }
    else
        g_print("Playing full screen\n");

}

//-- MediaPlayerSink ----------------------------------------------------------

#define IMPL static_cast<MediaPlayerSinkPrivate*>(getPrivateSinkImpl())

RMFResult MediaPlayerSink::setVideoRectangle(unsigned x, unsigned y, unsigned w, unsigned h, bool apply_now)
{
    return IMPL->setVideoRectangle(x, y, w, h, apply_now);
}

// virtual
void MediaPlayerSink::onSpeedChange(float new_speed)
{
    IMPL->onSpeedChange(new_speed);
}

// virtual
void* MediaPlayerSink::createElement()
{
    return IMPL->createElement();
}

void MediaPlayerSink::setMuted(bool muted)
{
    return IMPL->setMuted(muted);
}

bool MediaPlayerSink::getMuted()
{
    return IMPL->getMuted();
}

void MediaPlayerSink::setVolume(float volume)
{
    return IMPL->setVolume(volume);
}

float MediaPlayerSink::getVolume()
{
    return IMPL->getVolume();
}

RMFResult MediaPlayerSink::getMediaTime(double& t)
{
   return IMPL->getMediaTime(t);
}

void MediaPlayerSink::setHaveVideoCallback(callback_t cb, void* data)
{
    return IMPL->setHaveVideoCallback(cb, data);
}

void MediaPlayerSink::setHaveAudioCallback(callback_t cb, void* data)
{
    return IMPL->setHaveAudioCallback(cb, data);
}

void MediaPlayerSink::setVolumeChangedCallback(callback_t cb, void* data)
{
    // NOP. Implement this?
    (void) cb;
    (void) data;
}

void MediaPlayerSink::setMuteChangedCallback(callback_t cb, void* data)
{
    // NOP. Implement this?
    (void) cb;
    (void) data;
}

long MediaPlayerSink::getVideoDecoderHandle() const
{
    return IMPL->getVideoDecoderHandle();
}

// virtual
RMFMediaSinkPrivate* MediaPlayerSink::createPrivateSinkImpl()
{
    return new MediaPlayerSinkPrivate(this);
}

