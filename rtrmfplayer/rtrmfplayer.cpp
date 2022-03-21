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
#include "rtrmfplayer.h"

#include <glib.h>
#include <limits>
#include <cmath>
#include <unistd.h> // for sleep

#include "logger.h"
#include "mediaplayerdlna.h"
#include "mediaplayergeneric.h"

#define TO_MS(x) (uint32_t)( (x) / 1000)

struct ReleaseOnScopeEnd {
  rtObject& obj;
  ReleaseOnScopeEnd(rtObject& o) : obj(o) {
  }
  ~ReleaseOnScopeEnd() {
    obj.Release();
  }
};

#define CALL_ON_MAIN_THREAD(body)                             \
  do {                                                        \
    this->AddRef();                                           \
    g_timeout_add_full(G_PRIORITY_HIGH, 0,                    \
                       [](gpointer data) -> gboolean {        \
      rtRMFPlayer &self = *static_cast<rtRMFPlayer*>(data);   \
      ReleaseOnScopeEnd releaseOnScopeEnd(self);              \
      body                                                    \
      return G_SOURCE_REMOVE;                                 \
    }, this, 0);                                              \
  } while(0)                                                  \

#define TUNE_LOG_MAX_SIZE 100

namespace {

struct OnReadyEvent: public Event
{
  OnReadyEvent() : Event("onReady") { }
};

struct OnLoadedEvent: public Event
{
  OnLoadedEvent() : Event("onLoaded") { }
};

struct OnPlayingEvent: public Event
{
  OnPlayingEvent() : Event("onPlaying") { }
};

struct OnPausedEvent: public Event
{
  OnPausedEvent() : Event("onPaused") { }
};

struct OnCompleteEvent: public Event
{
  OnCompleteEvent() : Event("onComplete") { }
};

struct OnTimeUpdateEvent: public Event
{
  // Special case to ensure that caller receives the actual time
  struct rtTimeUpdate: public rtMapObject
  {
    rtTimeUpdate(rtRMFPlayer* player)
      : m_player(player)
    {
      rtValue nameVal = "onTimeUpdate";
      Set("name", &nameVal);
      rtValue currentTimeVal = -1.0f;
      Set("currentTime", &currentTimeVal);
    }
    rtError Get(const char* name, rtValue* value) const override
    {
      if (!value)
        return RT_FAIL;
      if (!strcmp(name, "currentTime"))
      {
        float time = -1.f;
        rtError rc = m_player->currentTime(time);
        *value = time;
        return rc;
      }
      return rtMapObject::Get(name, value);
    }
    rtRefT<rtRMFPlayer> m_player;
  };
  OnTimeUpdateEvent(rtRMFPlayer* player) : Event(new rtTimeUpdate(player)) { }
};

struct OnDurationChangeEvent: public Event
{
  OnDurationChangeEvent(float duration) : Event("onDurationChange")
  {
    m_object.set("duration", duration);
  }
};

struct OnRateChangeEvent: public Event
{
  OnRateChangeEvent(double rate) : Event("onRateChange")
  {
    m_object.set("rate", rate);
  }
};

struct OnMediaFrameReceivedEvent: public Event
{
  OnMediaFrameReceivedEvent() : Event("onMediaFrameReceived") { }
};

struct OnMediaWarningEvent: public Event
{
  OnMediaWarningEvent(uint32_t code, const std::string& description) : Event("onMediaWarning")
  {
    m_object.set("code", code);
    m_object.set("description", rtString(description.c_str()));
  }
};

struct OnEISSDataReceivedEvent: public Event
{
  OnEISSDataReceivedEvent(const std::string& eissData) : Event("onEISSDataReceived")
  {
    m_object.set("rmfEISSDataBuffer", rtString(eissData.c_str()));
  }
};

struct OnMediaErrorEvent: public Event
{
  OnMediaErrorEvent(uint32_t code, const std::string& description) : Event("onMediaError")
  {
    m_object.set("code", code);
    m_object.set("description", rtString(description.c_str()));
  }
};

struct OnVidHandleEvent: public Event
{
  OnVidHandleEvent(uint32_t vDecoderHandle) : Event("onVidHandle")
  {
    m_object.set("decoderHandle", vDecoderHandle);
  }
};

struct OnVideoMetadataEvent: public Event
{
  OnVideoMetadataEvent(const char* lang, const char* ccDescriptor) : Event("onVideoMetadata")
  {
    m_object.set("audioLanguages", lang);
    m_object.set("ccDescriptor", ccDescriptor);
  }
};

struct OnMetricLogEvent: public Event
{
  OnMetricLogEvent(const char* message) : Event("onMetricLog")
  {
    m_object.set("message", message);
  }
};

const guint kProgressMonitorTimeoutMs = 250;
const guint kRefreshCachedTimeThrottleMs = 100;

}  // namespace

