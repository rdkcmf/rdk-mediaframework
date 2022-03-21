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
#ifndef MEDIAPLAYERGENERIC_H
#define MEDIAPLAYERGENERIC_H

#include <gst/gst.h>

#include "intrect.h"
#include "mediaplayer.h"

class MediaPlayerGeneric : public MediaPlayer
{
public:
  explicit MediaPlayerGeneric(MediaPlayerClient* client);
  virtual ~MediaPlayerGeneric();

  MediaPlayer::RMFPlayerState rmf_playerState() const;
  MediaPlayer::RMFVideoBufferState rmf_videoState() const;
  bool rmf_load(const std::string &url);
  void rmf_play();
  void rmf_stop();
  void rmf_pause();
  bool rmf_canItPlay() const;
  bool rmf_isPaused() const;
  void rmf_setRate(float speed);
  float rmf_getRate() const;
  void rmf_setVolume(float volume);
  float rmf_getVolume() const;
  void rmf_setMute(bool mute);
  bool rmf_isMuted() const;
  float rmf_getDuration() const;
  float rmf_getCurrentTime() const;
  void rmf_seekToLivePosition();
  void rmf_seekToStartPosition();
  void rmf_seek(float time);
  bool rmf_isSeeking() const;
  unsigned long rmf_getCCDecoderHandle() const;
  std::string rmf_getAudioLanguages() const;
  void rmf_setAudioLanguage(const std::string &audioLang);
  void rmf_setAuxiliaryAudioLang(const std::string &audioLang);
  void rmf_setEissFilterStatus(bool status);
  void rmf_setVideoZoom(unsigned short zoomVal);
  void rmf_setVideoBufferLength(float bufferLength);
  void rmf_setInProgressRecording(bool isInProgress);
  bool rmf_isItInProgressRecording() const;
  std::string rmf_getMediaErrorMessage() const;
  void rmf_changeSpeed(float speed, short overshootTime);
  std::string rmf_getMediaWarnDescription() const;
  int rmf_getMediaWarnData() const;
  std::string rmf_getAvailableAudioTracks() const;
  std::string rmf_getCaptionDescriptor() const;
  std::string rmf_getEISSDataBuffer() const;
  void rmf_setNetworkBufferSize(int bufferSize);
  int rmf_getNetworkBufferSize() const;
  void rmf_setVideoRectangle(unsigned x, unsigned y, unsigned w, unsigned h);

private:
  static void busMessageCallback(GstBus* bus, GstMessage* msg, gpointer data);

  GstElement* pipeline() const;

  void updateStates();
  bool changePipelineState(GstState newState);
  void handleBusMessage(GstBus* bus, GstMessage* msg);
  void endOfStream();
  void notifyError(const char *message);
  void loadingFailed(MediaPlayer::RMFPlayerState error);
  void cleanup();

  MediaPlayerClient* m_playerClient;
  GstElement* m_pipeline;
  MediaPlayer::RMFPlayerState m_playerState;
  MediaPlayer::RMFVideoBufferState m_videoState;
  std::string m_url;
  std::string m_errorMsg;
  bool m_errorOccured;
  bool m_seekIsPending;
  float m_seekTime;
  IntRect m_lastKnownRect;
};

#endif /* MEDIAPLAYERGENERIC_H */
