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
class EventHandler;

class pipeline
{
  int c_Initialized;

  std::string c_source;
  std::string c_sink;
  std::string c_url;
  int c_currentChannel;
  int c_numOfChannels;
  std::string c_channels[50];

  int c_tsb;
  std::string c_tsbId;
  int c_duration;
  bool c_isLive;
  bool c_isPaused;
  bool c_goLive;
  double c_resumePoint;
 
 #ifdef USE_DVR 
  DVRManager *c_dvm;
  int c_dvrres;
  char c_work[256+1];
  std::string c_recordingId;
  char *c_recordingTitle;
  char *c_dvrLocator;
  bool c_useBufferedOnly;
#endif
  RMFMediaSourceBase* c_PlaySource;
  RMFMediaSinkBase* c_PlaySink;
  RMFMediaSinkBase* c_RecordSink;
  #ifdef USE_MEDIAPLAYERSINK
  MediaPlayerSink* c_pMediaPlayerSink;
  bool switchToLive( MediaPlayerSink* &pMediaPlayerSink );
  bool switchToTSB( MediaPlayerSink* &pMediaPlayerSink, const char *dvrLocator, bool start );
  #endif
  long long getCurrentTime();
  RMFMediaSinkBase* createSink();
  RMFMediaSourceBase* createSource();
  
public:
   IRMFMediaEvents *events;
  RMFMediaSourceBase* c_pSource;
  RMFMediaSinkBase* c_pSink;
  pipeline();
  ~pipeline();
  int init(launch_param_t *params);
  void reset(void);
  void handleCommand(int cmd, const char* cmd_option);
#if defined(ENABLE_CLOSEDCAPTION)
  void startcCaption();
  void stopcCaption();
#endif
  int run(void);
#ifdef USE_MEDIAPLAYERSINK
  int dvrKeys(int);
#endif
  int destroyPipeline();
  bool processRequest();
  static int initQAM();
  static int numofPipelines;
  static int mplayerInUse;
  static int podInitialized;
  
};