rtDefineObject    (rtRMFPlayer, rtObject);
rtDefineProperty  (rtRMFPlayer, src);
rtDefineProperty  (rtRMFPlayer, audioLanguage);
rtDefineProperty  (rtRMFPlayer, auxiliaryAudioLang);
rtDefineProperty  (rtRMFPlayer, currentTime);
rtDefineProperty  (rtRMFPlayer, playbackRate);
rtDefineProperty  (rtRMFPlayer, volume);
rtDefineProperty  (rtRMFPlayer, muted);
rtDefineProperty  (rtRMFPlayer, jumpToLive)
rtDefineProperty  (rtRMFPlayer, isInProgressRecording);;
rtDefineProperty  (rtRMFPlayer, videoZoom);
rtDefineProperty  (rtRMFPlayer, networkBufferSize);
rtDefineProperty  (rtRMFPlayer, videoBufferLength);
rtDefineProperty  (rtRMFPlayer, eissFilterStatus);
rtDefineProperty  (rtRMFPlayer, loadStartTime);
rtDefineMethod    (rtRMFPlayer, setVideoRectangle);
rtDefineMethod    (rtRMFPlayer, load);
rtDefineMethod    (rtRMFPlayer, play);
rtDefineMethod    (rtRMFPlayer, pause);
rtDefineMethod    (rtRMFPlayer, stop);
rtDefineMethod    (rtRMFPlayer, destroy);
rtDefineMethod    (rtRMFPlayer, changeSpeed);
rtDefineMethod    (rtRMFPlayer, setListener);
rtDefineMethod    (rtRMFPlayer, delListener);

rtRMFPlayer::rtRMFPlayer()
  : m_currentTime(0.f)
  , m_lastReportedCurrentTime(0.f)
  , m_duration(0.f)
  , m_lastReportedDuration(0.f)
  , m_seekTime(std::numeric_limits<float>::quiet_NaN())
  , m_playbackRate(1.f)
  , m_lastReportedPlaybackRate(0.f)
  , m_volume(1.f)
  , m_videoBufferLength(0.f)
  , m_isLoaded(false)
  , m_isMuted(false)
  , m_eissFilterStatus(false)
  , m_jumpToLive(false)
  , m_isInProgressRecording(false)
  , m_isPaused(true)
  , m_videoZoom(0)
  , m_networkBufferSize(0)
  , m_overshootTime(0)
  , m_playerState(MediaPlayer::RMF_PLAYER_EMPTY)
  , m_videoState(MediaPlayer::RMF_VIDEO_BUFFER_HAVENOTHING)
  , m_videoStateMaximum(MediaPlayer::RMF_VIDEO_BUFFER_HAVENOTHING)
  , m_playbackProgressMonitorTag(0)
  , m_lastRefreshTime(0) {
  m_mediaPlayer.reset(NULL);
}

rtRMFPlayer::~rtRMFPlayer() {
    LOG_WARNING("Destroying rtRMFPlayer (should almost NEVER happen!)");
}

void rtRMFPlayer::onInit() {
  LOG_INFO("rmf player started");
}

// rtRemote properties
rtError rtRMFPlayer::currentSrc(rtString& s) const {
  s = m_currentSrc;
  return RT_OK;
}
rtError rtRMFPlayer::setCurrentSrc(rtString const& s) {
  m_setSrcTime = g_get_monotonic_time();

  m_currentSrc = s;
  CALL_ON_MAIN_THREAD (
    bool isCurrentSrcEmpty = false;
    {
      isCurrentSrcEmpty = self.m_currentSrc.isEmpty();
    }
    LOG_INFO("currentSrc='%s'", self.m_currentSrc.cString());
    if (isCurrentSrcEmpty)
      self.doStop();
    else
      self.doLoad();
  );
  return RT_OK;
}

rtError rtRMFPlayer::audioLanguage(rtString& s) const {
  s = m_audioLanguage;
  return RT_OK;
}
rtError rtRMFPlayer::setAudioLanguage(rtString const& s) {
  m_audioLanguage = s;
  CALL_ON_MAIN_THREAD (
    self.doSetAudioLanguage();
  );
  return RT_OK;
}

rtError rtRMFPlayer::auxiliaryAudioLang(rtString& s) const {
  s = m_auxiliaryAudioLang;
  return RT_OK;
}
rtError rtRMFPlayer::setAuxiliaryAudioLang(rtString const& s) {
  m_auxiliaryAudioLang = s;
  CALL_ON_MAIN_THREAD (
    self.doSetAuxiliaryAudioLang();
  );
  return RT_OK;
}

rtError rtRMFPlayer::currentTime(float& t) const {
  const_cast<rtRMFPlayer*>(this)->refreshCachedCurrentTime();
  t = m_currentTime;
  return RT_OK;
}
rtError rtRMFPlayer::setCurrentTime(float const& t) {
  m_seekTime = t;
  CALL_ON_MAIN_THREAD (
    self.doSeek();
  );
  return RT_OK;
}

