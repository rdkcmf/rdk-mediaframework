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
#include <unistd.h>
#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

#include "config.h"

#define MPSINKLOG_ERROR(format...)       RDK_LOG(RDK_LOG_ERROR,  "LOG.RDK.MPSINK", format)
#define MPSINKLOG_WARN(format...)        RDK_LOG(RDK_LOG_WARN,   "LOG.RDK.MPSINK", format)
#define MPSINKLOG_INFO(format...)        RDK_LOG(RDK_LOG_INFO,   "LOG.RDK.MPSINK", format)
#define MPSINKLOG_DEBUG(format...)       RDK_LOG(RDK_LOG_DEBUG,  "LOG.RDK.MPSINK", format)
#define MPSINKLOG_TRACE(format...)       RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.MPSINK", format)

#include <string.h>
#include "config.h"

#define VIDEO_PLANE 1 // C
#define GSTPLAYERSINKBIN_EVENT_HAVE_VIDEO 0x01
#define GSTPLAYERSINKBIN_EVENT_HAVE_AUDIO 0x02
#define GSTPLAYERSINKBIN_EVENT_FIRST_VIDEO_FRAME 0x03
#define GSTPLAYERSINKBIN_EVENT_FIRST_AUDIO_FRAME 0x04
#define GSTPLAYERSINKBIN_EVENT_ERROR_VIDEO_UNDERFLOW 0x06
#define GSTPLAYERSINKBIN_EVENT_ERROR_AUDIO_UNDERFLOW 0x07
#define GSTPLAYERSINKBIN_EVENT_ERROR_VIDEO_PTS 0x08
#define GSTPLAYERSINKBIN_EVENT_ERROR_AUDIO_PTS 0x09
#define GSTPLAYERSINKBIN_EVENT_PMT_UPDATE      0x20
#define GSTPLAYERSINKBIN_EVENT_LANGUAGE_CHANGE 0x21

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
    RMFResult setVideoRectangle(unsigned x, unsigned y, unsigned w, unsigned h, unsigned steps, unsigned max_w, unsigned max_h, bool blocking, bool apply_now);
    void setMuted(bool muted);
    bool getMuted();
    RMFResult term();
    void setVolume(float volume);
    float getVolume();
    RMFResult getMediaTime(double& t);
    unsigned long getVideoDecoderHandle() const;
    const char * getAudioLanguages();
    const char * getCCDescriptor();
    const char * getEISSData();
    void setAudioLanguage (const char* pAudioLang);
    const char * getPreferredAudioLanguage(void);
    void setAuxiliaryAudioLang(const char* pAudioLang);
    void setPreferredAudioTrack(const char* pAudioTrackPreference);
    void setVideoZoom (unsigned short zoomVal);
    void setEissFilterStatus (bool eissStatus);
    void setVideoKeySlot (const char *keySlot);
    void setAudioKeySlot (const char *keySlot);
    void deleteVideoKeySlot (void);
    void deleteAudioKeySlot (void);
    int getVideoPid();
    int getAudioPid();
    void addInbandFilterFunc(unsigned int funcPointer,unsigned int cbData);
    void setPrimeDecode(bool enable);
    void setPipOutput(bool pip);
    bool getPipOutput() const;
    void setZOrder(int order);
    int getZOrder() const;
    void setVideoMute(bool muted);
    void showLastFrame(bool enable);
    void setAudioMute(bool muted);
    bool getVideoMute(void);
    const char *getAudioTracks(void);
    const char *getAudioTrackSelected(void);
    void getSubtitles (char* value, unsigned int max_size);
    void getTeletext (char* value, unsigned int max_size);
    void setSubtitlesTrack (const char* channel_path, const char* value);
    void setTeletextTrack (const char* channel_path, const char* value);
    void getSubtitlesTrack (char* value, unsigned int max_size);
    void getTeletextTrack (char* value, unsigned int max_size);
    void resetTtxtSubs(void);

    // from RMFMediaSinkBase
    /*virtual*/ RMFResult setSource(IRMFMediaSource* source);
    /* virtual */RMFResult getRMFError(int error_code);

    void setHaveVideoCallback(MediaPlayerSink::callback_t cb, void* data);
    void setHaveAudioCallback(MediaPlayerSink::callback_t cb, void* data);
    void setEISSDataCallback(MediaPlayerSink::callback_t cb, void* data);

    void setAudioPlayingCallback(MediaPlayerSink::callback_t cb, void* data);
    void setVideoPlayingCallback(MediaPlayerSink::callback_t cb, void* data);
    void setMediaWarningCallback(MediaPlayerSink::callback_t cb, void* data);
    void setPmtUpdateCallback(MediaPlayerSink::callback_t cb, void* data);
    void setLanguageChangeCallback(MediaPlayerSink::callback_t cb, void* data);
    const char* getMediaWarningString(void);
    short getMediaBufferSize(void);
    void setNetWorkBufferSize (short bufferSize);
    RMFCurrentAssetType getCurrentAssetType();

