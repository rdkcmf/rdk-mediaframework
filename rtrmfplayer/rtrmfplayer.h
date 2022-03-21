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
#ifndef _RT_RMF_PLAYER_H_
#define _RT_RMF_PLAYER_H_

#include <glib.h>
#include <mutex>
#include <memory>
#include <queue>

#include <rtRemote.h>
#include <rtError.h>

#include "intrect.h"
#include "mediaplayer.h"
#include "logger.h"

class Event
{
protected:
    rtObjectRef m_object;
    rtString name() const
    {
        return m_object.get<rtString>("name");
    }
    rtObjectRef object() const
    {
        return m_object;
    }
    Event(rtObjectRef o) : m_object(o)
    {
    }
public:
    Event(const char* eventName) : m_object(new rtMapObject)
    {
        m_object.set("name", eventName);
    }
    virtual ~Event() { }
    friend class EventEmitter;
};

class rtEmitWithResult : public rtEmit
{
public:
  virtual rtError Send(int numArgs, const rtValue* args, rtValue* result) override;
};

class EventEmitter
{
public:
  EventEmitter()
    : m_emit(new rtEmitWithResult)
    , m_timeoutId(0)
    , m_isRemoteClientHanging(false)
  {
  }

  ~EventEmitter()
  {
    if (m_timeoutId != 0) {
      g_source_remove(m_timeoutId);
    }
  }

  rtError setListener(const char* eventName, rtIFunction* f)
  {
    return m_emit->setListener(eventName, f);
  }
  rtError delListener(const char* eventName, rtIFunction* f)
  {
    return m_emit->delListener(eventName, f);
  }
  rtError send(Event&& event);
  bool isRemoteClientHanging() const { return m_isRemoteClientHanging; }

private:
  rtEmitRef m_emit;
  std::queue<rtObjectRef> m_eventQueue;
  int m_timeoutId;
  bool m_isRemoteClientHanging;
};

/**
 * @class rtRMFPlayer
 * @brief Exposes DLNA player interface through rtRemote API
 */
class rtRMFPlayer : public rtObject, public MediaPlayerClient {
public:
  rtDeclareObject(rtRMFPlayer, rtObject);
  rtProperty(src, currentSrc, setCurrentSrc, rtString);
  rtProperty(audioLanguage, audioLanguage, setAudioLanguage, rtString);
  rtProperty(auxiliaryAudioLang, auxiliaryAudioLang, setAuxiliaryAudioLang, rtString);
  rtProperty(currentTime, currentTime, setCurrentTime, float);
  rtProperty(playbackRate, playbackRate, setPlaybackRate, float);
  rtProperty(volume, volume, setVolume, float);
  rtProperty(muted, muted, setMuted, rtString);
  rtProperty(jumpToLive, jumpToLive, setJumpToLive, rtString);
  rtProperty(isInProgressRecording, isInProgressRecording, setIsInProgressRecording, rtString);
  rtProperty(videoZoom, videoZoom, setVideoZoom, int32_t);
  rtProperty(networkBufferSize, networkBufferSize, setNetworkBufferSize, int32_t);
  rtProperty(videoBufferLength, videoBufferLength, setVideoBufferLength, float);
  rtProperty(eissFilterStatus, eissFilterStatus, setEissFilterStatus, rtString);
  rtProperty(loadStartTime, loadStartTime, setLoadStartTime, int64_t);
  rtMethod4ArgAndNoReturn("setVideoRectangle", setVideoRectangle, int32_t, int32_t, int32_t, int32_t);
  rtMethodNoArgAndNoReturn("load", load);
  rtMethodNoArgAndNoReturn("play", play);
  rtMethodNoArgAndNoReturn("pause", pause);
  rtMethodNoArgAndNoReturn("stop", stop);
  rtMethodNoArgAndNoReturn("destroy", destroy);
  rtMethod2ArgAndNoReturn("changeSpeed", changeSpeed, float, int32_t);
  rtMethod2ArgAndNoReturn("on", setListener, rtString, rtFunctionRef);
  rtMethod2ArgAndNoReturn("delListener", delListener, rtString, rtFunctionRef);

  rtRMFPlayer();
  virtual ~rtRMFPlayer();

  virtual void onInit();