rtError rtRMFPlayer::playbackRate(float& t) const {
  t = m_lastReportedPlaybackRate;
  return RT_OK;
}
rtError rtRMFPlayer::setPlaybackRate(float const& t) {
  m_playbackRate = t;
  CALL_ON_MAIN_THREAD (
    self.doSetRate();
  );
  return RT_OK;
}

rtError rtRMFPlayer::volume(float& t) const {
  t = m_volume;
  return RT_OK;
}
rtError rtRMFPlayer::setVolume(float const& t) {
  m_volume = t;
  CALL_ON_MAIN_THREAD (
    self.doSetVolume();
  );
  return RT_OK;
}

rtError rtRMFPlayer::muted(rtString& t) const {
  t = m_isMuted ? rtString("true") : rtString("false");
  return RT_OK;
}

rtError rtRMFPlayer::setMuted(rtString const& t) {
  m_isMuted = t.compare("true") == 0;
  CALL_ON_MAIN_THREAD (
    self.doSetMuted();
  );
  return RT_OK;
}

rtError rtRMFPlayer::jumpToLive(rtString& t) const {
  t = m_jumpToLive ? rtString("true") : rtString("false");
  return RT_OK;
}
rtError rtRMFPlayer::setJumpToLive(rtString const& t) {
  m_jumpToLive = t.compare("true") == 0;
  if (m_jumpToLive) {
    m_playbackRate = 1.f;
    CALL_ON_MAIN_THREAD (
      self.doJumpToLive();
    );
  }
  return RT_OK;
}

rtError rtRMFPlayer::isInProgressRecording(rtString& t) const {
  t = m_isInProgressRecording ? rtString("true") : rtString("false");
  return RT_OK;
}
rtError rtRMFPlayer::setIsInProgressRecording(rtString const& t) {
  m_isInProgressRecording = t.compare("true") == 0;
  CALL_ON_MAIN_THREAD (
    self.doSetIsInProgressRecording();
  );
  return RT_OK;
}

rtError rtRMFPlayer::videoZoom(int32_t& t) const {
  t = m_videoZoom;
  return RT_OK;
}

rtError rtRMFPlayer::setVideoZoom(int32_t const& t) {
  m_videoZoom = t;
  CALL_ON_MAIN_THREAD (
    self.doSetVideoZoom();
  );
  return RT_OK;
}

rtError rtRMFPlayer::networkBufferSize(int32_t& t) const {
  t = m_networkBufferSize;
  return RT_OK;
}

rtError rtRMFPlayer::setNetworkBufferSize(int32_t const& t) {
  m_networkBufferSize = t;
  CALL_ON_MAIN_THREAD (
    self.doNetworkBufferSize();
  );
  return RT_OK;
}

rtError rtRMFPlayer::videoBufferLength(float& t) const {
  t = m_videoBufferLength;
  return RT_OK;
}
rtError rtRMFPlayer::setVideoBufferLength(float const& t) {
  if (m_videoBufferLength != t) {
    m_videoBufferLength = t;
    CALL_ON_MAIN_THREAD (
      self.doSetVideoBufferLength();
    );
  }
  return RT_OK;
}

rtError rtRMFPlayer::eissFilterStatus(rtString& t) const {
  t = m_eissFilterStatus ? rtString("true") : rtString("false");
  return RT_OK;
}

rtError rtRMFPlayer::setEissFilterStatus(rtString const& t) {
  m_eissFilterStatus = t.compare("true") == 0;
  CALL_ON_MAIN_THREAD (
    self.doSetEISSFilterStatus();
  );
  return RT_OK;
}

rtError rtRMFPlayer::loadStartTime(int64_t& t) const {
  t = m_loadStartTime;
  return RT_OK;
}

rtError rtRMFPlayer::setLoadStartTime(int64_t const& t) {
  m_loadStartTime = t;
  return RT_OK;
}

// rtRemote methods
rtError rtRMFPlayer::setVideoRectangle(int32_t x, int32_t y, int32_t w, int32_t h) {
  m_videoRect = IntRect(x,y,w,h);
  CALL_ON_MAIN_THREAD (
    self.doSetVideoRectangle();
  );
  return RT_OK;
}

rtError rtRMFPlayer::load() {
  // CALL_ON_MAIN_THREAD (
  //   self.doLoad();
  // );
  return RT_OK;
}

rtError rtRMFPlayer::play() {
  m_playStartTime = g_get_monotonic_time();
  CALL_ON_MAIN_THREAD (
    self.doPlay();
  );
  return RT_OK;
}

rtError rtRMFPlayer::pause() {
  CALL_ON_MAIN_THREAD (
    self.doPause();
  );
  return RT_OK;
}