private:
    class Rect
    {
    public:
        Rect() : m_x(0), m_y(0), m_w(0), m_h(0), m_steps(0), m_max_w(0), m_max_h(0), m_blocking(false)  {}
        bool isSet() const { return m_w && m_h; }
        void set(unsigned x, unsigned y, unsigned w, unsigned h)
        {
            m_x = x;
            m_y = y;
            m_w = w;
            m_h = h;
        }
        void set(unsigned x, unsigned y, unsigned w, unsigned h, unsigned steps, unsigned max_w, unsigned max_h, bool blocking)
        {
            m_x = x;
            m_y = y;
            m_w = w;
            m_h = h;
            m_steps = steps;
            m_max_w = max_w;
            m_max_h = max_h;
            m_blocking = blocking;
        }
        unsigned m_x, m_y, m_w, m_h; // video rect
        unsigned m_steps, m_max_w, m_max_h;
        bool     m_blocking;
    };

    void applyVideoRectangle();
    bool setAudioMutePriv (bool muted);
    bool setVideoMutePriv (bool muted);


    typedef Callback<MediaPlayerSink::callback_t> GenericCallback;

    GenericCallback m_haveVideoCallback;
    GenericCallback m_haveAudioCallback;
    GenericCallback m_haveEISSDataCallback;
    GenericCallback m_audioPlayingCallback;
    GenericCallback m_videoPlayingCallback;

    GenericCallback m_mediaWarningCallback;
    GenericCallback m_PmtUpdateCallback;
    GenericCallback m_LanguageChangeCallback;

    Rect m_videoRect;
    bool m_haveAudio;
    bool m_haveVideo;
    bool m_contentBlocked;
    GstElement* m_playersinkbin;
    gulong  m_probingID;
    const char *m_mediaWarnMsg;
    std::string m_audioLanguages;
    std::string m_audioTracks;
    std::string m_audioTrack;
    std::string m_captionDescriptor;
    std::string m_eissDataBuffer;
    std::string m_preferredAudioLang;
    double m_speed;
    float m_volume;
    bool m_show_last_frame;


    static const char *m_mediaWarnMsgVideoUnderFlow;
    static const char *m_mediaWarnMsgAudioUnderFlow;
    static const char *m_mediaWarnMsgVideoPTS;
    static const char *m_mediaWarnMsgAudioPTS;
    static const char *m_mediaWarnMsgBlockFailed;

    static void gstplayersinkbinCB(GstElement * playersinkbin, gint status,  MediaPlayerSinkPrivate* self);
    static void eissupdateCB(GstElement * playersinkbin, gchar *data,  MediaPlayerSinkPrivate* self);
#ifdef USE_GST1
    static GstPadProbeReturn playersinkbin_data_probeCB (GstPad *pad, GstPadProbeInfo *info, gpointer data);
#else
    static gboolean playersinkbin_data_probeCB (GstPad *pad, GstBuffer* buffer, gpointer data);
#endif
    void playersinkbin_cancel_probeCB (GstPad *pad);
    void eventplayersinkbinCB(GstElement * playersinkbin, gint status);
    void eissplayersinkbinCB(GstElement * playersinkbin, gchar* status);
    // stream props
};

const char *MediaPlayerSinkPrivate::m_mediaWarnMsgVideoUnderFlow = "BUFFER_UNDERFLOW";
const char *MediaPlayerSinkPrivate::m_mediaWarnMsgAudioUnderFlow = "AUDIO_BUFFER_UNDERFLOW";
const char *MediaPlayerSinkPrivate::m_mediaWarnMsgVideoPTS       = "VIDEO_PTS_ERROR";
const char *MediaPlayerSinkPrivate::m_mediaWarnMsgAudioPTS       = "AUDIO_PTS_ERROR";
const char *MediaPlayerSinkPrivate::m_mediaWarnMsgBlockFailed    = "AV_BLOCK_FAILED";

MediaPlayerSinkPrivate::MediaPlayerSinkPrivate(MediaPlayerSink* parent)
:   RMFMediaSinkPrivate(parent)
,   m_haveAudio(false)
,   m_haveVideo(false)
,   m_contentBlocked (false)
,   m_playersinkbin(NULL)
,   m_probingID(0)
,   m_mediaWarnMsg(NULL)
,   m_speed(1.0)
,   m_volume(1.0)
,   m_show_last_frame(true)
{
}

MediaPlayerSinkPrivate::~MediaPlayerSinkPrivate()
{
}

RMFResult MediaPlayerSinkPrivate::term()
{
    g_object_set(m_playersinkbin, "show-last-frame", 0, NULL);
    setAudioMutePriv(false);
    return RMFMediaSinkPrivate::term();
}

void MediaPlayerSinkPrivate::onSpeedChange(float new_speed)
{
    if (m_playersinkbin)
    {
        if (RMFMediaSinkPrivate::getSource()->getPrivateSourceImpl()->isLiveSource())
           g_object_set(m_playersinkbin, "is-live", TRUE, NULL);
        else
            g_object_set(m_playersinkbin, "is-live", FALSE, NULL);

        g_object_set(m_playersinkbin, "play-speed", new_speed, NULL);
        g_object_set(m_playersinkbin, "show-last-frame", 1, NULL);
    }

    if ((new_speed == 1) && (!m_contentBlocked) && (m_volume != 0.0))
    {
        setAudioMutePriv(false);
    }
    else
    {
        setAudioMutePriv(true);
    }
    m_speed = new_speed;
}

void* MediaPlayerSinkPrivate::createElement()
{
    // Create a bin to contain the sink elements.
    const char PLAYERSINKBIN[] = "playersinkbin";
    m_playersinkbin = gst_element_factory_make(PLAYERSINKBIN, NULL);
    if (!m_playersinkbin)
    {
        MPSINKLOG_ERROR("Failed to instantiate playersinkbin (%s)\n", PLAYERSINKBIN);
        RMF_ASSERT(m_playersinkbin);
        return NULL;
    }
    g_object_set(m_playersinkbin, "plane", VIDEO_PLANE, NULL);
    g_object_set(m_playersinkbin, "show-last-frame", m_show_last_frame, NULL);
    MPSINKLOG_INFO("Adding video and audio callbacks\n");
    g_signal_connect(m_playersinkbin, "event-callback", G_CALLBACK(gstplayersinkbinCB), this);
    g_signal_connect(m_playersinkbin, "eiss-callback", G_CALLBACK(eissupdateCB), this);

    m_speed = 1.0;
    /* To add a debug print to know whether the data is received or not */
    GstPad* pad = gst_element_get_static_pad(m_playersinkbin, "sink");
#ifdef USE_GST1
    m_probingID = gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, playersinkbin_data_probeCB, this, NULL);
