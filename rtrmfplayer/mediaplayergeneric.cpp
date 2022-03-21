/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2018 RDK Management
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
#include "mediaplayergeneric.h"

#include <fstream>
#include <limits>
#include <mutex>
#include <sstream>
#include <streambuf>
#include <string>
#include <unordered_map>

#include <jansson.h>
#include <math.h>

#include "logger.h"

namespace {

const char* kMediaConfigFilePath = "/etc/webkit_media_player_settings.json";
std::unordered_map<std::string, std::string> gMediaConfig;
bool gFlagGetVideoSinkFromPlaybin = true;

bool loadMediaConfig(const char* configPath)
{
  if (!gMediaConfig.empty())
    return true;

  std::ifstream configStream(configPath);
  if (!configStream)
    return false;

  std::string configBuffer((std::istreambuf_iterator<char>(configStream)),
                           std::istreambuf_iterator<char>());

  json_t *root;
  json_error_t error;
  root = json_loads(configBuffer.c_str(), 0, &error);
  if(!root)
    return false;

  if(!json_is_object(root))
  {
    json_decref(root);
    return false;
  }

  LOG_INFO("Media config:");
  const char *key;
  json_t *value;
  json_object_foreach(root, key, value)
  {
    if (!json_is_string(value))
      continue;
    LOG_INFO("\t%s=%s", key, json_string_value(value));
    gMediaConfig[std::string(key)] = json_string_value(value);
  }

  // Set flags
  auto it = gMediaConfig.find("get-video-sink-from-playbin");
  if (it != gMediaConfig.end())
  {
    gFlagGetVideoSinkFromPlaybin = (it->second == "true");
  }
  if (!gFlagGetVideoSinkFromPlaybin)
  {
    it = gMediaConfig.find("video-sink");
    gFlagGetVideoSinkFromPlaybin = (it == gMediaConfig.end()) || it->second.empty();
  }

  json_decref(root);
  return true;
}

void mediaPlayerPrivateElementAddedCallback(GstBin *bin, GstElement *element, gpointer);

void configureElement(GstElement* element, const char* elementName)
{
  const std::string propsName = std::string(elementName).append("-props");
  const auto& it = gMediaConfig.find(propsName);
  if (it == gMediaConfig.end())
    return;

  std::stringstream configStream(it->second);
  std::string configValue;
  while (std::getline(configStream, configValue, ','))
  {
    std::size_t idx = configValue.find("=");
    std::string propName = configValue.substr(0, idx);
    std::string propValue = "";
    if (idx != std::string::npos)
      propValue = configValue.substr(idx+1,std::string::npos);
    LOG_TRACE("set prop for %s: %s=%s", elementName, propName.c_str(), propValue.c_str());
    gst_util_set_object_arg(G_OBJECT(element), propName.c_str(), propValue.c_str());
  }
}

void configureElement(GstElement *element)
{
  if (!element)
    return;
  gchar* elementName = gst_element_get_name(element);
  if (elementName)
  {
    for (int i = strlen(elementName) - 1; i >= 0 && isdigit(elementName[i]); --i) {
      elementName[i] = '\0';
    }
    configureElement(element, elementName);
    g_free(elementName);
  }
  if(GST_IS_BIN(element))
  {
    g_signal_connect (element, "element-added", G_CALLBACK (mediaPlayerPrivateElementAddedCallback), nullptr);
    GValue item = G_VALUE_INIT;
    GstIterator* it = gst_bin_iterate_elements(GST_BIN(element));
    while(GST_ITERATOR_OK == gst_iterator_next(it, &item))
    {
      GstElement *next = GST_ELEMENT(g_value_get_object(&item));
      configureElement(next);
      g_value_reset (&item);
    }
    gst_iterator_free(it);
  }
}

void mediaPlayerPrivateElementAddedCallback(GstBin *bin, GstElement *element, gpointer)
{
  LOG_TRACE("added element=%s",GST_ELEMENT_NAME (element));
  configureElement(element);
}

}

#define NOT_IMPLEMENTED() LOG_WARNING("Not implemented")

void MediaPlayerGeneric::busMessageCallback(GstBus* bus, GstMessage* msg, gpointer data)
{
  MediaPlayerGeneric& self = *static_cast<MediaPlayerGeneric*>(data);
  self.handleBusMessage(bus, msg);
}