rtError rtRMFPlayer::stop() {
  CALL_ON_MAIN_THREAD (
    self.doStop();
  );
  return RT_OK;
}

rtError rtRMFPlayer::destroy() {
  exit(0);
}

rtError rtRMFPlayer::changeSpeed(float speed, int32_t overshootTime) {
  m_playbackRate = speed;
  m_overshootTime = overshootTime;
  CALL_ON_MAIN_THREAD (
    self.doChangeSpeed();
  );
  return RT_OK;
}

rtError rtRMFPlayer::setListener(rtString eventName, const rtFunctionRef& f) {
  return m_eventEmitter.setListener(eventName, f);
}

rtError rtRMFPlayer::delListener(rtString  eventName, const rtFunctionRef& f) {
  return m_eventEmitter.delListener(eventName, f);
}

// Implementation of MediaPlayerClient interface
void rtRMFPlayer::mediaPlayerEngineUpdated() {
}

void rtRMFPlayer::mediaPlaybackCompleted() {
  m_eventEmitter.send(OnCompleteEvent());
}

void rtRMFPlayer::mediaFrameReceived() {
  std::string tuneTimeLog;
  if (m_setSrcTime) {
    m_firstFrameTime = g_get_monotonic_time();

    if (m_loadStartTime == 0 || m_loadStartTime > m_setSrcTime)
        m_loadStartTime = m_setSrcTime;

    char buffer[TUNE_LOG_MAX_SIZE];

    int len = snprintf(buffer, TUNE_LOG_MAX_SIZE, "QAM_TUNE_TIME:TYPE-%s,%u,%u,%u,%u,%u",
               m_contentType.c_str(),
               TO_MS(m_loadedTime - m_loadStartTime),        // time taken by load
               TO_MS(m_loadCompleteTime - m_loadStartTime),  // time of onLoaded event
               TO_MS(m_playEndTime - m_playStartTime),       // time taken by play
               TO_MS(m_firstFrameTime - m_playStartTime),    // time to render first frame
               TO_MS(m_firstFrameTime - m_loadStartTime));   // tune time

    if (len > 0) {
      tuneTimeLog = std::string(buffer, len);
    }

    m_setSrcTime = m_loadStartTime = 0;
  }

  const std::string& lang = m_mediaPlayer->rmf_getAudioLanguages();
  const std::string& ccDescriptor = m_mediaPlayer->rmf_getCaptionDescriptor();
  m_eventEmitter.send(OnVideoMetadataEvent(lang.c_str(), ccDescriptor.c_str()));
  m_eventEmitter.send(OnMediaFrameReceivedEvent());

  if (!tuneTimeLog.empty()) {
    m_eventEmitter.send(OnMetricLogEvent(tuneTimeLog.c_str()));
  }
}

void rtRMFPlayer::mediaWarningReceived() {
  if (!m_isLoaded)
    return;
  uint32_t code = m_mediaPlayer->rmf_getMediaWarnData();
  const std::string& description = m_mediaPlayer->rmf_getMediaWarnDescription();
  m_eventEmitter.send(OnMediaWarningEvent(code, description));
}

void rtRMFPlayer::eissDataReceived() {
  if (!m_isLoaded)
    return;
  const std::string& eissData = m_mediaPlayer->rmf_getEISSDataBuffer();
  m_eventEmitter.send(OnEISSDataReceivedEvent(eissData));
}

void rtRMFPlayer::volumeChanged(float volume) {
}

void rtRMFPlayer::playerStateChanged() {
  MediaPlayer::RMFPlayerState oldState = m_playerState;
  MediaPlayer::RMFPlayerState newState = m_mediaPlayer->rmf_playerState();
  if (newState == oldState)
    return;

  LOG_INFO("oldState=%s newState=%s", StateString(oldState), StateString(newState));

  if (newState == MediaPlayer::RMF_PLAYER_FORMATERROR ||
      newState == MediaPlayer::RMF_PLAYER_NETWORKERROR ||
      newState == MediaPlayer::RMF_PLAYER_DECODEERROR) {
    uint32_t code = static_cast<uint32_t>(m_mediaPlayer->rmf_playerState());
    const std::string& description = m_mediaPlayer->rmf_getMediaErrorMessage();
    m_eventEmitter.send(OnMediaErrorEvent(code, description));
    return;
  }
  m_playerState = newState;
}