#else
    m_probingID = gst_pad_add_buffer_probe(pad, G_CALLBACK(playersinkbin_data_probeCB), this);
#endif
    gst_object_unref(GST_OBJECT(pad));

    return m_playersinkbin;
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

RMFResult MediaPlayerSinkPrivate::setVideoRectangle(unsigned x, unsigned y, unsigned w, unsigned h, unsigned steps, unsigned max_w, unsigned max_h, bool blocking, bool apply_now)
{
    if (!w || !h)
        return RMF_RESULT_INVALID_ARGUMENT;

    m_videoRect.set(x, y, w, h, steps, max_w, max_h, blocking);
    if (apply_now)
        applyVideoRectangle();
    return RMF_RESULT_SUCCESS;
}

void MediaPlayerSinkPrivate::setMuted(bool muted)
{
    m_contentBlocked = muted;

    bool result = true;
    result &= setAudioMutePriv(muted);
    result &= setVideoMutePriv(muted);

    if((0 == access("/tmp/force_block_change_failure", F_OK)) && (muted))
    {
        /* Force failure when enabling AV mute while the flag is present.*/
        if(result)
            MPSINKLOG_WARN("MediaPlayerSinkPrivate:: forcing block change failure\n");
        result = false;
    }
    if((!result) && (muted))
    {
        m_mediaWarnMsg = m_mediaWarnMsgBlockFailed;
        m_mediaWarningCallback();
    }

    MPSINKLOG_WARN("setMuted result is %s\n", (true == result? "success": "failure"));
}

bool MediaPlayerSinkPrivate::setAudioMutePriv (bool muted)
{
    if (!m_playersinkbin)
        return false;

    g_object_set(m_playersinkbin, "audio-mute", muted, NULL);
    /* Since the video mute is not called standalone, this should suffice */
    g_object_set(m_playersinkbin, "enable-cc-passthru", !muted, NULL);
    return true;
}

bool MediaPlayerSinkPrivate::setVideoMutePriv (bool muted)
{
    if (!m_playersinkbin)
        return false;

    g_object_set(m_playersinkbin, "video-mute", muted, NULL);
    return true;
}

bool MediaPlayerSinkPrivate::getMuted()
{
    if (!m_playersinkbin)
        return false;

    return m_contentBlocked;
}

void MediaPlayerSinkPrivate::setVolume(float volume)
{
    if (!m_playersinkbin)
        return;

    gdouble vol = (gdouble) volume;
    m_volume = volume;
    g_object_set(m_playersinkbin, "volume", vol, NULL);
}

float MediaPlayerSinkPrivate::getVolume()
{
    gdouble volume = 1.0f;
    if (!m_playersinkbin) return 1.0f;
    g_object_get(m_playersinkbin, "volume", &volume, NULL);
    m_volume = (float) volume;
    return m_volume;
}

RMFResult MediaPlayerSinkPrivate::getMediaTime(double& t)
{
    gboolean rc;
    gint64 cur_pos= 0.0;
    GstFormat cur_pos_fmt= GST_FORMAT_TIME;
    if ( m_haveVideo )
    {
#ifdef USE_GST1
        rc= gst_element_query_position(m_playersinkbin, cur_pos_fmt, &cur_pos );
#else
        rc= gst_element_query_position(m_playersinkbin, &cur_pos_fmt, &cur_pos );
#endif
    }
    else
    {
#ifdef USE_GST1
        rc= gst_element_query_position(getSink(), cur_pos_fmt, &cur_pos );
#else
        rc= gst_element_query_position(getSink(), &cur_pos_fmt, &cur_pos );
#endif
    }

    if (!rc)
        return RMF_RESULT_FAILURE;

    t = (double) GST_TIME_AS_NSECONDS(cur_pos);
    t = t/1000000000.0L;
    t = std::max(0., t); // prevent negative offsets
    
    return RMF_RESULT_SUCCESS;
}

void MediaPlayerSinkPrivate::setAudioLanguage (const char* pAudioLang)
{
    if (pAudioLang)
    {
        MPSINKLOG_WARN ("Language Selected = %s\n", pAudioLang);
        if (m_playersinkbin)
            g_object_set(m_playersinkbin, "preferred-language", pAudioLang, NULL);
    }
    else
        MPSINKLOG_ERROR("The audio language selected is empty. Ignore it\n");
    return;
}

void MediaPlayerSinkPrivate::setAuxiliaryAudioLang (const char* pAudioLang)
{
    if (pAudioLang)
    {
        MPSINKLOG_WARN ("Auxiliary Language Selected = %s\n", pAudioLang);
        if (m_playersinkbin)
            g_object_set(m_playersinkbin, "auxiliary-audio-language", pAudioLang, NULL);
    }
    else
        MPSINKLOG_ERROR("The audio language selected is empty. Ignore it\n");
    return;
}

void MediaPlayerSinkPrivate::setPreferredAudioTrack (const char* pAudioTrackPreference)
{
    if (pAudioTrackPreference)
    {
        MPSINKLOG_WARN("Audio Track Preference = %s\n", pAudioTrackPreference);
        if (m_playersinkbin)
            g_object_set(m_playersinkbin, "preferred-audio-track", pAudioTrackPreference, NULL);
    }
    else
        MPSINKLOG_ERROR("The audio track preference is empty. Ignore it\n");
    return;
}