MediaPlayerGeneric::MediaPlayerGeneric(MediaPlayerClient* client)
  : m_playerClient(client)
  , m_pipeline(nullptr)
  , m_playerState(MediaPlayer::RMF_PLAYER_EMPTY)
  , m_videoState(MediaPlayer::RMF_VIDEO_BUFFER_HAVENOTHING)
  , m_errorOccured(false)
  , m_seekIsPending(false)
  , m_seekTime(0.f)
{
  static std::once_flag loadConfigFlag;
  std::call_once(loadConfigFlag, [](){
      loadMediaConfig(kMediaConfigFilePath);
  });
  LOG_INFO("Generic video player created");
}

MediaPlayerGeneric::~MediaPlayerGeneric()
{
  LOG_INFO("Generic video player destroying");
  cleanup();
  m_playerClient = nullptr;
  LOG_INFO("Generic video player destroyed");
}

MediaPlayer::RMFPlayerState MediaPlayerGeneric::rmf_playerState() const
{
  return m_playerState;
}

MediaPlayer::RMFVideoBufferState MediaPlayerGeneric::rmf_videoState() const
{
  return m_videoState;
}

bool MediaPlayerGeneric::rmf_load(const std::string &url)
{
  LOG_INFO("LOAD IS CALLED (%s)", url.c_str());

  cleanup();

  m_pipeline = gst_element_factory_make("playbin", nullptr);

  if (!m_pipeline)
  {
    m_errorMsg = "Failed to create playbin";
    loadingFailed(MediaPlayer::RMF_PLAYER_DECODEERROR);
    return false;
  }

  configureElement(pipeline());

  if(!gFlagGetVideoSinkFromPlaybin)
  {
    std::string videoSinkName = gMediaConfig["video-sink"];
    GstElement* videoSink = gst_element_factory_make(videoSinkName.c_str(), nullptr);
    if (videoSink)
    {
      g_object_set(pipeline(), "video-sink", videoSink, NULL);
      configureElement(videoSink);
      gst_object_unref(videoSink);
    }
    else
    {
      LOG_WARNING("Failed to create '%s', fallback to playbin defaults", videoSinkName.c_str());
    }
  }

  m_url = url;
  g_object_set(pipeline(), "uri", m_url.c_str(),  nullptr);

  GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline()));
  g_signal_connect(bus, "message", G_CALLBACK(busMessageCallback), this);
  gst_bus_add_signal_watch(bus);
  gst_object_unref(bus);

  updateStates();

  LOG_INFO("LOAD FINISHED");
  return true;
}

void MediaPlayerGeneric::rmf_play()
{
  LOG_INFO("PLAY IS CALLED");

  bool rc = changePipelineState(GST_STATE_PLAYING);
  if (!rc) {
    LOG_ERROR("Play failed for some unknown reason..");
    loadingFailed(MediaPlayer::RMF_PLAYER_EMPTY);
  }

  LOG_INFO("PLAY");
}

void MediaPlayerGeneric::rmf_stop()
{
  LOG_INFO("STOP IS CALLED");

  cleanup();

  LOG_INFO("STOP");
}

void MediaPlayerGeneric::rmf_pause()
{
  LOG_INFO("PAUSE IS CALLED");

  changePipelineState(GST_STATE_PAUSED);

  LOG_INFO("PAUSE");
}

bool MediaPlayerGeneric::rmf_canItPlay() const
{
  return m_pipeline != nullptr && !m_url.empty();
}

bool MediaPlayerGeneric::rmf_isPaused() const
{
  GstState state = GST_STATE_NULL;
  gst_element_get_state(pipeline(), &state, 0, 0);
  return state <= GST_STATE_PAUSED;
}

float MediaPlayerGeneric::rmf_getDuration() const
{
  gint64 duration = GST_CLOCK_TIME_NONE;
  gboolean rc =
    gst_element_query_duration(pipeline(), GST_FORMAT_TIME, &duration);
  if (rc && static_cast<GstClockTime>(duration) != GST_CLOCK_TIME_NONE)
    return static_cast<float>(duration) / GST_SECOND;
  return std::numeric_limits<float>::infinity();;
}

float MediaPlayerGeneric::rmf_getCurrentTime() const
{
  gint64 position = GST_CLOCK_TIME_NONE;
  GstQuery *query= gst_query_new_position(GST_FORMAT_TIME);
  if (gst_element_query(pipeline(), query))
    gst_query_parse_position(query, 0, &position);
  gst_query_unref(query);
  if (static_cast<GstClockTime>(position) != GST_CLOCK_TIME_NONE)
    return static_cast<float>(position) / GST_SECOND;
  return 0.f;
}