void rtRMFPlayer::videoStateChanged() {
  MediaPlayer::RMFVideoBufferState oldState = m_videoState;
  MediaPlayer::RMFVideoBufferState newState = m_mediaPlayer->rmf_videoState();
  if (newState == oldState)
    return;

  LOG_INFO("oldState=%s newState=%s", StateString(oldState), StateString(newState));

  m_videoState = newState;
  if (oldState > m_videoStateMaximum)
    m_videoStateMaximum = oldState;

  bool isPotentiallyPlaying = potentiallyPlaying();

  bool isTimeUpdated = false;
  isTimeUpdated  = (isPotentiallyPlaying && newState < MediaPlayer::RMF_VIDEO_BUFFER_HAVEFUTUREDATA);
  isTimeUpdated |= (newState >= MediaPlayer::RMF_VIDEO_BUFFER_HAVEMETADATA    && oldState < MediaPlayer::RMF_VIDEO_BUFFER_HAVEMETADATA);
  isTimeUpdated |= (newState >= MediaPlayer::RMF_VIDEO_BUFFER_HAVECURRENTDATA && oldState < MediaPlayer::RMF_VIDEO_BUFFER_HAVEMETADATA);

  if (isTimeUpdated)
    timeChanged();

  bool isPlaying = false;
  if (isPotentiallyPlaying) {
    isPlaying  = (newState >= MediaPlayer::RMF_VIDEO_BUFFER_HAVEFUTUREDATA && oldState <= MediaPlayer::RMF_VIDEO_BUFFER_HAVECURRENTDATA);
  }

  if (isPlaying) {
    if (m_isPaused) {
      m_isPaused = false;
      m_eventEmitter.send(OnPlayingEvent());
    }
  } else if (!m_isPaused &&
             m_videoStateMaximum >= MediaPlayer::RMF_VIDEO_BUFFER_HAVEFUTUREDATA &&
             m_videoState > MediaPlayer::RMF_VIDEO_BUFFER_HAVENOTHING &&
             m_videoState < MediaPlayer::RMF_VIDEO_BUFFER_HAVEFUTUREDATA) {
    m_isPaused = true;
    m_eventEmitter.send(OnPausedEvent());
  }
}

void rtRMFPlayer::durationChanged() {
  doTimeUpdate();
}

void rtRMFPlayer::timeChanged() {
  doTimeUpdate();
}

void rtRMFPlayer::rateChanged() {
  float rate = m_mediaPlayer->rmf_getRate();
  if (rate != m_lastReportedPlaybackRate) {
    m_lastReportedPlaybackRate = rate;
    LOG_INFO("newRate=%f", rate);
    doTimeUpdate(true);
    m_eventEmitter.send(OnRateChangeEvent(rate));
  }
}

void rtRMFPlayer::videoDecoderHandleReceived() {
  uint32_t vDecoderHandle = m_mediaPlayer->rmf_getCCDecoderHandle();
  m_eventEmitter.send(OnVidHandleEvent(vDecoderHandle));
}

// Private methods
void rtRMFPlayer::doSetVideoRectangle() {
  if(!m_mediaPlayer)
    return;

  IntRect videoRect;
  videoRect = m_videoRect;
  if (videoRect.width() > 0 && videoRect.height() > 0) {
    LOG_INFO("set video rectangle: %dx%d %dx%d",
      videoRect.x(), videoRect.y(),
      videoRect.width(), videoRect.height());
    m_mediaPlayer->rmf_setVideoRectangle(
      videoRect.x(), videoRect.y(),
      videoRect.width(), videoRect.height());
  }
}

void rtRMFPlayer::doSetAudioLanguage() {
  if(!m_mediaPlayer)
    return;

  std::string lang = std::string(m_audioLanguage.cString());
  LOG_INFO("set lang: %s", lang.c_str());
  m_mediaPlayer->rmf_setAudioLanguage(lang);
}

void rtRMFPlayer::doSetAuxiliaryAudioLang() {
  if(!m_mediaPlayer)
    return;

  std::string lang = std::string(m_auxiliaryAudioLang.cString());
  LOG_INFO("set auxiliary lang: %s", lang.c_str());
  m_mediaPlayer->rmf_setAuxiliaryAudioLang(lang);
}

void rtRMFPlayer::doLoad() {
  std::string currentSrc;
  {
    m_playerState = MediaPlayer::RMF_PLAYER_LOADING;
    m_videoState = MediaPlayer::RMF_VIDEO_BUFFER_HAVENOTHING;
    m_videoStateMaximum = MediaPlayer::RMF_VIDEO_BUFFER_HAVENOTHING;
    m_lastReportedCurrentTime = 0.f;
    m_lastReportedDuration = 0.f;
    m_lastReportedPlaybackRate = 0.f;
    m_isPaused = true;
    currentSrc = m_currentSrc.cString();
  }

  if (MediaPlayerDLNA::supportsUrl(currentSrc)) {
      m_mediaPlayer.reset(NULL);
      m_mediaPlayer.reset(new MediaPlayerDLNA(this));
  }    
  else {
      m_mediaPlayer.reset(NULL);
      m_mediaPlayer.reset(new MediaPlayerGeneric(this));
  }

  m_eventEmitter.send(OnReadyEvent());

  setContentType(currentSrc);

  LOG_INFO("load(%s)", currentSrc.c_str());

  if(m_mediaPlayer)
    m_isLoaded = m_mediaPlayer->rmf_load(currentSrc);

  m_loadedTime = g_get_monotonic_time();

  if (m_isLoaded) {
    m_eventEmitter.send(OnLoadedEvent());
    doSetVideoRectangle();
  }

  m_loadCompleteTime = g_get_monotonic_time();
}