  /**
   * rtRemote properties
   */
  rtError currentSrc(rtString& s) const;
  rtError setCurrentSrc(rtString const& s);
  rtError audioLanguage(rtString& s) const;
  rtError setAudioLanguage(rtString const& s);
  rtError auxiliaryAudioLang(rtString& s) const;
  rtError setAuxiliaryAudioLang(rtString const& s);
  rtError currentTime(float& t) const;
  rtError setCurrentTime(float const& t);
  rtError playbackRate(float& t) const;
  rtError setPlaybackRate(float const& t);
  rtError volume(float& t) const;
  rtError setVolume(float const& t);
  rtError muted(rtString& t) const;
  rtError setMuted(rtString const& t);
  rtError jumpToLive(rtString& t) const;
  rtError setJumpToLive(rtString const& t);
  rtError isInProgressRecording(rtString& t) const;
  rtError setIsInProgressRecording(rtString const& t);
  rtError videoZoom(int32_t& t) const;
  rtError setVideoZoom(int32_t const& t);
  rtError networkBufferSize(int32_t& t) const;
  rtError setNetworkBufferSize(int32_t const& t);
  rtError videoBufferLength(float& t) const;
  rtError setVideoBufferLength(float const& t);
  rtError eissFilterStatus(rtString& t) const;
  rtError setEissFilterStatus(rtString const& t);
  rtError loadStartTime(int64_t& t) const;
  rtError setLoadStartTime(int64_t const& t);

  /**
   * rtRemote methods
   */
  rtError setVideoRectangle(int32_t x, int32_t y, int32_t w, int32_t h);
  rtError load();
  rtError play();
  rtError pause();
  rtError stop();
  rtError destroy();
  rtError changeSpeed(float speed, int32_t overshootTime);
  rtError setListener(rtString eventName, const rtFunctionRef& f);
  rtError delListener(rtString  eventName, const rtFunctionRef& f);

  /**
   * Implementation of MediaPlayerClient interface
   */
  void mediaPlayerEngineUpdated();
  void mediaPlaybackCompleted();
  void mediaFrameReceived();
  void mediaWarningReceived();
  void volumeChanged(float volume);
  void playerStateChanged();
  void videoStateChanged();
  void durationChanged();
  void timeChanged();
  void rateChanged();
  void videoDecoderHandleReceived();
  void eissDataReceived();

private:
  void doSetVideoRectangle();
  void doSetAudioLanguage();
  void doSetAuxiliaryAudioLang();
  void doLoad();
  void doPlay();
  void doPause();
  void doStop();
  void doChangeSpeed();
  void doSetRate();
  void doSetMuted();
  void doSetEISSFilterStatus();
  void doSetVolume();
  void doJumpToLive();
  void doSetIsInProgressRecording();
  void doSetVideoZoom();
  void doNetworkBufferSize();
  void doSetVideoBufferLength();
  void doSeek();
  void doTimeUpdate(bool forced = false);

  void startPlaybackProgressMonitor();
  void stopPlaybackProgressMonitor();
  bool refreshCachedCurrentTime(bool forced = false);

  bool endedPlayback() const;
  bool potentiallyPlaying() const;
  bool couldPlayIfEnoughData() const;
  void setContentType(const std::string &uri);

  std::unique_ptr<MediaPlayer> m_mediaPlayer;
  EventEmitter m_eventEmitter;
  rtString m_currentSrc;
  rtString m_audioLanguage;
  rtString m_auxiliaryAudioLang;
  IntRect  m_videoRect;
  float    m_currentTime;
  float    m_lastReportedCurrentTime;
  float    m_duration;
  float    m_lastReportedDuration;
  float    m_seekTime;
  float    m_playbackRate;
  float    m_lastReportedPlaybackRate;
  float    m_volume;
  float    m_videoBufferLength;
  bool     m_isLoaded;
  bool     m_isMuted;
  bool     m_eissFilterStatus;
  bool     m_jumpToLive;
  bool     m_isInProgressRecording;
  bool     m_isPaused;
  int32_t  m_videoZoom;
  int32_t  m_networkBufferSize;
  int32_t  m_overshootTime;

  MediaPlayer::RMFPlayerState m_playerState;
  MediaPlayer::RMFVideoBufferState m_videoState;
  MediaPlayer::RMFVideoBufferState m_videoStateMaximum;

  guint m_playbackProgressMonitorTag;
  gint64 m_lastRefreshTime;

  std::string m_contentType;
  gint64 m_loadStartTime { 0 };
  gint64 m_setSrcTime { 0 };
  gint64 m_loadedTime { 0 };
  gint64 m_loadCompleteTime { 0 };
  gint64 m_playStartTime { 0 };
  gint64 m_playEndTime { 0 };
  gint64 m_firstFrameTime { 0 };
};
#endif  // _RT_RMF_PLAYER_H_