void MediaPlayerSinkPrivate::getSubtitles (char* value, unsigned int max_size)
{
  GValue gvalue = G_VALUE_INIT;
  const char *val_str;
  g_value_init(&gvalue, G_TYPE_STRING);

  g_object_get_property(G_OBJECT(m_playersinkbin), "subs-tracks", &gvalue);
  val_str = (const char *) g_value_get_string (&gvalue);
  if (val_str) {
      if (strlen(val_str) < max_size) {
          strcpy(value, val_str);
      }
  }
  g_value_unset(&gvalue);
}

void MediaPlayerSinkPrivate::getTeletext (char* value, unsigned int max_size)
{
  GValue gvalue = G_VALUE_INIT;
  const char *val_str;
  g_value_init(&gvalue, G_TYPE_STRING);

  g_object_get_property(G_OBJECT(m_playersinkbin), "ttxt-tracks", &gvalue);
  val_str = (const char *) g_value_get_string (&gvalue);
  if (val_str) {
      if (strlen(val_str) < max_size) {
          strcpy(value, val_str);
      }
  }
  g_value_unset(&gvalue);
}

void MediaPlayerSinkPrivate::getSubtitlesTrack(char* value, unsigned int max_size)
{
  GValue gvalue = G_VALUE_INIT;
  const char *val_str;
  g_value_init(&gvalue, G_TYPE_STRING);

  g_object_get_property(G_OBJECT(m_playersinkbin), "subs-track-selected", &gvalue);
  val_str = (const char *) g_value_get_string (&gvalue);
  if (val_str) {
      if (strlen(val_str) < max_size) {
          strcpy(value, val_str);
      }
  }
  g_value_unset(&gvalue);
}

void MediaPlayerSinkPrivate::getTeletextTrack (char* value, unsigned int max_size)
{
  GValue gvalue = G_VALUE_INIT;
  const char *val_str;
  g_value_init(&gvalue, G_TYPE_STRING);

  g_object_get_property(G_OBJECT(m_playersinkbin), "ttxt-track-selected", &gvalue);
  val_str = (const char *) g_value_get_string (&gvalue);
  if (val_str) {
      if (strlen(val_str) < max_size) {
          strcpy(value, val_str);
      }
  }
  g_value_unset(&gvalue);
}

void MediaPlayerSinkPrivate::setSubtitlesTrack (const char* channel_path, const char* value)
{
  g_object_set(G_OBJECT(m_playersinkbin), "subtecsink-path", channel_path, NULL);
  g_object_set(G_OBJECT(m_playersinkbin), "subs-track-selected", value, NULL);
}

void MediaPlayerSinkPrivate::setTeletextTrack (const char* channel_path, const char* value)
{
  g_object_set(G_OBJECT(m_playersinkbin), "subtecsink-path", channel_path, NULL);
  g_object_set(G_OBJECT(m_playersinkbin), "ttxt-track-selected", value, NULL);
}

void MediaPlayerSinkPrivate::resetTtxtSubs (void)
{
  g_object_set(G_OBJECT(m_playersinkbin), "ttxt-subs-reset", true, NULL);
}

void MediaPlayerSinkPrivate::setVideoZoom (unsigned short zoomVal)
{
    MPSINKLOG_WARN("Zoom settings received as %u\n", zoomVal);
    if (m_playersinkbin)
        g_object_set(m_playersinkbin, "zoom", zoomVal, NULL);

    return;
}

void MediaPlayerSinkPrivate::setEissFilterStatus (bool eissStatus)
{
    MPSINKLOG_WARN("EISS filter status received as %d \n", eissStatus);
    if (m_playersinkbin)
            g_object_set(m_playersinkbin, "enable-ib-eiss", eissStatus, NULL);

    return;
}

void MediaPlayerSinkPrivate::setVideoKeySlot (const char *keySlot)
{
    MPSINKLOG_WARN("Video Key Slot received\n");
    if (m_playersinkbin)
            g_object_set(m_playersinkbin, "video-keyslot", keySlot, NULL);
}

void MediaPlayerSinkPrivate::setAudioKeySlot (const char *keySlot)
{
    MPSINKLOG_WARN("Audio Key Slot received\n");
    if (m_playersinkbin)
            g_object_set(m_playersinkbin, "audio-keyslot", keySlot, NULL);

}

int MediaPlayerSinkPrivate::getVideoPid()
{
    int videoPid = -1;

    if (!m_playersinkbin)
        return videoPid;

    g_object_get(m_playersinkbin, "video-pid", &videoPid, NULL);

    return videoPid;
}

int MediaPlayerSinkPrivate::getAudioPid()
{
    int audioPid = -1;

    if (!m_playersinkbin)
        return audioPid;

    g_object_get(m_playersinkbin, "audio-pid", &audioPid, NULL);

    return audioPid;
}

#define G_VALUE_INIT {0,{{0}}}
unsigned long MediaPlayerSinkPrivate::getVideoDecoderHandle() const
{
    GValue gVideoDecoder = G_VALUE_INIT;
    g_value_init(&gVideoDecoder, G_TYPE_POINTER);

    if (m_playersinkbin)
        g_object_get_property(G_OBJECT(m_playersinkbin), "video-decode-handle", &gVideoDecoder);

    gpointer hVideoDecoderHwdl = g_value_get_pointer(&gVideoDecoder);

    if (m_playersinkbin)
        g_value_unset(&gVideoDecoder);

    MPSINKLOG_WARN("The Video Decoder Handle:%p\n", hVideoDecoderHwdl);
    return (unsigned long) hVideoDecoderHwdl;
}

const char * MediaPlayerSinkPrivate::getAudioLanguages()
{
    GValue value = G_VALUE_INIT;
    g_value_init(&value, G_TYPE_STRING);

    g_object_get_property(G_OBJECT(m_playersinkbin), "available-languages", &value);

    m_audioLanguages = (const char *) g_value_get_string (&value);

    g_value_unset(&value);

    return m_audioLanguages.c_str();
}