void rtRMFPlayer::doPlay() {
  if(!m_mediaPlayer)
    return;

  static const bool testHangDetector = getenv("RTRMFPLAYER_TEST_HANG_DETECTOR");
  if (testHangDetector) {
      sleep(1000);
  }
  if (!m_mediaPlayer->rmf_canItPlay()) {
    LOG_INFO("cannot play, skip play()");
    return;
  }
  if (m_isPaused || m_mediaPlayer->rmf_isPaused()) {
    LOG_INFO("play()");
    m_mediaPlayer->rmf_play();
    if (m_videoState >= MediaPlayer::RMF_VIDEO_BUFFER_HAVEFUTUREDATA) {
      m_isPaused = false;
      m_eventEmitter.send(OnPlayingEvent());
    }
  } else {
    LOG_INFO("not paused, skip play()");
  }

  m_playEndTime = g_get_monotonic_time();
  
  startPlaybackProgressMonitor();
}

void rtRMFPlayer::doPause() {
  if(!m_mediaPlayer)
    return;

  if (!m_mediaPlayer->rmf_isPaused()) {
    LOG_INFO("pause()");
    m_mediaPlayer->rmf_pause();
    doTimeUpdate(true);
    if (m_mediaPlayer->rmf_isPaused()) {
      m_isPaused = true;
      m_eventEmitter.send(OnPausedEvent());
      stopPlaybackProgressMonitor();
    }
  }
}

void rtRMFPlayer::doStop() {
  if(!m_mediaPlayer)
    return;

  LOG_INFO("stop()");
  m_isLoaded = false;
  stopPlaybackProgressMonitor();
  m_mediaPlayer->rmf_stop();
  m_mediaPlayer.reset(NULL);
}

void rtRMFPlayer::doChangeSpeed() {
  if(!m_mediaPlayer)
    return;

  float speed;
  int32_t overshootTime;
  speed = m_playbackRate;
  overshootTime = m_overshootTime;
  LOG_INFO("changeSpeed(%f, %d)", speed, overshootTime);
  if (m_isPaused) {
    m_isPaused = false;
    m_eventEmitter.send(OnPlayingEvent());
    startPlaybackProgressMonitor();
  }

  m_mediaPlayer->rmf_changeSpeed(speed, overshootTime);
}

void rtRMFPlayer::doSetRate() {
  if(!m_mediaPlayer)
    return;

  float rate = 0.f;
  rate = m_playbackRate;
  if (m_mediaPlayer->rmf_getRate() != rate) {
    LOG_INFO("set rate: %f", rate);
    m_mediaPlayer->rmf_setRate(rate);
  }
}

void rtRMFPlayer::doSetMuted() {
  if(!m_mediaPlayer)
    return;

  bool isMuted = false;
  isMuted = m_isMuted;
  if (m_mediaPlayer->rmf_isMuted() != isMuted) {
    LOG_INFO("set mute: %s", isMuted ? "true" : "false");
    m_mediaPlayer->rmf_setMute(isMuted);
  }
}

void rtRMFPlayer::doSetEISSFilterStatus() {
  if(!m_mediaPlayer)
    return;

  bool bEISSFilterStatus = false;
  bEISSFilterStatus = m_eissFilterStatus;
  LOG_INFO("set EISS Filter Status: %s", bEISSFilterStatus? "true" : "false");
  m_mediaPlayer->rmf_setEissFilterStatus(bEISSFilterStatus);
}

void rtRMFPlayer::doSetVolume() {
  if(!m_mediaPlayer)
    return;

  float volume = 0.f;
  volume = m_volume;
  LOG_INFO("set volume: %f", volume);
  if(!m_isMuted)
  {
    if(m_volume > 0)
        m_mediaPlayer->rmf_setMute(m_isMuted);
  }
  m_mediaPlayer->rmf_setVolume(volume);
}

void rtRMFPlayer::doJumpToLive() {
  if(!m_mediaPlayer)
    return;

  bool jump = false;
  jump = m_jumpToLive;
  if (jump && m_mediaPlayer->rmf_isItInProgressRecording()) {
    if (m_mediaPlayer->rmf_getRate() != 1.f) {
      LOG_INFO("set rate: 1.0");
      m_mediaPlayer->rmf_setRate(1.f);
    }
    LOG_INFO("jumpToLive: true");
    m_mediaPlayer->rmf_seekToLivePosition();
  }
}

