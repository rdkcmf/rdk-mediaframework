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
#ifndef _MEDIAPLAYERDLNA_H_
#define _MEDIAPLAYERDLNA_H_

#include "intrect.h"
#include "mediaplayer.h"
#include "rmfbase.h"

class HNSource;
class MediaPlayerSink;
class DumpFileSink;
class IRMFMediaEvents;
struct RequestInfo;
struct ResponseInfo;

class MediaPlayerDLNA : public MediaPlayer {
public:
  explicit MediaPlayerDLNA(MediaPlayerClient* host);
  virtual ~MediaPlayerDLNA();

  bool rmf_load(const std::string &url);
  const std::string& rmf_getURL() const { return m_url; }

  bool rmf_canItPlay() const { return !m_url.empty(); }
  void rmf_play();
  void rmf_pause();
  void rmf_stop();
  bool rmf_isPaused() const;
  bool rmf_isSeeking() const;
  float rmf_getDuration() const;
  float rmf_getCurrentTime() const;
  void  rmf_seek(float position);
  void rmf_seekToLivePosition(void);
  void rmf_seekToStartPosition(void);
  void rmf_changeSpeed(float, short);
  void  rmf_setRate(float speed);
  float rmf_getRate() const { return m_playbackRate; }
  void  rmf_setVolume(float volume);
  float rmf_getVolume() const;
  void  rmf_setMute(bool muted);
  bool  rmf_isMuted() const { return m_muted; }
  unsigned long rmf_getCCDecoderHandle() const;
  std::string rmf_getAudioLanguages() const;
  void rmf_setAudioLanguage(const std::string& audioLang);
  void rmf_setAuxiliaryAudioLang(const std::string& audioLang);
  void rmf_setEissFilterStatus(bool status);
  void rmf_setVideoZoom(unsigned short zoomVal);
  void rmf_setVideoBufferLength(float bufferLength);
  void rmf_setInProgressRecording(bool isInProgress);
  bool rmf_isItInProgressRecording() const { return m_isInProgressRecording; }
  std::string rmf_getMediaErrorMessage() const;
  std::string rmf_getMediaWarnDescription() const;
  int rmf_getMediaWarnData() const;
  std::string rmf_getAvailableAudioTracks() const;
  std::string rmf_getCaptionDescriptor() const;
  std::string rmf_getEISSDataBuffer() const;
  void rmf_setNetworkBufferSize(int bufferSize);
  int rmf_getNetworkBufferSize() const { return  m_networkBufferSize; } 
  void rmf_setVideoRectangle(unsigned x, unsigned y, unsigned w, unsigned h);

  MediaPlayer::RMFPlayerState rmf_playerState() const;
  MediaPlayer::RMFVideoBufferState rmf_videoState() const;

  /* Signals */
  void notifyPlayerOfVideoAsync();
  void notifyPlayerOfVideo();
  void notifyPlayerOfAudioAsync();
  void notifyPlayerOfAudio();

  void notifyPlayerOfFirstAudioFrame();
  void notifyPlayerOfFirstVideoFrame();
  void notifyPlayerOfMediaWarning();
  void notifyPlayerOfEISSData();
  void notifyPresenceOfVideo();
  void onPlay();
  void onPause();
  void onStop();

  void ended();
  void notifyError (RMFResult err, const char *pMsg);

  static bool supportsUrl(const std::string& urlStr);

 private:
  void commitLoad();
  void cancelLoad();
  void prepareToPlay();
  void loadingFailed(MediaPlayer::RMFPlayerState);
  void setMediaErrorMessage(const std::string &errorMsg);
  void timeChanged();
  void durationChanged();

  bool isMpegTS() const;
  bool isVOD() const;
  bool isOCAP() const;
  bool wantAudio() const { return rmf_getRate() == 1; }  // no audio when seeking

  float playbackPosition() const;
  void cacheDuration();
  void updateStates();
  float maxTimeLoaded() const;

  void onReplyError();
  void onReplyFinished(const ResponseInfo& response);
  void onProgressTimerTimeout();
  void fetchHeaders(void);
  void fetchOnWorkerThread(const RequestInfo &request);
  void fetchWithCurl(const RequestInfo &request, ResponseInfo &response);
  void onFrameReceived(void);
  void startProgressTimer(double interval);

  void time_to_hms(float time, unsigned& h, unsigned& m, float& s);
  void fetchHeadersForTrickMode(float speed, double time);
  void onFirstVideoFrame();
  void onEISSDUpdate();
  void onFirstAudioFrame();
  void onMediaWarningReceived();
#ifdef ENABLE_DIRECT_QAM
  void getErrorMapping(RMFResult err, const char *pMsg);
#endif /* ENABLE_DIRECT_QAM */
  void handleCurrentAssetType();


  MediaPlayerClient* m_playerClient;
  HNSource* m_hnsource;
  IRMFMediaSource* m_source;
  MediaPlayerSink* m_sink;
  DumpFileSink* m_dfsink;
  IRMFMediaEvents* m_events;
  IRMFMediaEvents* m_events_sink;

  std::string m_url;
  bool m_changingRate;
  float m_endTime;
  MediaPlayer::RMFPlayerState m_playerState;
  MediaPlayer::RMFVideoBufferState m_videoState;
  mutable bool m_isStreaming;
  bool m_paused;
  bool m_pausedInternal;
  bool m_seeking;
  float m_playbackRate;
  bool m_errorOccured;
  float m_mediaDuration;
  bool m_startedBuffering;
  float m_maxTimeLoaded;
  bool m_delayingLoad;
  bool m_mediaDurationKnown;
  unsigned int m_volumeTimerHandler;
  unsigned int m_muteTimerHandler;
  bool m_muted;

  unsigned int m_videoTimerHandler;
  unsigned int m_audioTimerHandler;
  IntRect m_lastKnownRect;
  float m_currentPosition;
  bool m_isVOnlyOnce;
  bool m_isAOnlyOnce;
  bool m_isEndReached;
  bool m_isInProgressRecording;
  std::string m_errorMsg;
  float m_baseSeekTime;
  float m_currentProgressTime;
  mutable std::string m_rmfMediaWarnDesc;
  mutable std::string m_rmfAudioTracks;
  mutable std::string m_rmfCaptionDescriptor;
  mutable std::string m_rmfEISSDataBuffer;
  int m_rmfMediaWarnData;
  int m_networkBufferSize;
  unsigned char m_restartPlaybackCount;
  bool m_allowPositionQueries;

  bool m_EOSPending;
  bool m_fetchTrickModeHeaders;
  bool m_isVODPauseRequested;
  bool m_isVODRateChangeRequested;
  bool m_isVODSeekRequested;
  bool m_isVOD5XHackEnabled;
  bool m_isVODAsset;
  bool m_isMusicOnlyAsset;
#ifdef ENABLE_DIRECT_QAM
  bool m_isLiveAsset;
  bool m_isPPVAsset;
  bool m_isDVRAsset;
#endif /* ENABLE_DIRECT_QAM */
  bool m_VODKeepPipelinePlaying;
  bool m_eissFilterStatus;

  unsigned int m_onFirstVideoFrameHandler;
  unsigned int m_onFirstAudioFrameHandler;
  unsigned int m_currentAssetTypeHandler;
  unsigned int m_onEISSDUpdateHandler;
  unsigned int m_onMediaWarningReceivedHandler;
  unsigned int m_progressTimerHandler;
};

#endif  // _MEDIAPLAYERDLNA_H_