const char * MediaPlayerSinkPrivate::getAudioTracks()
{
    GValue value = G_VALUE_INIT;

    g_value_init(&value, G_TYPE_STRING);

    g_object_get_property(G_OBJECT(m_playersinkbin), "audio-tracks", &value);

    const char* val_str = (const char *) g_value_get_string (&value);
    if (val_str) {
        m_audioTracks = val_str;
    } else {
        m_audioTracks = "";
    }

    g_value_unset(&value);

    return m_audioTracks.c_str();
}

const char * MediaPlayerSinkPrivate::getAudioTrackSelected(void)
{
    GValue value = G_VALUE_INIT;

    g_value_init(&value, G_TYPE_STRING);

    g_object_get_property(G_OBJECT(m_playersinkbin), "audio-track-selected", &value);

    const char* val_str = (const char *) g_value_get_string (&value);
    if (val_str) {
        m_audioTrack = val_str;
    } else {
        m_audioTrack = "";
    }

    g_value_unset(&value);

    return m_audioTrack.c_str();
}

const char * MediaPlayerSinkPrivate::getCCDescriptor()
{
    GValue value = G_VALUE_INIT;
    g_value_init(&value, G_TYPE_STRING);

    g_object_get_property(G_OBJECT(m_playersinkbin), "cc-descriptor", &value);

    m_captionDescriptor = (const char *) g_value_get_string (&value);

    g_value_unset(&value);

    return m_captionDescriptor.c_str();
}

const char * MediaPlayerSinkPrivate::getEISSData()
{
    return m_eissDataBuffer.c_str();
}

void MediaPlayerSinkPrivate::addInbandFilterFunc(unsigned int funcPointer,unsigned int cbData)
{
    bool ret = false;
    MPSINKLOG_WARN("EISS Received InbandFilterFunction reference %d , %d \n", funcPointer, cbData);

    GValueArray *array = NULL;

    array =  g_value_array_new (0);

    guint param1 = (guint)funcPointer;
    guint param2 = (guint)cbData;
    GValue value1 = G_VALUE_INIT;
    GValue value2 = G_VALUE_INIT;

    g_value_init (&value1, G_TYPE_UINT);
    g_value_init (&value2, G_TYPE_UINT);

    g_value_set_uint(&value1, param1);
    g_value_set_uint(&value2, param2);

    if(array)
    {
        g_value_array_append(array, &value1);
        g_value_array_append(array, &value2);
        g_object_set(m_playersinkbin, "inband-filter-eiss", array, NULL);
    }

    g_value_array_free(array);

    return;
}

// virtual
RMFResult MediaPlayerSinkPrivate::setSource(IRMFMediaSource* source)
{
    if (source)
        applyVideoRectangle();

    if(source)
    {
        if (source->getPrivateSourceImpl()->isLiveSource())
           g_object_set(m_playersinkbin, "is-live", TRUE, NULL);
        else
            g_object_set(m_playersinkbin, "is-live", FALSE, NULL);
    }

    return RMFMediaSinkPrivate::setSource(source);
}

const char* MediaPlayerSinkPrivate::getMediaWarningString(void)
{
    return m_mediaWarnMsg;
}

short MediaPlayerSinkPrivate::getMediaBufferSize (void)
{
    guint bufferVal = 0;

    if (m_playersinkbin)
        g_object_get(m_playersinkbin, "fifo-size", &bufferVal, NULL);

    return (short)bufferVal;
}

void MediaPlayerSinkPrivate::setHaveVideoCallback(MediaPlayerSink::callback_t cb, void* data)
{
    m_haveVideoCallback.set(cb, data);
}

void MediaPlayerSinkPrivate::setHaveAudioCallback(MediaPlayerSink::callback_t cb, void* data)
{
    m_haveAudioCallback.set(cb, data);
}

void MediaPlayerSinkPrivate::setEISSDataCallback(MediaPlayerSink::callback_t cb, void* data)
{
    m_haveEISSDataCallback.set(cb, data);
}

void MediaPlayerSinkPrivate::setAudioPlayingCallback(MediaPlayerSink::callback_t cb, void* data)
{
    m_audioPlayingCallback.set(cb, data);
}

void MediaPlayerSinkPrivate::setVideoPlayingCallback(MediaPlayerSink::callback_t cb, void* data)
{
    m_videoPlayingCallback.set(cb, data);
}

void MediaPlayerSinkPrivate::setMediaWarningCallback(MediaPlayerSink::callback_t cb, void* data)
{
    m_mediaWarningCallback.set(cb, data);
}

void MediaPlayerSinkPrivate::setPmtUpdateCallback(MediaPlayerSink::callback_t cb, void* data)
{
    m_PmtUpdateCallback.set(cb, data);
}

void MediaPlayerSinkPrivate::setLanguageChangeCallback(MediaPlayerSink::callback_t cb, void* data)
{
    m_LanguageChangeCallback.set(cb, data);
}