void rtRMFPlayer::doSetIsInProgressRecording() {
  if(!m_mediaPlayer)
    return;

  bool isInProgressRecording = false;
  isInProgressRecording = m_isInProgressRecording;
  LOG_TRACE("set isInProgressRecording: %s", isInProgressRecording ? "true" : "false");
  m_mediaPlayer->rmf_setInProgressRecording(isInProgressRecording);
}

void rtRMFPlayer::doSetVideoZoom() {
  if(!m_mediaPlayer)
    return;

  int32_t videoZoom;
  videoZoom = m_videoZoom;
  LOG_INFO("set videoZoom: %d", videoZoom);
  m_mediaPlayer->rmf_setVideoZoom(videoZoom);
}

void rtRMFPlayer::doNetworkBufferSize() {
  if(!m_mediaPlayer)
    return;

  int32_t networkBufferSize;
  networkBufferSize = m_networkBufferSize;
  LOG_INFO("set networkBufferSize: %d", networkBufferSize);
  m_mediaPlayer->rmf_setNetworkBufferSize(networkBufferSize);
}

void rtRMFPlayer::doSetVideoBufferLength() {
  if(!m_mediaPlayer)
    return;

  float videoBufferLength;
  videoBufferLength = m_videoBufferLength;
  LOG_TRACE("set videoBufferLength: %f", videoBufferLength);
  m_mediaPlayer->rmf_setVideoBufferLength(videoBufferLength);
}

void rtRMFPlayer::doSeek() {
  if(!m_mediaPlayer)
    return;

  float seekTime = 0.f;
  seekTime = m_seekTime;
  m_seekTime = std::numeric_limits<float>::quiet_NaN();
  if (!std::isnan(seekTime)) {
      LOG_INFO("set currentTime: %f", seekTime);
      m_mediaPlayer->rmf_seek(seekTime);
      /* Seek will ensure that the video is Playing */
      if(m_isPaused) {
          m_isPaused = false;
          m_eventEmitter.send(OnPlayingEvent());
          startPlaybackProgressMonitor();
      }
  }
}

void rtRMFPlayer::doTimeUpdate(bool forced) {
  if (!m_isLoaded)
    return;

  refreshCachedCurrentTime(forced);

  if (m_duration != m_lastReportedDuration) {
    m_lastReportedDuration = m_duration;
    m_eventEmitter.send(OnDurationChangeEvent(m_duration));
  }

  if (m_currentTime != m_lastReportedCurrentTime) {
    m_lastReportedCurrentTime = m_currentTime;
    m_eventEmitter.send(OnTimeUpdateEvent(this));
  }
}

void rtRMFPlayer::startPlaybackProgressMonitor() {
  if (m_playbackProgressMonitorTag != 0)
    return;
  static const auto triggerTimeUpdateFunc = [](gpointer data) -> gboolean {
    rtRMFPlayer &self = *static_cast<rtRMFPlayer*>(data);
    self.doTimeUpdate();
    return G_SOURCE_CONTINUE;
  };
  m_playbackProgressMonitorTag =
    g_timeout_add(kProgressMonitorTimeoutMs, triggerTimeUpdateFunc, this);
}

void rtRMFPlayer::stopPlaybackProgressMonitor() {
  if (m_playbackProgressMonitorTag)
    g_source_remove(m_playbackProgressMonitorTag);
  m_playbackProgressMonitorTag = 0;
  m_lastRefreshTime = 0;
}

bool rtRMFPlayer::refreshCachedCurrentTime(bool forced) {
  if (!m_isLoaded) {
    return false;
  }

  gint64 now = g_get_monotonic_time();
  if (!forced && m_currentTime > 0.f &&
      (now - m_lastRefreshTime) < kRefreshCachedTimeThrottleMs * 1000) {
    return false;
  }

  m_duration = m_mediaPlayer->rmf_getDuration();
  m_currentTime = m_mediaPlayer->rmf_getCurrentTime();
  m_lastRefreshTime = now;

  return true;
}

bool rtRMFPlayer::endedPlayback() const {
  float dur = m_mediaPlayer->rmf_getDuration();
  if (!std::isnan(dur))
    return false;
  if (m_videoState < MediaPlayer::RMF_VIDEO_BUFFER_HAVEMETADATA)
    return false;
  int rate = m_mediaPlayer->rmf_getRate();
  float now = m_mediaPlayer->rmf_getCurrentTime();
  if (rate > 0)
    return dur > 0 && now >= dur;
  if (rate < 0)
    return now <= 0;
  return false;
}

bool rtRMFPlayer::couldPlayIfEnoughData() const {
  if (m_videoState >= MediaPlayer::RMF_VIDEO_BUFFER_HAVEMETADATA &&
      m_playerState >= MediaPlayer::RMF_PLAYER_FORMATERROR) {
    return false;
  }
  return !m_mediaPlayer->rmf_isPaused() && !endedPlayback();
}