std::string MediaPlayerGeneric::rmf_getMediaErrorMessage() const
{
  return m_errorMsg;
}

void MediaPlayerGeneric::rmf_setRate(float speed)
{
  NOT_IMPLEMENTED();
}
float MediaPlayerGeneric::rmf_getRate() const
{
  return 1.f;
}

void MediaPlayerGeneric::rmf_setVolume(float volume)
{
    gdouble vol = static_cast<gdouble>(volume);
    g_object_set(pipeline(), "volume", vol, NULL);
}

float MediaPlayerGeneric::rmf_getVolume() const
{
    gdouble volume;
    g_object_get(pipeline(), "volume", &volume, NULL);
    return static_cast<float>(volume);
}

void MediaPlayerGeneric::rmf_setMute(bool mute)
{
  NOT_IMPLEMENTED();
}
bool MediaPlayerGeneric::rmf_isMuted() const
{
  return false;
}

void MediaPlayerGeneric::rmf_seekToLivePosition()
{
  rmf_seek(0.f);
}
void MediaPlayerGeneric::rmf_seekToStartPosition()
{
  rmf_seek(0.f);
}

static GstClockTime convertPositionToGstClockTime(float time)
{
    // Extract the integer part of the time (seconds) and the fractional part (microseconds). Attempt to
    // round the microseconds so no floating point precision is lost and we can perform an accurate seek.
    double seconds;
    float microSeconds = modf(time, &seconds) * 1000000;
    GTimeVal timeValue;
    timeValue.tv_sec = static_cast<glong>(seconds);
    timeValue.tv_usec = static_cast<glong>(roundf(microSeconds / 10000) * 10000);
    return GST_TIMEVAL_TO_TIME(timeValue);
}

void MediaPlayerGeneric::rmf_seek(float time)
{
    GstState state, pending;
    gint64 startTime, endTime;

    if (!pipeline())
        return;

    if (m_errorOccured)
        return;

    LOG_INFO("[Seek] seek attempt to %f secs", time);

    // Avoid useless seeking.
    float current = rmf_getCurrentTime();
    if (time == current)
        return;

    LOG_INFO("HTML5 video: Seeking from %f to %f seconds", current, time);

    GstClockTime clockTime = convertPositionToGstClockTime(time);

    GstStateChangeReturn ret = gst_element_get_state(pipeline(), &state, &pending, 250 * GST_NSECOND);
    LOG_INFO ("state: %s, pending: %s, ret = %s", gst_element_state_get_name(state), gst_element_state_get_name(pending), gst_element_state_change_return_get_name(ret));

    if (ret == GST_STATE_CHANGE_ASYNC || state < GST_STATE_PAUSED) {
        m_seekIsPending = true;
    } else {
        m_seekIsPending = false;

        startTime = clockTime;
        endTime = GST_CLOCK_TIME_NONE;

        /* Call Seek Now */
        if (!gst_element_seek(pipeline(), 1.0, GST_FORMAT_TIME, static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE), GST_SEEK_TYPE_SET, startTime, GST_SEEK_TYPE_SET, endTime)) {
            LOG_WARNING("[Seek] seeking to %f failed", time);
        }

        /* Set it to Play */
        changePipelineState(GST_STATE_PLAYING);
    }

    /* Update the seek time */
    m_seekTime = time;
}