void MediaPlayerSinkPrivate::eventplayersinkbinCB(GstElement * playersinkbin, gint status)
{
    switch (status)
    {
        case GSTPLAYERSINKBIN_EVENT_PMT_UPDATE :
            m_PmtUpdateCallback();
            break;
        case GSTPLAYERSINKBIN_EVENT_HAVE_VIDEO :
            MPSINKLOG_INFO("MediaPlayerSinkPrivate::eventplayersinkbin: got Video PES.\n");
            m_haveVideo = true;
            m_haveVideoCallback();
            break;
        case GSTPLAYERSINKBIN_EVENT_HAVE_AUDIO :
            MPSINKLOG_INFO("MediaPlayerSinkPrivate::eventplayersinkbin. got Audio PES\n");
            m_haveAudioCallback();
            break;
        case GSTPLAYERSINKBIN_EVENT_FIRST_VIDEO_FRAME:
            MPSINKLOG_INFO("MediaPlayerSinkPrivate::eventplayersinkbin. got First Video Frame\n");
            m_videoPlayingCallback();
            break;
        case GSTPLAYERSINKBIN_EVENT_FIRST_AUDIO_FRAME:
            MPSINKLOG_INFO("MediaPlayerSinkPrivate::eventplayersinkbin. got First Audio Sample\n");
            m_audioPlayingCallback();
            break;
        case GSTPLAYERSINKBIN_EVENT_ERROR_VIDEO_UNDERFLOW:
            if (m_speed == 1.0)
            {
                m_mediaWarnMsg = m_mediaWarnMsgVideoUnderFlow;
                m_mediaWarningCallback();
            }
            break;
        case GSTPLAYERSINKBIN_EVENT_ERROR_AUDIO_UNDERFLOW:
            if (m_speed == 1.0)
            {
                m_mediaWarnMsg = m_mediaWarnMsgAudioUnderFlow;
                m_mediaWarningCallback();
            }
            break;
        case GSTPLAYERSINKBIN_EVENT_ERROR_VIDEO_PTS:
            if (m_speed == 1.0)
            {
                MPSINKLOG_WARN("MediaPlayerSinkPrivate::eventplayersinkbin. Got Video PTS Error\n");
                m_mediaWarnMsg = m_mediaWarnMsgVideoPTS;
                /*XREINT-2233: PTS Errors are not needed to be notified to XRE Server as it is flooding the splunk; only underflow is needed */
                //m_mediaWarningCallback();
            }
            break;
        case GSTPLAYERSINKBIN_EVENT_ERROR_AUDIO_PTS:
            if (m_speed == 1.0)
            {
                MPSINKLOG_WARN("MediaPlayerSinkPrivate::eventplayersinkbin. Got Audio PTS Error\n");
                m_mediaWarnMsg = m_mediaWarnMsgAudioPTS;
                /*XREINT-2233: PTS Errors are not needed to be notified to XRE Server as it is flooding the splunk; only underflow is needed */
                //m_mediaWarningCallback();
            }
            break;
	case GSTPLAYERSINKBIN_EVENT_LANGUAGE_CHANGE :
             MPSINKLOG_INFO("MediaPlayerSinkPrivate::eventplayersinkbin: got language Change.\n");
             m_LanguageChangeCallback();
             break;
        default:
            MPSINKLOG_INFO("%s status = 0x%x (Unknown)\n", __FUNCTION__, status);
            break;
    }
}
void MediaPlayerSinkPrivate::eissplayersinkbinCB(GstElement * playersinkbin, gchar* data)
{
     m_eissDataBuffer = (std::string)data;
     m_haveEISSDataCallback();
}

// signals video / audio source pad added
void MediaPlayerSinkPrivate::playersinkbin_cancel_probeCB (GstPad *pad)
{
    if (pad)
#ifdef USE_GST1
        gst_pad_remove_probe(pad, m_probingID);
#else
        gst_pad_remove_buffer_probe(pad, m_probingID);
#endif
    return;
}

// static
#ifdef USE_GST1
GstPadProbeReturn MediaPlayerSinkPrivate::playersinkbin_data_probeCB (GstPad *pad, GstPadProbeInfo *info, gpointer data)
#else
gboolean MediaPlayerSinkPrivate::playersinkbin_data_probeCB (GstPad *pad, GstBuffer* buffer, gpointer data)
#endif
{
    if (data)
    {
        MediaPlayerSinkPrivate* self = (MediaPlayerSinkPrivate*) data;
        self->playersinkbin_cancel_probeCB(pad);
        MPSINKLOG_WARN("MediaPlayerSink : First Buffer Received from the Source Element after DTCP\n");
    }
#ifdef USE_GST1
    return GST_PAD_PROBE_OK;
#else
    return TRUE;
#endif
}

// static
void MediaPlayerSinkPrivate::gstplayersinkbinCB(GstElement * playersinkbin, gint status,  MediaPlayerSinkPrivate* self)
{
    self->eventplayersinkbinCB(playersinkbin, status);
}

// static
void MediaPlayerSinkPrivate::eissupdateCB(GstElement * playersinkbin, gchar *data,  MediaPlayerSinkPrivate* self)
{
    self->eissplayersinkbinCB(playersinkbin, data);
}


// virtual
RMFResult MediaPlayerSinkPrivate::getRMFError(int error_code)
{
    RMFResult rmf_error = RMF_RESULT_FAILURE;

	MPSINKLOG_ERROR("MediaPlayerSinkPrivate::getRMFError Error Code - %d\n", error_code);
#if 0 /* treat all errors as failure error */
    switch(error_code)
    {
		case GST_STREAM_ERROR_DEMUX:

			rmf_error = RMF_RESULT_DECODE_ERROR;
			break;
        default:
			rmf_error = RMF_RESULT_FAILURE;
            break;
    }
#endif
    return rmf_error;
}