bool rtRMFPlayer::potentiallyPlaying() const {
  bool pausedToBuffer = (m_videoStateMaximum >= MediaPlayer::RMF_VIDEO_BUFFER_HAVEFUTUREDATA) && (m_videoState < MediaPlayer::RMF_VIDEO_BUFFER_HAVEFUTUREDATA);
  return (pausedToBuffer || m_videoState >= MediaPlayer::RMF_VIDEO_BUFFER_HAVEFUTUREDATA) && couldPlayIfEnoughData();
}

void rtRMFPlayer::setContentType(const std::string &uri) {
  const char* dvrStrIdentifier  = "recordingId=";
  const char* liveStrIdentifier = "live=ocap://";
  const char* vodStrIdentifier  = "live=vod://";

  if (uri.find(dvrStrIdentifier) != std::string::npos) {
    if (uri.find("169.") != std::string::npos)  {
      m_contentType = "mDVR";
    } else {
      m_contentType = "DVR";
    }
  } else if (uri.find(liveStrIdentifier) != std::string::npos) {
    m_contentType = "LIVE";
  } else if (uri.find(vodStrIdentifier) != std::string::npos) {
    m_contentType = "VOD";
  } else {
    m_contentType = "WebVideo";
  }
}

static const rtError RT_ERROR_SYS_EPIPE = rtErrorFromErrno(EPIPE);

rtError rtEmitWithResult::Send(int numArgs, const rtValue* args, rtValue*) {
  rtError error = RT_OK;
  if (numArgs > 0) {
    rtString eventName = args[0].toString();
    LOG_TRACE("RDKBrowserEmit::Send %s", eventName.cString());

    std::vector<_rtEmitEntry>::iterator itr = mEntries.begin();
    while (itr != mEntries.end()) {
      _rtEmitEntry& entry = *itr;
      if (entry.n != eventName) {
        ++itr;
        continue;
      }

      rtValue discard;
      // SYNC EVENTS
#ifndef DISABLE_SYNC_EVENTS
      // SYNC EVENTS ... enables stopPropagation() ...
      //
      error = entry.f->Send(numArgs - 1, args + 1, &discard);
#else

#warning "  >>>>>>  No SYNC EVENTS... stopPropagation() will be broken !!"

      error = entry.f->Send(numArgs-1, args+1, NULL);
#endif
      if (error != RT_OK) {
        LOG_ERROR("Failed to send. [%d] %s", error, rtStrError(error));
        // EPIPE means it's disconnected
        if (error == RT_ERROR_SYS_EPIPE || error == RT_ERROR_STREAM_CLOSED) {
          LOG_WARNING("Broken entry in mEntries");
        }
      }

      // There can be only one listener for an event as of now
      break;
    }
  }

  return error;
}

rtError EventEmitter::send(Event&& event) {
  auto handleEvent = [](gpointer data) -> gboolean {
    EventEmitter& self = *static_cast<EventEmitter*>(data);

    if (!self.m_eventQueue.empty()) {
      rtObjectRef obj = self.m_eventQueue.front();
      self.m_eventQueue.pop();

      rtError rc = self.m_emit.send(obj.get<rtString>("name"), obj);
      if (RT_OK != rc) {
          LOG_ERROR("Can't send event (%s), error code: %d", obj.get<rtString>("name").cString(), rc);
      }

      // if timeout occurs do not increment hang detector or stream is closed disable hang detection.
      if (RT_ERROR_TIMEOUT == rc || rc == RT_ERROR_SYS_EPIPE || rc == RT_ERROR_STREAM_CLOSED) {
        if (!self.m_isRemoteClientHanging) {
          self.m_isRemoteClientHanging = true;
          LOG_WARNING("Remote client is entered to a hanging state");
        }
        if (rc == RT_ERROR_SYS_EPIPE || rc == RT_ERROR_STREAM_CLOSED) {
          LOG_WARNING("Remote client connection seems to be closed/broken");
          // Clear the listeners here
          self.m_emit->clearListeners();
        }
      }
      else if (RT_OK == rc) {
        if (self.m_isRemoteClientHanging) {
          self.m_isRemoteClientHanging = false;
          LOG_WARNING("Remote client is recovered after the hanging state; Sent (%s) Event to Remote client", obj.get<rtString>("name").cString());
        }
      }

      if (!self.m_eventQueue.empty()) {
        return G_SOURCE_CONTINUE;
      }
    }

    self.m_timeoutId = 0;
    return G_SOURCE_REMOVE;
  };

  m_eventQueue.push(event.object());

  if (m_timeoutId == 0) {
    m_timeoutId = g_timeout_add(0, handleEvent, (void*) this);
  }

  return RT_OK;
}
