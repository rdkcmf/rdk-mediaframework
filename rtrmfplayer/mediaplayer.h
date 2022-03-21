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
#ifndef _MEDIA_PLAYER_H_
#define _MEDIA_PLAYER_H_

#include <string>

class MediaPlayerClient {
 public:
  virtual void mediaPlayerEngineUpdated() = 0;
  virtual void mediaPlaybackCompleted() = 0;
  virtual void mediaFrameReceived() = 0;
  virtual void mediaWarningReceived() = 0;
  virtual void volumeChanged(float volume) = 0;
  virtual void playerStateChanged() = 0;
  virtual void videoStateChanged() = 0;
  virtual void durationChanged() = 0;
  virtual void timeChanged() = 0;
  virtual void rateChanged() = 0;
  virtual void videoDecoderHandleReceived() = 0;
  virtual void eissDataReceived() = 0;
};

class MediaPlayer {
 public:
  enum RMFPlayerState {
    RMF_PLAYER_EMPTY,
    RMF_PLAYER_IDLE,
    RMF_PLAYER_LOADING,
    RMF_PLAYER_LOADED,
    RMF_PLAYER_FORMATERROR,
    RMF_PLAYER_NETWORKERROR,
    RMF_PLAYER_DECODEERROR
  };

  enum RMFVideoBufferState {
    RMF_VIDEO_BUFFER_HAVENOTHING,
    RMF_VIDEO_BUFFER_HAVEMETADATA,
    RMF_VIDEO_BUFFER_HAVECURRENTDATA,
    RMF_VIDEO_BUFFER_HAVEFUTUREDATA,
    RMF_VIDEO_BUFFER_HAVEENOUGHDATA
  };

  virtual ~MediaPlayer() {}

  virtual MediaPlayer::RMFPlayerState rmf_playerState() const = 0;
  virtual MediaPlayer::RMFVideoBufferState rmf_videoState() const = 0;
  virtual bool rmf_load(const std::string &url) = 0;
  virtual void rmf_play() = 0;
  virtual void rmf_stop() = 0;
  virtual void rmf_pause() = 0;
  virtual bool rmf_canItPlay() const = 0;
  virtual bool rmf_isPaused() const = 0;
  virtual void rmf_setRate(float speed) = 0;
  virtual float rmf_getRate() const = 0;
  virtual void rmf_setVolume(float volume) = 0;
  virtual float rmf_getVolume() const = 0;
  virtual void rmf_setMute(bool mute) = 0;
  virtual bool rmf_isMuted() const = 0;
  virtual float rmf_getDuration() const = 0;
  virtual float rmf_getCurrentTime() const = 0;
  virtual void rmf_seekToLivePosition() = 0;
  virtual void rmf_seekToStartPosition() = 0;
  virtual void rmf_seek(float time) = 0;
  virtual bool rmf_isSeeking() const = 0;
  virtual unsigned long rmf_getCCDecoderHandle() const = 0;
  virtual std::string rmf_getAudioLanguages() const = 0;
  virtual void rmf_setAudioLanguage(const std::string &audioLang) = 0;
  virtual void rmf_setAuxiliaryAudioLang(const std::string &audioLang) = 0;
  virtual void rmf_setEissFilterStatus(bool status) = 0;
  virtual void rmf_setVideoZoom(unsigned short zoomVal) = 0;
  virtual void rmf_setVideoBufferLength(float bufferLength) = 0;
  virtual void rmf_setInProgressRecording(bool isInProgress) = 0;
  virtual bool rmf_isItInProgressRecording() const = 0;
  virtual std::string rmf_getMediaErrorMessage() const = 0;
  virtual void rmf_changeSpeed(float speed, short overshootTime) = 0;
  virtual std::string rmf_getMediaWarnDescription() const = 0;
  virtual int rmf_getMediaWarnData() const = 0;
  virtual std::string rmf_getAvailableAudioTracks() const = 0;
  virtual std::string rmf_getCaptionDescriptor() const = 0;
  virtual std::string rmf_getEISSDataBuffer() const = 0;
  virtual void rmf_setNetworkBufferSize(int bufferSize) = 0;
  virtual int rmf_getNetworkBufferSize() const = 0;
  virtual void rmf_setVideoRectangle(unsigned x, unsigned y, unsigned w, unsigned h) = 0;
};

#define CASE(x) case x: return #x
inline const char* StateString(MediaPlayer::MediaPlayer::RMFPlayerState st) {
  switch (st) {
    CASE(MediaPlayer::RMF_PLAYER_EMPTY);
    CASE(MediaPlayer::RMF_PLAYER_IDLE);
    CASE(MediaPlayer::RMF_PLAYER_LOADING);
    CASE(MediaPlayer::RMF_PLAYER_LOADED);
    CASE(MediaPlayer::RMF_PLAYER_FORMATERROR);
    CASE(MediaPlayer::RMF_PLAYER_NETWORKERROR);
    CASE(MediaPlayer::RMF_PLAYER_DECODEERROR);
    default: return "Unknown";
  }
}
inline const char* StateString(MediaPlayer::RMFVideoBufferState st) {
  switch (st) {
    CASE(MediaPlayer::RMF_VIDEO_BUFFER_HAVENOTHING);
    CASE(MediaPlayer::RMF_VIDEO_BUFFER_HAVEMETADATA);
    CASE(MediaPlayer::RMF_VIDEO_BUFFER_HAVECURRENTDATA);
    CASE(MediaPlayer::RMF_VIDEO_BUFFER_HAVEFUTUREDATA);
    CASE(MediaPlayer::RMF_VIDEO_BUFFER_HAVEENOUGHDATA);
    default: return "Unknown";
  }
}
#undef CASE

#endif  // _MEDIA_PLAYER_H_