#define DEFAULT_WINDOW_W 1280
#define DEFAULT_WINDOW_H  720
void MediaPlayerSinkPrivate::applyVideoRectangle()
{
    if (m_videoRect.isSet())
    {
        Rect& rect = m_videoRect;
        char rect_str[256];
        if (rect.m_steps && rect.m_max_w && rect.m_max_h) {
          if (g_object_class_find_property(G_OBJECT_GET_CLASS(m_playersinkbin), "rectangle_move")) {
            snprintf(rect_str, sizeof(rect_str), "%d,%d,%d,%d,%d,%d,%d,%d", rect.m_x, rect.m_y, rect.m_w, rect.m_h, rect.m_steps, rect.m_max_w, rect.m_max_h, rect.m_blocking?1:0);
            g_object_set(m_playersinkbin, "rectangle_move", rect_str, NULL);
          } else {
            unsigned x,y,w,h;
            x = (rect.m_x * DEFAULT_WINDOW_W) / rect.m_max_w;
            y = (rect.m_y * DEFAULT_WINDOW_H) / rect.m_max_h;
            w = (rect.m_w * DEFAULT_WINDOW_W) / rect.m_max_w;
            h = (rect.m_h * DEFAULT_WINDOW_H) / rect.m_max_h;
            snprintf(rect_str, sizeof(rect_str), "%d,%d,%d,%d", x,y,w,h);
            g_object_set(m_playersinkbin, "rectangle", rect_str, NULL);
          }
        } else {
          snprintf(rect_str, sizeof(rect_str), "%d,%d,%d,%d", rect.m_x, rect.m_y, rect.m_w, rect.m_h);
          g_object_set(m_playersinkbin, "rectangle", rect_str, NULL);
        }
    }
}

void MediaPlayerSinkPrivate::setNetWorkBufferSize(short bufferSize)
{
    if (!m_playersinkbin)
        return;

    if (bufferSize > 0)
        g_object_set(m_playersinkbin, "fifo-pre-buf-min", bufferSize, NULL);

    return;
}

void MediaPlayerSinkPrivate::setPrimeDecode(bool enable)
{
        g_object_set(m_playersinkbin, "prime", enable, NULL);
}

/**
 * @brief This function returns the channel type that is playing now.
 *
 * @return Returns the channel type.
 */
RMFCurrentAssetType MediaPlayerSinkPrivate::getCurrentAssetType()
{
    RMFCurrentAssetType type = RMF_CURRENTLY_PLAYING_AV_ASSET;
    g_object_get(m_playersinkbin, "channel-type", &type, NULL);
	MPSINKLOG_INFO("MediaPlayerSinkPrivate::getCurrentAssetType Type = %d\n", type);

    return type;
}

void MediaPlayerSinkPrivate::setPipOutput(bool pip)
{
    g_object_set(m_playersinkbin, "pip", pip, NULL);
}

bool MediaPlayerSinkPrivate::getPipOutput() const
{
    gboolean pip = false;

    if (m_playersinkbin)
        g_object_get(m_playersinkbin, "pip", &pip, NULL);

    return pip;
}

void MediaPlayerSinkPrivate::setZOrder(int order)
{
    g_object_set(m_playersinkbin, "zorder", order, NULL);
}

int MediaPlayerSinkPrivate::getZOrder() const
{
    int zorder = 0;

    if (m_playersinkbin)
        g_object_get(m_playersinkbin, "zorder", &zorder, NULL);

    return zorder;
}

void MediaPlayerSinkPrivate::setVideoMute(bool muted)
{
	this->setVideoMutePriv(muted);
}

void MediaPlayerSinkPrivate::setAudioMute(bool muted)
{
  if (!m_playersinkbin)
      return;

  g_object_set(m_playersinkbin, "audio-mute", muted, NULL);
}

bool MediaPlayerSinkPrivate::getVideoMute(void)
{
	bool result = true;

	if (!m_playersinkbin)
	{
		result = false;
	}
	else
	{
		g_object_get(m_playersinkbin, "video-mute", &result, NULL);
	}

	return result;
}

void MediaPlayerSinkPrivate::showLastFrame(bool enable)
{
    m_show_last_frame = enable;
    if (m_playersinkbin) {
        g_object_set(m_playersinkbin, "show-last-frame", m_show_last_frame, NULL);
    }
}


//-- MediaPlayerSink ----------------------------------------------------------
#define IMPL static_cast<MediaPlayerSinkPrivate*>(getPrivateSinkImpl())

RMFResult MediaPlayerSink::setVideoRectangle(unsigned x, unsigned y, unsigned w, unsigned h, bool apply_now)
{
    return IMPL->setVideoRectangle(x, y, w, h, apply_now);
}