bool MediaPlayerGeneric::rmf_isSeeking() const
{
  return false;
}
unsigned long MediaPlayerGeneric::rmf_getCCDecoderHandle() const
{
  NOT_IMPLEMENTED();
  return 0;
}
std::string MediaPlayerGeneric::rmf_getAudioLanguages() const
{
  NOT_IMPLEMENTED();
  return "eng";
}
void MediaPlayerGeneric::rmf_setAudioLanguage(const std::string &)
{
  NOT_IMPLEMENTED();
}
void MediaPlayerGeneric::rmf_setAuxiliaryAudioLang(const std::string &)
{
  NOT_IMPLEMENTED();
}
void MediaPlayerGeneric::rmf_setEissFilterStatus(bool status)
{
  NOT_IMPLEMENTED();
}
void MediaPlayerGeneric::rmf_setVideoZoom(unsigned short zoomVal)
{
  NOT_IMPLEMENTED();
}
void MediaPlayerGeneric::rmf_setVideoBufferLength(float bufferLength)
{
//  NOT_IMPLEMENTED();
}
void MediaPlayerGeneric::rmf_setInProgressRecording(bool isInProgress)
{
  NOT_IMPLEMENTED();
}
bool MediaPlayerGeneric::rmf_isItInProgressRecording() const
{
  NOT_IMPLEMENTED();
  return false;
}
void MediaPlayerGeneric::rmf_changeSpeed(float speed, short overshootTime)
{
  NOT_IMPLEMENTED();
}
std::string MediaPlayerGeneric::rmf_getMediaWarnDescription() const
{
  NOT_IMPLEMENTED();
  return "";
}
int MediaPlayerGeneric::rmf_getMediaWarnData() const
{
  NOT_IMPLEMENTED();
  return 0;
}
std::string MediaPlayerGeneric::rmf_getAvailableAudioTracks() const
{
  NOT_IMPLEMENTED();
  return "eng";
}
std::string MediaPlayerGeneric::rmf_getCaptionDescriptor() const
{
  NOT_IMPLEMENTED();
  return "";
}
std::string MediaPlayerGeneric::rmf_getEISSDataBuffer() const
{
  NOT_IMPLEMENTED();
  return "";
}
void MediaPlayerGeneric::rmf_setNetworkBufferSize(int bufferSize)
{
  NOT_IMPLEMENTED();
}
int MediaPlayerGeneric::rmf_getNetworkBufferSize() const
{
  NOT_IMPLEMENTED();
  return 0;
}
void MediaPlayerGeneric::rmf_setVideoRectangle(unsigned x, unsigned y, unsigned w, unsigned h)
{
  IntRect temp(x, y, w, h);
  if (m_lastKnownRect == temp)
    return;

  m_lastKnownRect = temp;

  if (!pipeline())
    return;

  GstElement* videoSink;
  g_object_get(pipeline(), "video-sink", &videoSink, nullptr);
  if (!videoSink)
    return;

  gchar* rectStr = g_strdup_printf("%d,%d,%d,%d", x, y, w, h);
  if (!rectStr) {
      gst_object_unref(videoSink);
      return;
  }

  g_object_set(videoSink, "rectangle", rectStr, nullptr);
  gst_object_unref(videoSink);
  g_free(rectStr);
}

void MediaPlayerGeneric::handleBusMessage(GstBus* bus, GstMessage* msg)
{
  bool isPlayerPipelineMessage = GST_MESSAGE_SRC(msg) == reinterpret_cast<GstObject*>(pipeline());

  switch (GST_MESSAGE_TYPE (msg))
  {
    case GST_MESSAGE_EOS: {
      endOfStream();
      break;
    }
    case GST_MESSAGE_STATE_CHANGED:
    case GST_MESSAGE_ASYNC_DONE: {
      if (isPlayerPipelineMessage)
        updateStates();
      break;
    }
    case GST_MESSAGE_ERROR: {
      GError *err = nullptr;
      gchar *dbg = nullptr;
      gst_message_parse_error (msg, &err, &dbg);
      if (err) {
        notifyError(err->message);
        g_error_free (err);
      } else {
        notifyError(nullptr);
      }
      if (dbg) {
        LOG_ERROR("[Debug details: %s]", dbg);
        g_free (dbg);
      }
      break;
    }
    default:
      LOG_TRACE("Unhandled message type: %s", GST_MESSAGE_TYPE_NAME(msg));
      break;
  }
}