RMFResult MediaPlayerSink::setVideoRectangle(unsigned x, unsigned y, unsigned w, unsigned h, unsigned steps, unsigned max_w, unsigned max_h, bool blocking, bool apply_now)
{
    return IMPL->setVideoRectangle(x, y, w, h, steps, max_w, max_h, blocking, apply_now);
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

void MediaPlayerSink::setPipOutput(bool pip)
{
    IMPL->setPipOutput(pip);
}

bool MediaPlayerSink::getPipOutput() const
{
    return IMPL->getPipOutput();
}

void MediaPlayerSink::setZOrder(int order)
{
    IMPL->setZOrder(order);
}
void MediaPlayerSink::showLastFrame(bool enable)
{
    IMPL->showLastFrame(enable);
}

void MediaPlayerSink::setVideoMute(bool muted)
{
	IMPL->setVideoMute(muted);
}

bool MediaPlayerSink::getVideoMute(void)
{
	return IMPL->getVideoMute();
}

void MediaPlayerSink::setAudioMute(bool muted)
{
	IMPL->setAudioMute(muted);
}

void MediaPlayerSink::setPrimeDecode(bool enable)
{
        IMPL->setPrimeDecode(enable);
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

void MediaPlayerSink::setEISSDataCallback(callback_t cb, void* data)
{
    return IMPL->setEISSDataCallback(cb, data);
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

void MediaPlayerSink::setVideoPlayingCallback(callback_t cb, void* data)
{
    return IMPL->setVideoPlayingCallback(cb, data);
}

void MediaPlayerSink::setAudioPlayingCallback(callback_t cb, void* data)
{
    return IMPL->setAudioPlayingCallback(cb, data);
}

void MediaPlayerSink::setMediaWarningCallback(callback_t cb, void* data)
{
    return IMPL->setMediaWarningCallback(cb, data);
}

void MediaPlayerSink::setPmtUpdateCallback(callback_t cb, void* data)
{
    return IMPL->setPmtUpdateCallback(cb, data);
}

void MediaPlayerSink::setLanguageChangeCallback(callback_t cb, void* data)
{
    return IMPL->setLanguageChangeCallback(cb, data);
}

unsigned long MediaPlayerSink::getVideoDecoderHandle() const
{
    return IMPL->getVideoDecoderHandle();
}

const char *MediaPlayerSink::getAudioLanguages()
{
    return IMPL->getAudioLanguages();
}

const char *MediaPlayerSink::getAudioTracks()
{
    return IMPL->getAudioTracks();
}

const char *MediaPlayerSink::getAudioTrackSelected()
{
    return IMPL->getAudioTrackSelected();
}

const char *MediaPlayerSink::getCCDescriptor()
{
    return IMPL->getCCDescriptor();
}

const char *MediaPlayerSink::getEISSData()
{
    return IMPL->getEISSData();
}

void MediaPlayerSink::setAudioLanguage (const char* pAudioLang)
{
    IMPL->setAudioLanguage (pAudioLang);
}

void MediaPlayerSink::setAuxiliaryAudioLang (const char* pAudioLang)
{
    IMPL->setAuxiliaryAudioLang (pAudioLang);
}

void MediaPlayerSink::setPreferredAudioTrack (const char* pAudioTrackPreference)
{
    IMPL->setPreferredAudioTrack (pAudioTrackPreference);
}

void MediaPlayerSink::getSubtitles (char* value, int max_size)
{
  IMPL->getSubtitles(value, max_size);
}

void MediaPlayerSink::getTeletext (char* value, int max_size)
{
  IMPL->getTeletext(value, max_size);
}

void MediaPlayerSink::setSubtitlesTrack (const char* channel_path, const char* value)
{
  IMPL->setSubtitlesTrack(channel_path, value);
}

void MediaPlayerSink::setTeletextTrack (const char* channel_path, const char* value)
{
  IMPL->setTeletextTrack(channel_path, value);
}

void MediaPlayerSink::getSubtitlesTrack (char* value, int max_size)
{
  IMPL->getSubtitlesTrack(value, max_size);
}

void MediaPlayerSink::getTeletextTrack (char* value, int max_size)
{
  IMPL->getTeletextTrack(value, max_size);
}

void MediaPlayerSink::resetTtxtSubs (void)
{
  IMPL->resetTtxtSubs();
}


void MediaPlayerSink::setVideoZoom (unsigned short zoomVal)
{
    IMPL->setVideoZoom (zoomVal);
}

void MediaPlayerSink::setEissFilterStatus (bool eissStatus)
{
    IMPL->setEissFilterStatus (eissStatus);
}

void MediaPlayerSink::setVideoKeySlot (const char *keySlot)
{
    IMPL->setVideoKeySlot (keySlot);
}

void MediaPlayerSink::setAudioKeySlot (const char *keySlot)
{
    IMPL->setAudioKeySlot (keySlot);
}

void MediaPlayerSinkPrivate::deleteVideoKeySlot (void)
{
    if (m_playersinkbin)
            g_object_set(m_playersinkbin, "video-keyslot", NULL, NULL);
}

void MediaPlayerSinkPrivate::deleteAudioKeySlot (void)
{
    if (m_playersinkbin)
            g_object_set(m_playersinkbin, "audio-keyslot", NULL, NULL);

}

const char * MediaPlayerSinkPrivate::getPreferredAudioLanguage(void)
{
    GValue value = G_VALUE_INIT;

    g_value_init(&value, G_TYPE_STRING);

    g_object_get_property(G_OBJECT(m_playersinkbin), "preferred-language", &value);

    const char* val_str = (const char *) g_value_get_string (&value);

    if (val_str) {
        m_preferredAudioLang = val_str;
    } else {
        m_preferredAudioLang = "";
    }
    g_value_unset(&value);

    return m_preferredAudioLang.c_str();
}

void MediaPlayerSink::deleteVideoKeySlot (void)
{
    IMPL->deleteVideoKeySlot ();
}

void MediaPlayerSink::deleteAudioKeySlot (void)
{
    IMPL->deleteAudioKeySlot ();
}

const char *MediaPlayerSink::getPreferredAudioLanguage()
{
    return IMPL->getPreferredAudioLanguage();
}

int MediaPlayerSink::getVideoPid()
{
    return IMPL->getVideoPid();
}

int MediaPlayerSink::getAudioPid()
{
    return IMPL->getAudioPid();
}

void MediaPlayerSink::addInbandFilterFunc(unsigned int funcPointer,unsigned int cbData)
{
    IMPL->addInbandFilterFunc(funcPointer, cbData);
}

const char* MediaPlayerSink::getMediaWarningString(void)
{
    return IMPL->getMediaWarningString();
}

short MediaPlayerSink::getMediaBufferSize(void)
{
    return IMPL->getMediaBufferSize();
}

void MediaPlayerSink::setNetWorkBufferSize(short bufferSize)
{
    IMPL->setNetWorkBufferSize (bufferSize);
    return;
}

// virtual
RMFMediaSinkPrivate* MediaPlayerSink::createPrivateSinkImpl()
{
    return new MediaPlayerSinkPrivate(this);
}

// virtual
RMFResult MediaPlayerSink::close()
{
    return RMF_RESULT_SUCCESS;
}

/**
 * @brief This function returns the channel type that is playing now.
 *
 * @return Returns the channel type.
 */
RMFCurrentAssetType MediaPlayerSink::getCurrentAssetType()
{
    return IMPL->getCurrentAssetType();
}