void MediaPlayerGeneric::updateStates()
{
  if (!pipeline())
    return;

  if (m_errorOccured)
    return;

  MediaPlayer::RMFPlayerState oldPlayerState = m_playerState;
  MediaPlayer::RMFVideoBufferState oldVideoState = m_videoState;

  GstState state, pending;
  GstStateChangeReturn ret =
    gst_element_get_state(pipeline(), &state, &pending, 250 * GST_NSECOND);

  LOG_VERBOSE("updateStates(): state: %s, pending: %s, ret = %s",
              gst_element_state_get_name(state),
              gst_element_state_get_name(pending),
              gst_element_state_change_return_get_name(ret));

  switch (ret)
  {
    case GST_STATE_CHANGE_SUCCESS:
    {
      if (state == GST_STATE_READY) {
        m_videoState = MediaPlayer::RMF_VIDEO_BUFFER_HAVEMETADATA;
        m_playerState = MediaPlayer::RMF_PLAYER_EMPTY;
      } else if (state == GST_STATE_PAUSED) {
        m_videoState = MediaPlayer::RMF_VIDEO_BUFFER_HAVECURRENTDATA;
        m_playerState = MediaPlayer::RMF_PLAYER_LOADING;
      } else if (state == GST_STATE_PLAYING) {
        m_videoState = MediaPlayer::RMF_VIDEO_BUFFER_HAVEENOUGHDATA;
        m_playerState = MediaPlayer::RMF_PLAYER_LOADED;
      }
      break;
    }
    case GST_STATE_CHANGE_NO_PREROLL:
    {
      if (state == GST_STATE_READY) {
        m_videoState = MediaPlayer::RMF_VIDEO_BUFFER_HAVENOTHING;
      } else if (state == GST_STATE_PAUSED) {
        m_videoState = MediaPlayer::RMF_VIDEO_BUFFER_HAVEENOUGHDATA;
      }
      m_playerState = MediaPlayer::RMF_PLAYER_LOADING;
      break;
    }
    default:
      break;
  }

  // update video rect on playback start
  if (m_playerState != oldPlayerState && state == GST_STATE_PLAYING)
  {
    IntRect temp = m_lastKnownRect;
    m_lastKnownRect = IntRect();
    rmf_setVideoRectangle(temp.x(),temp.y(),temp.width(),temp.height());
  }

  if (m_playerState != oldPlayerState)
  {
    LOG_INFO("Player State Changed from %u to %u",
             oldPlayerState, m_playerState);
    m_playerClient->playerStateChanged();
  }
  if (m_videoState != oldVideoState)
  {
    LOG_INFO("Video State Changed from %u to %u",
             oldVideoState, m_videoState);
    m_playerClient->videoStateChanged();
  }

  if ((ret == GST_STATE_CHANGE_SUCCESS) && (state >= GST_STATE_PAUSED) && m_seekIsPending) {
      LOG_INFO ("[Seek] committing pending seek to %f", m_seekTime);
      m_seekIsPending = false;
      rmf_seek(m_seekTime);
  }
}

void MediaPlayerGeneric::endOfStream()
{
  LOG_TRACE("mediaPlaybackCompleted");
  m_playerClient->mediaPlaybackCompleted();
}

void MediaPlayerGeneric::notifyError(const char *message)
{
  std::string tmp = "130:ErrorCode:Unknown Error";
  if (message)
    tmp = std::string(message);
  m_errorMsg = tmp;
  LOG_ERROR("error message='%s'", m_errorMsg.c_str());
  loadingFailed(MediaPlayer::RMF_PLAYER_DECODEERROR);
}

void MediaPlayerGeneric::loadingFailed(MediaPlayer::RMFPlayerState error)
{
  LOG_ERROR("player state=%s", StateString(error));
  m_errorOccured = true;
  if (m_playerState != error) {
    m_playerState = error;
    m_playerClient->playerStateChanged();
  }
  if (m_videoState != MediaPlayer::RMF_VIDEO_BUFFER_HAVENOTHING) {
    m_videoState = MediaPlayer::RMF_VIDEO_BUFFER_HAVENOTHING;
    m_playerClient->videoStateChanged();
  }
}

GstElement* MediaPlayerGeneric::pipeline() const
{
  return m_pipeline;
}

bool MediaPlayerGeneric::changePipelineState(GstState newState)
{
  GstState currentState, pending;

  gst_element_get_state(pipeline(), &currentState, &pending, 0);
  if (currentState == newState || pending == newState) {
    LOG_INFO("Rejected state change to %s from %s with %s pending",
             gst_element_state_get_name(newState),
             gst_element_state_get_name(currentState),
             gst_element_state_get_name(pending));
    return true;
  }

  GstStateChangeReturn setStateResult = gst_element_set_state(pipeline(), newState);
  if (setStateResult == GST_STATE_CHANGE_FAILURE) {
    LOG_ERROR("Failed to change state to %s from %s with %s pending",
              gst_element_state_get_name(newState),
              gst_element_state_get_name(currentState),
              gst_element_state_get_name(pending));
    return false;
  }
  return true;
}

void MediaPlayerGeneric::cleanup()
{
  if (!m_pipeline)
    return;

  GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline()));
  gst_bus_remove_signal_watch(bus);
  gst_object_unref(bus);

  gst_element_set_state(pipeline(), GST_STATE_NULL);
  gst_object_unref(m_pipeline);

  m_pipeline = nullptr;
  m_playerState = MediaPlayer::RMF_PLAYER_EMPTY;
  m_videoState = MediaPlayer::RMF_VIDEO_BUFFER_HAVENOTHING;
  m_url.clear();
  m_errorMsg.clear();
  m_errorOccured = false;
  m_lastKnownRect = IntRect();
}
