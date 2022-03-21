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
#include "mediaplayerdlna.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>
#include <limits>
#include <cmath>
#include <cassert>
#include <cstring>

#include <gst/audio/streamvolume.h>
#include <gst/video/video.h>
#include <curl/curl.h>

#include <hnsource.h>
#include <mediaplayersink.h>
#include <dumpfilesink.h>

#ifdef ENABLE_DIRECT_QAM
#include <rmfqamsrc.h>
#ifdef USE_VODSRC
#include <rmfvodsource.h>
#endif
#ifdef IPPV_CLIENT_ENABLED
#include <ippvsrc.h>
#endif
#endif

#include "logger.h"

#define HEADER_NAME_SEEK_RANGE "availableSeekRange.dlna.org"
#define HEADER_NAME_PLAYSPEED   "PlaySpeed.dlna.org"
#define HEADER_NAME_VODADINSERTION_STATUS "vodAdInsertionStatus.cmst.com"
#define RMF_VOD_BEGIN_PLAYBACK 7
#define RMF_VOD_BAD_START_POSITION_VAL 0x7FFFFFFFu

#define CURL_EASY_SETOPT(curl, CURLoption ,option)  \
        if (curl_easy_setopt(curl, CURLoption ,option) != 0) {\
                LOG_ERROR("Failed at curl_easy_setopt %d \n", curl);   \
        }  //CID:127965 - checked return

typedef std::vector<std::string> headers_t;

struct RequestInfo {
  std::string url;
  headers_t headers;
};

struct ResponseInfo {
  int http_code;
  std::string url;
  std::string payload;
  headers_t headers;
};

namespace {

bool starts_with(const std::string &s, const std::string &starting) {
  if (starting.size() > s.size())
    return false;
  return std::equal(starting.begin(), starting.end(), s.begin());
}

bool ends_with(const std::string &s, const std::string &ending) {
  if (ending.size() > s.size())
    return false;
  return std::equal(ending.rbegin(), ending.rend(), s.rbegin());
}

bool has_substring(const std::string &s, const std::string &substring) {
  return s.find(substring) != std::string::npos;
}

std::vector<std::string>& split_string(
    const std::string &s, char delim, std::vector<std::string> &elems) {
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

std::string &ltrim_string(std::string &s) {
  s.erase(
    s.begin(),
    std::find_if(
      s.begin(),
      s.end(),
      std::not1(std::ptr_fun<int, int>(std::isspace))));
  return s;
}

std::string &rtrim_string(std::string &s) {
  s.erase(
    std::find_if(
      s.rbegin(),
      s.rend(),
      std::not1(std::ptr_fun<int, int>(std::isspace))).base(),
    s.end());
  return s;
}

std::string &trim_string(std::string &s) {
  return ltrim_string(rtrim_string(s));
}

std::string& find_header(const headers_t &headers, const std::string &header_name, std::string &header_value) {
  header_value = std::string();
  for (headers_t::const_iterator cit = headers.begin();
       cit != headers.end(); ++cit) {
    const std::string& raw_header = *cit;
    if (starts_with(raw_header, header_name)) {
      const size_t column_pos = raw_header.find_first_of(':');
      header_value = raw_header.substr(column_pos + 1);
      header_value = trim_string(header_value);
      break;
    }
  }
  return header_value;
}

const int kCurlConnectTimeoutInSeconds = 10;
const int kCurlTimeoutInSeconds = 20;

#if LIBCURL_VERSION_NUM >= 0x071306
#define _CURLINFO_RESPONSE_CODE CURLINFO_RESPONSE_CODE
#else
#define _CURLINFO_RESPONSE_CODE CURLINFO_HTTP_CODE
#endif

size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  std::string *mem = reinterpret_cast<std::string*>(userp);
  *mem += std::string(reinterpret_cast<char*>(contents), realsize);
  return realsize;
}

class MediaEvents : public IRMFMediaEvents {
 public:
  explicit MediaEvents(MediaPlayerDLNA* player) : m_player(player) {}
  void playing()  { m_player->onPlay(); }
  void paused()   { m_player->onPause(); }
  void stopped()  { m_player->onStop(); }
  void complete() {
    LOG_INFO("DLNA player: End of Stream");
    m_player->ended();
  }
  void error(RMFResult err, const char* msg) {
    LOG_ERROR("DLNA player: Error %s (%d)", msg, (int)err);
    m_player->notifyError(err, msg);
  }
 private:
  MediaPlayerDLNA* m_player;
};

MediaPlayerDLNA *g_playerInstance = NULL;

}  // namespace

static void mediaPlayerPrivateHaveVideoCallback(void* player) {
    LOG_INFO("Got video");
    ASSERT(player == g_playerInstance);
    static_cast<MediaPlayerDLNA*>(player)->notifyPresenceOfVideo();
}

static void mediaPlayerPrivateHaveAudioCallback(void* player) {
  LOG_INFO("Got audio");
}

static void mediaPlayerPrivateEISSDataCallback(void* player)
{
    //LOG_INFO("Got EISS");
    ASSERT(player == g_playerInstance);
    static_cast<MediaPlayerDLNA*>(player)->notifyPlayerOfEISSData();
}
static void mediaPlayerPrivateAudioNotifyFirstFrameCallback(void* player) {
  ASSERT(player == g_playerInstance);
  static_cast<MediaPlayerDLNA*>(player)->notifyPlayerOfFirstAudioFrame();
}

static void mediaPlayerPrivateNotifyMediaWarningCallback(void* player) {
  ASSERT(player == g_playerInstance);
  static_cast<MediaPlayerDLNA*>(player)->notifyPlayerOfMediaWarning();
}

static void mediaPlayerPrivateVideoNotifyFirstFrameCallback(void* player) {
  ASSERT(player == g_playerInstance);
  static_cast<MediaPlayerDLNA*>(player)->notifyPlayerOfFirstVideoFrame();
}

bool MediaPlayerDLNA::isVOD() const {
  return has_substring(m_url, "vod://");
}

bool MediaPlayerDLNA::isOCAP() const {
  return has_substring(m_url, "ocap://");
}

bool MediaPlayerDLNA::isMpegTS() const {
  return ends_with(m_url, ".ts") || has_substring(m_url, "/vldms/") ||
         has_substring(m_url, "profile=MPEG_TS");
}

MediaPlayerDLNA::MediaPlayerDLNA(MediaPlayerClient* client)
    : m_playerClient(client)
    , m_hnsource(0)
    , m_source(0)
    , m_sink(0)
    , m_dfsink(0)
    , m_events(new ::MediaEvents(this))
    , m_events_sink(new ::MediaEvents(this))
    , m_changingRate(false)
    , m_endTime(std::numeric_limits<float>::infinity())
    , m_playerState(MediaPlayer::RMF_PLAYER_EMPTY)
    , m_videoState(MediaPlayer::RMF_VIDEO_BUFFER_HAVENOTHING)
    , m_isStreaming(false)
    , m_paused(true)
    , m_pausedInternal(true)
    , m_seeking(false)
    , m_playbackRate(1)
    , m_errorOccured(false)
    , m_mediaDuration(0)
    , m_delayingLoad(false)
    , m_mediaDurationKnown(false)
    , m_volumeTimerHandler(0)
    , m_muteTimerHandler(0)
    , m_muted(false)
    , m_lastKnownRect()
    , m_currentPosition(0)
    , m_isVOnlyOnce(true)
    , m_isAOnlyOnce(true)
    , m_isEndReached(false)
    , m_isInProgressRecording(false)
    , m_errorMsg("")
    , m_progressTimerHandler(0)
    , m_baseSeekTime(0.0)
    , m_currentProgressTime(0.0)
    , m_rmfMediaWarnDesc("")
    , m_rmfAudioTracks("")
    , m_rmfCaptionDescriptor("")
    , m_rmfEISSDataBuffer("")
    , m_rmfMediaWarnData(0)
    , m_networkBufferSize(0)
    , m_restartPlaybackCount(0)
    , m_allowPositionQueries(false)
    , m_EOSPending(false)
    , m_fetchTrickModeHeaders(false)
    , m_isVODPauseRequested(false)
    , m_isVODRateChangeRequested(false)
    , m_isVODSeekRequested(false)
    , m_isVODAsset(false)
    , m_isMusicOnlyAsset(true) /* It will be reset when Video Frame is found */
#ifdef ENABLE_DIRECT_QAM
    , m_isLiveAsset(false)
    , m_isPPVAsset(false)
    , m_isDVRAsset (false)
#endif
    , m_VODKeepPipelinePlaying(true)
    , m_eissFilterStatus(false)
    , m_onFirstVideoFrameHandler(0)
    , m_onFirstAudioFrameHandler(0)
    , m_currentAssetTypeHandler(0)
    , m_onEISSDUpdateHandler(0)
    , m_onMediaWarningReceivedHandler(0) {
  LOG_INFO("DLNA video player created");
  g_playerInstance = this;

  char *pVODKeepPipelinePlaying = NULL;
  pVODKeepPipelinePlaying = getenv("RMF_VOD_KEEP_PIPELINE");
  if (pVODKeepPipelinePlaying && (strcasecmp(pVODKeepPipelinePlaying, "FALSE") == 0)) {
    m_VODKeepPipelinePlaying = false;
  }
  m_maxTimeLoaded = 0.0f;  //CID 87555 - Initialized values
  m_videoTimerHandler = 0;
  m_audioTimerHandler = 0;
  m_startedBuffering = false;
  m_isVOD5XHackEnabled = true;
}

MediaPlayerDLNA::~MediaPlayerDLNA() {
  LOG_INFO("DLNA video player destroying");


  if (m_progressTimerHandler != 0)
    g_source_remove(m_progressTimerHandler);

  rmf_stop();

  m_playerClient = NULL;

  delete m_events;
  m_events = NULL;

  delete m_events_sink;
  m_events_sink = NULL;

  if (m_muteTimerHandler)
    g_source_remove(m_muteTimerHandler);

  if (m_volumeTimerHandler)
    g_source_remove(m_volumeTimerHandler);

  if (m_onFirstVideoFrameHandler)
    g_source_remove(m_onFirstVideoFrameHandler);

  if (m_onFirstAudioFrameHandler)
    g_source_remove(m_onFirstAudioFrameHandler);

  if (m_onMediaWarningReceivedHandler)
    g_source_remove(m_onMediaWarningReceivedHandler);

  if (m_onEISSDUpdateHandler)
    g_source_remove(m_onEISSDUpdateHandler);

  if (this == g_playerInstance)
    g_playerInstance = NULL;

  if (m_currentAssetTypeHandler)
    g_source_remove(m_currentAssetTypeHandler);

  ASSERT(!m_sink);
  ASSERT(!m_dfsink);
  ASSERT(!m_source);

  LOG_INFO("DLNA video player destroyed");
}

void MediaPlayerDLNA::notifyError(RMFResult err, const char *pMsg) {
  std::string tmp = "130:ErrorCode:Unknown Error";
  if (pMsg)
    tmp = std::string(pMsg);

  /* Set the Error msg first before posting the error */
#ifdef ENABLE_DIRECT_QAM
  getErrorMapping(err, pMsg);
#else
  setMediaErrorMessage(tmp);
#endif /* ENABLE_DIRECT_QAM */
  loadingFailed(MediaPlayer::RMF_PLAYER_DECODEERROR);
}

#ifdef ENABLE_DIRECT_QAM
void MediaPlayerDLNA::getErrorMapping(RMFResult err, const char *pMsg)
{
  std::string errorMsg = "Unknown Error";
  std::string inputErr = "Unknown Error";
  if (pMsg)
    inputErr = std::string(pMsg);

  if (m_isVODAsset)
    errorMsg = "130:Error:" + inputErr;
  else if (m_isLiveAsset | m_isPPVAsset)
  {
    std::string errorCode = "0";
    switch (err)
    {
      case RMF_QAMSRC_EVENT_CANNOT_DESCRAMBLE_ENTITLEMENT:
      case RMF_QAMSRC_EVENT_CANNOT_DESCRAMBLE_RESOURCES:
      case RMF_QAMSRC_EVENT_MMI_PURCHASE_DIALOG:
      case RMF_QAMSRC_EVENT_MMI_TECHNICAL_DIALOG:
      case RMF_QAMSRC_EVENT_POD_REMOVED:
      case RMF_QAMSRC_EVENT_POD_RESOURCE_LOST:
        errorCode = "101";
        break;

      case RMF_QAMSRC_ERROR_PAT_NOT_AVAILABLE:
      case RMF_QAMSRC_ERROR_PMT_NOT_AVAILABLE:
      case RMF_QAMSRC_ERROR_PROGRAM_NUMBER_INVALID:
      case RMF_QAMSRC_ERROR_PROGRAM_NUMBER_INVALID_ON_PAT_UPDATE:
        errorCode = "102";
        break;

      case RMF_QAMSRC_ERROR_UNRECOVERABLE_ERROR:
        errorCode = "103";
        break;

      case RMF_QAMSRC_ERROR_TUNER_NOT_LOCKED:
      case RMF_QAMSRC_ERROR_TUNE_FAILED:
        errorCode = "104";
        break;
      default:
        inputErr = "Service Unavailable";
        errorCode = "106";
        break;
    }
    errorMsg = errorCode + ":Error:" + inputErr;
  }
  else if (m_isDVRAsset)
  {
    //since its mDVR and HNSource is used, send the eror code as same as what you get.. :)
    errorMsg = inputErr;
  }

  LOG_INFO ("The Error String set is %s", errorMsg.c_str());
  setMediaErrorMessage(errorMsg);
}
#endif /* ENABLE_DIRECT_QAM */

void MediaPlayerDLNA::startProgressTimer(double interval) {

  if (m_progressTimerHandler)
    g_source_remove(m_progressTimerHandler);

  static const auto timerCallbackFunc = [](gpointer data) -> gboolean { MediaPlayerDLNA& self = *static_cast<MediaPlayerDLNA*>(data);
                                                                        self.m_progressTimerHandler = 0;
                                                                        self.onProgressTimerTimeout();
                                                                        return G_SOURCE_REMOVE;
                                                                      };

  m_progressTimerHandler = g_timeout_add( (guint) (interval * 1000), timerCallbackFunc, this);
}

void MediaPlayerDLNA::fetchHeaders() {
#ifdef USE_VODSRC
  startProgressTimer(1.0);
  return;
#else
  RequestInfo request;
  request.url = m_url;
  request.headers.push_back("getAvailableSeekRange.dlna.org: 1");
  fetchOnWorkerThread(request);
#endif /* USE_VODSRC */
}

void MediaPlayerDLNA::fetchOnWorkerThread(const RequestInfo& request) {
  // fetch on worker thread
  ResponseInfo response;
  fetchWithCurl(request, response);
  /*  Looks like there is no need of worker thread at this point.. If the worker thread is implemented, pl update rmf_load() for the 'm_isVOD5XHackEnabled' */
  onReplyFinished(response);
}

void MediaPlayerDLNA::fetchWithCurl(
    const RequestInfo& request, ResponseInfo& response) {
  LOG_VERBOSE("curl request for url: %s", request.url.c_str());
  LOG_VERBOSE("headers:");
  for (headers_t::const_iterator cit = request.headers.begin();
       cit != request.headers.end(); ++cit) {
    const std::string& header = *cit;
    LOG_VERBOSE("\t%s", header.c_str());
  }

  CURLcode res = CURLE_OK;
  CURL *curl_handle = NULL;

  std::string headers_buf;
  std::string payload_buf;

  curl_handle = curl_easy_init();
  CURL_EASY_SETOPT(curl_handle, CURLOPT_URL, request.url.c_str());
  CURL_EASY_SETOPT(curl_handle, CURLOPT_FOLLOWLOCATION, 1);
  CURL_EASY_SETOPT(curl_handle, CURLOPT_CONNECTTIMEOUT, kCurlConnectTimeoutInSeconds);
  CURL_EASY_SETOPT(curl_handle, CURLOPT_TIMEOUT, kCurlTimeoutInSeconds);
  CURL_EASY_SETOPT(curl_handle, CURLOPT_HEADERFUNCTION, write_callback);
  CURL_EASY_SETOPT(curl_handle, CURLOPT_HEADERDATA,
                   reinterpret_cast<void *>(&headers_buf));
  // We're not interested in body, but if we do
  // curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
  // curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&payload_buf);
  // curl_easy_setopt(curl_handle, CURLOPT_HTTPGET, 1L);
  CURL_EASY_SETOPT(curl_handle, CURLOPT_NOBODY, 1);
  CURL_EASY_SETOPT(curl_handle, CURLOPT_HEADER, 1);
  CURL_EASY_SETOPT(curl_handle, CURLOPT_NOPROGRESS, 1);
  CURL_EASY_SETOPT(curl_handle, CURLOPT_NOSIGNAL, 1);
  // curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1);

  if (!request.headers.empty()) {
    struct curl_slist *headers = NULL;
    for (headers_t::const_iterator cit = request.headers.begin();
         cit != request.headers.end(); ++cit) {
      const std::string& header = *cit;
      headers = curl_slist_append(headers, header.c_str());
    }
    CURL_EASY_SETOPT(curl_handle, CURLOPT_HTTPHEADER, headers);
  }

  res = curl_easy_perform(curl_handle);

  if (res != CURLE_OK) {
    LOG_ERROR("curl failed with result: %d", res);
    response.http_code = -1;
    curl_easy_cleanup(curl_handle);
    return;
  }

  long httpCode = -1;
  if (CURLE_OK == curl_easy_getinfo(curl_handle, _CURLINFO_RESPONSE_CODE, &httpCode))
    response.http_code = httpCode;
  else
    response.http_code = -1;

  response.url = request.url;
  response.payload = payload_buf;
  response.headers = split_string(headers_buf, '\n', response.headers);

  LOG_VERBOSE("curl got response for url: %s", response.url.c_str());
  LOG_VERBOSE("headers:");
  for (headers_t::const_iterator cit = response.headers.begin();
       cit != response.headers.end(); ++cit) {
    const std::string& header = *cit;
    LOG_VERBOSE("\t%s", header.c_str());
  }

  curl_easy_cleanup(curl_handle);
}

void MediaPlayerDLNA::time_to_hms(
    float time, unsigned& h, unsigned& m, float& s) {
  if (time < 0)
    time = 0;

  h = time / 3600;
  m = (time - h * 3600) / 60;
  s = time - h * 3600 - m * 60;
}

void MediaPlayerDLNA::fetchHeadersForTrickMode(float speed, double pos) {
#ifdef USE_VODSRC
  (void)speed;
  (void)pos;
  return;
#else
  m_fetchTrickModeHeaders = true;

  /* reset the flag before sending the speed command */
  if (speed != 0.0)
  {
    LOG_INFO("MediaPlayerDLNA: fetchHeadersForTrickMode speed change request. So reset the paused flag if any..");
    m_isVODPauseRequested = false;
  }

  char speed_str[64];
  char time_str[64];

  snprintf(speed_str, sizeof(speed_str), "speed=%f", speed);
  LOG_INFO("MediaPlayerDLNA: fetchHeadersForTrickMode speed: [%s]", speed_str);

  unsigned h = 0, m = 0;
  float s = 0;
  time_to_hms(pos, h, m, s);

  snprintf(time_str, sizeof(time_str), "npt=%02u:%02u:%.2f-", h, m, s);
  LOG_INFO("MediaPlayerDLNA: fetchHeadersForTrickMode position: [%s] (%f)", time_str, pos);

  RequestInfo request;
  request.url = m_url;
  request.headers.push_back(std::string("getAvailableSeekRange.dlna.org: 1"));
  request.headers.push_back(std::string("PlaySpeed.dlna.org: ") + speed_str);
  request.headers.push_back(std::string("TimeSeekRange.dlna.org: ") + time_str);

  fetchOnWorkerThread(request);
#endif /* USE_VODSRC */
}

void MediaPlayerDLNA::onReplyError() {
  setMediaErrorMessage("VOD:: MediaStreamer is not reachable");
  loadingFailed(MediaPlayer::RMF_PLAYER_NETWORKERROR);
  startProgressTimer(0);
}

void MediaPlayerDLNA::onReplyFinished(const ResponseInfo& response) {
  const headers_t& headers = response.headers;
  LOG_TRACE("Load finished with http_code: %d", response.http_code);
  if (response.http_code < 0) {
    onReplyError();
    return;
  }

  if (m_progressTimerHandler != 0)
    g_source_remove(m_progressTimerHandler);

  std::string seek_header;
  std::string speed_header;
  std::string adHeader;
  find_header(headers, HEADER_NAME_VODADINSERTION_STATUS, adHeader);
  if (!adHeader.empty()) {
    LOG_INFO ("Ad_header: %s", adHeader.c_str());
    /* The response will be either "started" or "completed" */
    if (std::string::npos != adHeader.find("started"))
    {
      LOG_INFO ("Ad is being played; could not honor the trick request");
      if (1.0 != m_playbackRate)
      {
        LOG_INFO ("Ad Started during trick it seems; switch to 1.0f");
        m_playbackRate = 1.0f;
        m_playerClient->rateChanged();
      }
      m_rmfMediaWarnDesc = std::string("TRICK_MODE_NOT_ALLOWED");
      m_rmfMediaWarnData = 1; /* Speed */
      m_playerClient->mediaWarningReceived();

      if ((m_paused) && (m_source))
      {
        float speed = 0.0; /* Pause is called internally when speed is called with 0 */
        double time = 0.0; /* Time is not used when the speed is 0.0; */
        g_print ("Current state is paused; So update the hnsource for the timeouts..\n");
        m_isVODPauseRequested = true;
        m_source->play (speed, time);
      }
    }
    else
      LOG_TRACE ("No Opp as og now for this ad insertion  %s", adHeader.c_str());
  } else {
      bool postTimeChanged = true;
      bool postRateChanged = false;
      unsigned int seekTime = 0;
    //backport changes from qtwebkit/Source/WebCore/platform/graphics/dlna/MediaPlayerPrivateDLNA.cpp
    find_header(headers, HEADER_NAME_SEEK_RANGE, seek_header);
    if (!seek_header.empty() && (!m_EOSPending)) {

      LOG_INFO("seek_header: %s", seek_header.c_str());
      // Split the response to get the NPT value

      std::size_t foundEq = seek_header.find('=');
      if (foundEq != std::string::npos) {
        std::vector<std::string> first_split;
        split_string(seek_header, '=', first_split);
        LOG_TRACE("first_split ok");
        std::string npt_str = first_split.at(1);
        LOG_INFO("npt_str: %s", npt_str.c_str());
        // The start time is in the format of hh:mm:ss per DLNA standard.

        std::size_t foundColEq = npt_str.find(':');
        if (foundColEq != std::string::npos) {
          std::vector<std::string> second_split;
          split_string(npt_str, ':', second_split);
          // The length must be 3 bcoz the format hh:mm:ss per standard
          if (3 == second_split.size()) {
            LOG_TRACE("second_split ok");
            int hour   = atoi(second_split.at(0).c_str());
            int minute = atoi(second_split.at(1).c_str());
            int second = atoi(second_split.at(2).c_str());
            LOG_TRACE("hour: %d, min: %d, second: %d", hour, minute, second);
            seekTime = (hour * 3600) + (minute * 60) + second;
            LOG_INFO ("MediaPlayerDLNA:: Received lastStartNPT is %u", seekTime);

            if (m_VODKeepPipelinePlaying) {
              if (m_isVODAsset && (seekTime == (RMF_VOD_BAD_START_POSITION_VAL/1000u)))
              {
                LOG_INFO("MediaPlayerDLNA:: VOD EOS Pending");
                m_EOSPending = true;
              }
            }
          }
        }
      }
    }
    /***/
    find_header(headers, HEADER_NAME_PLAYSPEED, speed_header);
    if (!speed_header.empty() && (!m_EOSPending))
    {
      LOG_TRACE("speed_header: %s", speed_header.c_str());

      std::size_t foundEq = speed_header.find('=');
      if (foundEq != std::string::npos) {
        std::vector<std::string> speed_vec;
        split_string(speed_header, '=', speed_vec);
        std::string new_param = speed_vec.at(1);
        LOG_INFO ("MediaPlayerDLNA:: Header Response:: Speed Header is = %s\n", new_param.c_str());

        /* Reset the progressTime as it is going to restart the timer now with new speed */
        m_currentProgressTime = 0;
        float speed = atof(new_param.c_str());


        if (!m_isVOD5XHackEnabled)
        {
          if (0 == speed)
          {
            m_paused = true;
            speed = 1.0;
          }
          else
            m_paused = false;

          if (m_playbackRate != speed)
          {
            m_playbackRate = speed;
            postRateChanged = true;
          }

          LOG_INFO ("MediaPlayerDLNA:: Header Response:: Asset [VOD] speed: [%f]\n", m_playbackRate);
        }
        else
        {
          if ((m_isVODPauseRequested) && (0 == speed))
          {
             speed = 1.0;
             m_paused = true;
             if (speed != m_playbackRate)
             {
                m_playbackRate = speed;
                postRateChanged = true;
             }
           }
           else if ((m_isVODPauseRequested) && (0 != speed))
           {
              speed = 1.0;
              m_paused = true;
              if (speed != m_playbackRate)
              {
                 m_playbackRate = speed;
                 postRateChanged = true;
              }
              postTimeChanged = false;
              LOG_WARNING("MediaPlayerPrivateDLNA:: Header Response:: Ignore this [%s] response as it is invalid at this point in time", new_param.c_str());
          }
          else
          {
              m_paused = false;
              if (0 == speed)
              {
                 LOG_WARNING("MediaPlayerPrivateDLNA:: Header Response received for the Pause request after we sent other trick mode command.");
                 speed = 1.0;
                 postTimeChanged = false;
              }
              else if ((m_isVODSeekRequested) && (speed != 1.0))
              {
                 LOG_WARNING("MediaPlayerPrivateDLNA:: Header Response received for the trick modes after the seek command sent.");
                 speed = 1.0;
                 postTimeChanged = false;
              }
              else
              {
                 m_isVODSeekRequested = false;
                 if (speed != m_playbackRate)
                 {
                    m_playbackRate = speed;
                    postRateChanged = true;
                 }
                 LOG_WARNING("MediaPlayerPrivateDLNA:: Header Response:: Asset [VOD] speed: [%f]\n", m_playbackRate);
              }
          }
        }
      }
    }
    /* Update the time & post a event to WebKit */
    if(postTimeChanged)
    {
       /* Update the Base Time as it is associated with the speed */
       m_baseSeekTime = seekTime;
       m_playerClient->timeChanged();
    }

    if (postRateChanged)
        m_playerClient->rateChanged();
  }

  if (!m_paused) {
    startProgressTimer(1.0);
  }
}

void MediaPlayerDLNA::onProgressTimerTimeout() {
  m_currentProgressTime += m_playbackRate;

  LOG_VERBOSE("MediaPlayerDLNA::onProgressTimerTimeout: %f", m_currentProgressTime);

  // This logic may be extended for Live and DVR also; But for VOD; the rate is
  // 8.0 by the vod server; so to avoid wrong calculation, use conditional check
  if (m_isVODAsset) {
    if ((m_playbackRate == 4.0) || (m_playbackRate == -4.0))
      m_currentProgressTime += m_playbackRate;
  }

  startProgressTimer(1.0);

  if ((m_playbackRate < 0) && (playbackPosition() == 0)) {
    m_restartPlaybackCount++;
    if (m_restartPlaybackCount > RMF_VOD_BEGIN_PLAYBACK) {
      m_restartPlaybackCount = 0;
      ended();
    }
  }
}

bool MediaPlayerDLNA::rmf_load(const std::string& httpUrl) {
  LOG_INFO("LOAD IS CALLED (%s)", httpUrl.c_str());

  /* As of now the VOD Head Requested for the trick mode are handled synchronously using curl request; Refer `fetchOnWorkerThread() `*/
#if 1
  m_isVOD5XHackEnabled = false;
#else
  char *pVOD5XHack = NULL;
  pVOD5XHack = getenv("RMF_VOD_5X_MITIGATION");
  if (pVOD5XHack && (strcasecmp(pVOD5XHack, "TRUE") == 0)) 
  {
      LOG_WARNING("RMF_VOD_5X_MITIGATION is set to TRUE..\n");
      m_isVOD5XHackEnabled = true;;
  }
#endif

  if (httpUrl.empty()) {
      LOG_ERROR("Empty URL, skipping RMF pipeline construction");
      return false;
  }

  if (m_source) {
    LOG_ERROR("It seems like, It is loaded was already but not stopped yet. "
              "This is under worst case.. Just return or call stop internally");
    rmf_stop();
    return false;
  }

  m_url = httpUrl;
  /* Set it to TRUE as a backup */
  m_isMusicOnlyAsset = true;

  char url[1024];
#ifdef ENABLE_DIRECT_QAM
  char *p;
  char ocapUrl[32];
  uint c, len;
  if (m_url.find("ocap://") != std::string::npos) {
    strcpy(url, m_url.c_str());
    p= strstr(url,"ocap://0x");
    len= strlen("ocap://0x");
    c= p[len];
    while(((c >= '0') && (c <= '9')) ||
          ((c >= 'a') && (c <= 'f')) ||
          ((c >= 'A') && (c <= 'F')))
    {
      c= p[++len];
      if (len >= (sizeof(ocapUrl) - 1))
        break;
    }
    strncpy(ocapUrl, p, len);
    ocapUrl[len]= '\0';
    LOG_INFO("QAM service: %s", ocapUrl);

    m_hnsource = NULL;
    if (true == RMFQAMSrc::useFactoryMethods()) {
      m_source = RMFQAMSrc::getQAMSourceInstance(ocapUrl);
    } else {
      m_source = new RMFQAMSrc();
    }
  }
#ifdef IPPV_CLIENT_ENABLED
  else if (m_url.find("ippv://") != std::string::npos) {
    strcpy(url, m_url.c_str());
    p= strstr(url,"ippv://0x");
    len= strlen("ippv://0x");
    c= p[len];
    while(((c >= '0') && (c <= '9')) ||
          ((c >= 'a') && (c <= 'f')) ||
          ((c >= 'A') && (c <= 'F'))) {
      c= p[++len];
      if (len >= (sizeof(ocapUrl) - 1))
        break;
    }
    strncpy(ocapUrl, p, len);
    ocapUrl[len]= '\0';
    LOG_INFO("IPPV service: %s", ocapUrl);
    strcpy(url, ocapUrl);
    m_hnsource = NULL;
    m_source = new RMFiPPVSrc();
  }
#endif
#ifdef USE_VODSRC
  else if (m_url.find("vod://") != std::string::npos) {
    strcpy(url, m_url.c_str());
    p= strstr(url,"vod://");
    char *tempstr = strchr(p,'&');
    //char *tempstr1 = strchr(p,'\0');
    if(tempstr != NULL) {
      strncpy(ocapUrl, p, tempstr-p);
      ocapUrl[tempstr-p]= '\0';
    } else {
      strcpy(ocapUrl,p);
    }
    LOG_INFO("vod service: %s", ocapUrl);
    strcpy(url, ocapUrl);
    m_hnsource = NULL;
    m_source = new RMFVODSrc();
    m_VODKeepPipelinePlaying = false;
  }
#endif
  else {
#endif

    strcpy(url, m_url.c_str());
    m_source = new HNSource();
    m_hnsource = dynamic_cast<HNSource *>(m_source);

#ifdef ENABLE_DIRECT_QAM
  }
#endif

  if(NULL == m_source)  {
    LOG_ERROR("Source create failed");
    m_videoState = MediaPlayer::RMF_VIDEO_BUFFER_HAVENOTHING;
    notifyError(0, "Service unavailable");
    return false;
  }

  if (m_url.find("vod://") != std::string::npos) {
    m_isVODAsset = true;
  }
#ifdef ENABLE_DIRECT_QAM
  else if (m_url.find("ocap://") != std::string::npos) {
    m_isLiveAsset = true;
  }
  else if (m_url.find("ippv://") != std::string::npos) {
    m_isPPVAsset = true;
  }
  else /* This plugin can play ocap://, vod://, ppv:// and DVR ONLY. so if none of the above, we can consider that it is of DVR */
    m_isDVRAsset = true;
#endif

  if (m_source->init() != RMF_RESULT_SUCCESS) {
    m_errorMsg = "Failed to initialize source";
    m_videoState = MediaPlayer::RMF_VIDEO_BUFFER_HAVENOTHING;
    LOG_ERROR("%s", m_errorMsg.c_str());
    loadingFailed(MediaPlayer::RMF_PLAYER_DECODEERROR);
    return false;
  }

  m_source->setEvents(m_events);

  if(m_source->open(url, NULL) != RMF_RESULT_SUCCESS) {
    m_errorMsg = "Failed to open source";
    m_videoState = MediaPlayer::RMF_VIDEO_BUFFER_HAVENOTHING;
    LOG_ERROR("%s", m_errorMsg.c_str());
    loadingFailed(MediaPlayer::RMF_PLAYER_DECODEERROR);
    return false;
  }

  m_sink = new MediaPlayerSink();

  if(NULL == m_sink)  {
    m_errorMsg = "Sink create failed";
    m_videoState = MediaPlayer::RMF_VIDEO_BUFFER_HAVENOTHING;
    LOG_ERROR("%s", m_errorMsg.c_str());
    loadingFailed(MediaPlayer::RMF_PLAYER_DECODEERROR);
    return false;
  }

  if (m_sink->init() != RMF_RESULT_SUCCESS) {
    m_errorMsg = "Failed to initialize video sink";
    m_videoState = MediaPlayer::RMF_VIDEO_BUFFER_HAVENOTHING;
    LOG_ERROR("%s", m_errorMsg.c_str());
    loadingFailed(MediaPlayer::RMF_PLAYER_DECODEERROR);
    return false;
  }
    m_sink->setEvents(m_events_sink);

  if (getenv("USE_SINK_DUMP")) {
    LOG_INFO("######################### Adding dump file sink ##########################");
    m_dfsink = new DumpFileSink();
  }

  if (m_dfsink) {
    char url[1024];
    char ipaddr[20];
    char location[100];
    snprintf(url, sizeof(url), "%s", m_url.c_str());
    snprintf(ipaddr, sizeof(ipaddr), "127.0.0.1");
    char* ip_start_pos = strstr(url, "http://") + strlen("http://");
    if (NULL != ip_start_pos) {
      char* ip_end_pos = strstr(ip_start_pos, ":");
      if (NULL != ip_end_pos) {
        strncpy(ipaddr, ip_start_pos, ip_end_pos - ip_start_pos);
        ipaddr[ip_end_pos - ip_start_pos] = 0;
      }
    }
    snprintf(location, sizeof(location), "/opt/SNK_IN_%s.ts", ipaddr);
    if (m_dfsink->init() != RMF_RESULT_SUCCESS) {
      LOG_ERROR("Failed to initialize fd sink");
      return false;
    }
    m_dfsink->setLocation(location);
    m_dfsink->setSource(m_source);
  }

  m_sink->setSource(m_source);
  m_sink->setHaveVideoCallback(mediaPlayerPrivateHaveVideoCallback, this);
  m_sink->setHaveAudioCallback(mediaPlayerPrivateHaveAudioCallback, this);
  m_sink->setEISSDataCallback(mediaPlayerPrivateEISSDataCallback, this);
  m_sink->setVideoPlayingCallback(mediaPlayerPrivateVideoNotifyFirstFrameCallback, this);
  m_sink->setAudioPlayingCallback(mediaPlayerPrivateAudioNotifyFirstFrameCallback, this);
  m_sink->setMediaWarningCallback(mediaPlayerPrivateNotifyMediaWarningCallback, this);

#ifdef USE_VODSRC
  if (m_isVODAsset) {
    m_source->setSpeed(1.0f);
    m_source->setMediaTime(0.0);
  }
#endif /* USE_VODSRC */

  if (!m_delayingLoad)
    commitLoad();
  else
    LOG_INFO("Not committing load");

  LOG_INFO("LOAD FINISHED");
  return true;
}

void MediaPlayerDLNA::commitLoad() {
  ASSERT(!m_delayingLoad);
  LOG_INFO("Committing load.");
  updateStates();
}

float MediaPlayerDLNA::playbackPosition() const {
  if (m_isVODAsset) {
    float pos = m_baseSeekTime + m_currentProgressTime;
    if (pos >= 0)
      return pos;
    else
      return 0.f;
  } else {
    double pos = m_currentPosition;
#ifdef ENABLE_DIRECT_QAM
    if (!m_isDVRAsset) {
      if ((m_sink) && (m_sink->getMediaTime(pos) != RMF_RESULT_SUCCESS)) {
        LOG_ERROR("Failed to get playback position, return previous: %f", m_currentPosition);
        return m_currentPosition;
      }
    } else {
#endif  /* ENABLE_DIRECT_QAM */
      if (m_allowPositionQueries && (!m_source || m_source->getMediaTime(pos) != RMF_RESULT_SUCCESS)) {
        LOG_ERROR("Failed to get playback position, return previous: %f", m_currentPosition);
        return m_currentPosition;
      }
#ifdef ENABLE_DIRECT_QAM
    }
#endif  /* ENABLE_DIRECT_QAM */
    return static_cast<float>(pos);
  }
}

void MediaPlayerDLNA::prepareToPlay() {
  if (m_delayingLoad) {
    m_delayingLoad = false;
    commitLoad();
  }
}

void MediaPlayerDLNA::rmf_play() {
  LOG_INFO("PLAY IS CALLED");

  if (!m_source) {
    LOG_WARNING("No source, skip play.");
    return;
  }

  m_isEndReached = false;
  m_paused = false;

  if ((m_networkBufferSize) && (m_isVOnlyOnce)) {
    m_sink->setNetWorkBufferSize(m_networkBufferSize);
  }

  if (m_VODKeepPipelinePlaying) {
    if (m_isVODAsset) {
      if (m_EOSPending) {
        LOG_WARNING("Play cannot be done now as the VOD Server posted that it is sending EOS shortly");
        return;
      }
      if (!m_isAOnlyOnce && m_allowPositionQueries)
        fetchHeadersForTrickMode(rmf_getRate(), m_baseSeekTime);
    }
  }

  if (m_source->play() == RMF_RESULT_SUCCESS) {
    LOG_INFO("Play");

    /* Update the time at which the asset was paused */
    m_currentPosition = rmf_getCurrentTime();
    m_pausedInternal = false;
  } else {
    LOG_ERROR("Play failed for some unknown reason..");
    loadingFailed(MediaPlayer::RMF_PLAYER_EMPTY);
  }

#ifdef IPPV_CLIENT_ENABLED
  if (m_isPPVAsset) {
    unsigned int eIdValue = 0x00;
    char url[1024];
    char *p;
    uint c, len;
    char eid[32];

    strcpy(url, m_url.c_str());
    p= strstr(url,"token=");
    len= 6;
    if (p != NULL) {
      c= p[len];
      while ((c >= '0') && (c <= '9')) {
        c= p[++len];
        if (len >= (sizeof(eid) - 1))
          break;
      }
      strncpy(eid, p+6, len-6);
      eid[len-6]= '\0';
      eIdValue = atoi(eid);
      LOG_INFO("play:: ippv asset: EID='%s',%d", eid, eIdValue);
    }

    if (0x00 != eIdValue) {
      int rc = ((RMFiPPVSrc*)m_source)->purchasePPVEvent( eIdValue );
      if ( 0 != rc ) {
        LOG_ERROR("PPV event purchase failed");
        /* Handle error */
      } else {
        LOG_INFO("PPV event purchase succeeded");
      }
    } else {
      LOG_INFO("Already purchased PPV event");
    }
  }
#endif //IPPV_CLIENT_ENABLED
}

void MediaPlayerDLNA::rmf_pause() {
  LOG_INFO("PAUSE IS CALLED");
  if (!m_source)
    return;
  if (m_isEndReached)
    return;

#ifdef ENABLE_DIRECT_QAM
  /* Trick mode is allowed only for VOD and mDVR */
  if (!(m_isDVRAsset | m_isVODAsset))
    return;
#endif /* ENABLE_DIRECT_QAM */

  if (!m_pausedInternal) {
    float speed = 0.0;  // Pause is called internally when speed is called with 0
    double time = 0.0;  // Time is not used when the speed is 0.0

    if (m_VODKeepPipelinePlaying) {
      if (m_isVODAsset) {
        if (m_EOSPending) {
          LOG_WARNING("Pause cannot be done now as the VOD Server posted that it is sending EOS shortly");
          return;
        }

        if (!m_isAOnlyOnce)
        {
          m_isVODPauseRequested = true;
          fetchHeadersForTrickMode(speed, time);
        }
      }
    }

#ifdef USE_VODSRC
    if (m_isVODAsset) {
      m_source->setSpeed (speed);
      m_source->setMediaTime (time);
    } else {
#endif /* USE_VODSRC */

    if (m_source->play(speed, time) == RMF_RESULT_SUCCESS)
      LOG_INFO("Pause");

    if (m_progressTimerHandler != 0)
      g_source_remove(m_progressTimerHandler);

#ifdef USE_VODSRC
    }
#endif /* USE_VODSRC*/

    if (m_isVODAsset) {
#ifdef USE_VODSRC
      double npt = 0.00;
      m_source->getMediaTime(npt);
      m_baseSeekTime = (long long) npt/1000;
#else
      m_baseSeekTime = playbackPosition();
#endif /* USE_VODSRC */
      m_currentProgressTime = 0;
    }

    m_paused = true;
  } else {
    LOG_INFO("Pause Ignored. We are still in the process of trick mode...");
  }
}

void MediaPlayerDLNA::rmf_stop() {
  LOG_INFO("STOP IS CALLED");
  if (!m_sink || !m_source)
    return;

  g_source_remove(m_progressTimerHandler);
  m_allowPositionQueries = false;
  m_source->setEvents(NULL);

  m_sink->setMediaWarningCallback(NULL, NULL);
  m_sink->setAudioPlayingCallback(NULL, NULL);
  m_sink->setVideoPlayingCallback(NULL, NULL);
  m_sink->setEISSDataCallback(NULL, NULL);
  m_sink->setHaveAudioCallback(NULL, NULL);
  m_sink->setHaveVideoCallback(NULL, NULL);

  m_sink->setEvents(NULL);
  m_source->removeEventHandler(m_events);
#ifdef ENABLE_DIRECT_QAM
  m_source->stop();
  m_source->close();
#endif /* ENABLE_DIRECT_QAM */
  m_source->term();

#ifdef ENABLE_DIRECT_QAM
  if (m_isLiveAsset) {
    RMFQAMSrc *qamSrc= dynamic_cast<RMFQAMSrc *>(m_source);
    if ( qamSrc && (true == RMFQAMSrc::useFactoryMethods())) {
      //qamSrc->pause();
      RMFQAMSrc::freeQAMSourceInstance( qamSrc );
    }
  } else {
    delete m_source;
  }
#else /* ENABLE_DIRECT_QAM */
  delete m_source;
  m_hnsource = NULL;
#endif /* ENABLE_DIRECT_QAM */
    if (m_sink)
    {
        m_sink->term();
        delete m_sink;
    }

    if(m_dfsink)
    {
        m_dfsink->term();
        delete m_dfsink;
    }
  m_dfsink = NULL;
  m_sink = NULL;
  m_source = NULL;

  m_paused = true;

  LOG_INFO("Stop");
}

float MediaPlayerDLNA::rmf_getDuration() const {
  if (!m_source)
    return 0.0f;

  if (m_errorOccured)
    return 0.0f;

  // Media duration query failed already, don't attempt new useless queries.
  if (!m_mediaDurationKnown)
    return std::numeric_limits<float>::infinity();

  return m_mediaDuration;
}

float MediaPlayerDLNA::rmf_getCurrentTime() const {
  if (m_errorOccured)
    return 0.0f;

  return playbackPosition();
}

unsigned long MediaPlayerDLNA::rmf_getCCDecoderHandle() const {
  if (m_sink)
    return m_sink->getVideoDecoderHandle();

  return 0;
}

std::string MediaPlayerDLNA::rmf_getAudioLanguages() const
{
    if (!m_sink)
        return "eng";
    return m_sink->getAudioLanguages();
}

void MediaPlayerDLNA::rmf_setAudioLanguage(const std::string& audioLang) {
  if (m_sink)
    m_sink->setAudioLanguage(audioLang.c_str());

  return;
}

void MediaPlayerDLNA::rmf_setAuxiliaryAudioLang(const std::string& audioLang) {
  if (m_sink)
    m_sink->setAuxiliaryAudioLang(audioLang.c_str());

  return;
}

void MediaPlayerDLNA::rmf_setEissFilterStatus (bool eissStatus)
{
    m_eissFilterStatus = eissStatus;
    if(m_sink)
        m_sink->setEissFilterStatus (eissStatus);

    return;
}

void MediaPlayerDLNA::rmf_setVideoZoom(unsigned short zoomVal) {
  if (m_sink)
    m_sink->setVideoZoom(zoomVal);

  return;
}

void MediaPlayerDLNA::rmf_setVideoBufferLength(float bufferLength) {
  m_mediaDuration = bufferLength;
  if (m_hnsource)
    m_hnsource->setVideoLength(m_mediaDuration);
}

void MediaPlayerDLNA::rmf_setInProgressRecording(bool isInProgress) {
  m_isInProgressRecording = isInProgress;
}

void MediaPlayerDLNA::setMediaErrorMessage(const std::string& errorMsg) {
  m_errorMsg = errorMsg;

  if (m_VODKeepPipelinePlaying) {
    if (m_isVODAsset && (m_errorMsg == "VOD-Pause-10Min-Timeout")) {
      LOG_INFO("MediaPlayerDLNA::VOD EOS Pending\n");
      m_EOSPending = true;
    }
  }

  return;
}

// This is to seek to the live point of Linear; must be used for linear only
void MediaPlayerDLNA::rmf_seekToLivePosition(void) {
  if (!m_source) {
    LOG_INFO("No source, skip seekToLivePosition.");
    return;
  }
#ifdef ENABLE_DIRECT_QAM
  /* Trick mode is allowed only for VOD and mDVR */
  if (!(m_isDVRAsset | m_isVODAsset))
    return;
#endif /* ENABLE_DIRECT_QAM */

  m_playbackRate = 1.0f;
  m_pausedInternal = false;
  /* Update the position as 3 sec less than the duration */
  m_currentPosition = m_mediaDuration - 3.0;
  m_allowPositionQueries = false;
  if (isOCAP())
    m_hnsource->playAtLivePosition(m_mediaDuration);
  else
    m_hnsource->setMediaTime(m_currentPosition);

  return;
}

// This is to seek to the beginning of the VOD asset; as of for VOD only. can be
// extended for DVR as well
void MediaPlayerDLNA::rmf_seekToStartPosition(void) {
  if (!m_source) {
    LOG_INFO("No source, skip seekToStartPosition.");
    return;
  }

  m_playbackRate = 1.0f;
  m_currentPosition = 0.0;

  if (m_isVODAsset) {
    m_currentProgressTime = 0;
    m_baseSeekTime = 0.0;

    if (m_VODKeepPipelinePlaying) {
      m_fetchTrickModeHeaders = false;
    }

    if (m_progressTimerHandler != 0)
      g_source_remove(m_progressTimerHandler);
  }

  m_allowPositionQueries = false; // Will ensure that redundant position requests are avoided
  m_pausedInternal = false;
#ifdef USE_VODSRC
  m_source->setSpeed (m_playbackRate);
  m_source->setMediaTime(m_currentPosition);
  /* timer is stopped at ended */
  startProgressTimer(1.0);
#else
  if (m_isVODAsset) {
    m_hnsource->playAtLivePosition(m_currentPosition);
  }
  else
  {
    m_source->setMediaTime(m_currentPosition);
  }
#endif /* USE_VODSRC */
  return;
}

std::string MediaPlayerDLNA::rmf_getMediaErrorMessage() const {
  return m_errorMsg;
}

std::string MediaPlayerDLNA::rmf_getMediaWarnDescription() const
{
    return m_rmfMediaWarnDesc;
}

std::string MediaPlayerDLNA::rmf_getAvailableAudioTracks() const
{
  if (m_rmfAudioTracks.empty())
    m_rmfAudioTracks = m_sink->getAudioLanguages();

  /* reset m_rmfAudioTracks when PMT changes. */
  return m_rmfAudioTracks;
}

std::string MediaPlayerDLNA::rmf_getCaptionDescriptor() const
{
    if (m_rmfCaptionDescriptor.empty() && m_sink != nullptr)
        m_rmfCaptionDescriptor = m_sink->getCCDescriptor();

    /* reset m_rmfCaptionDescriptor when PMT changes. */
    return m_rmfCaptionDescriptor;
}

std::string MediaPlayerDLNA::rmf_getEISSDataBuffer() const
{
    if (m_sink != nullptr)
        m_rmfEISSDataBuffer = m_sink->getEISSData();

    return m_rmfEISSDataBuffer;
}
int MediaPlayerDLNA::rmf_getMediaWarnData() const
{
    return m_rmfMediaWarnData;
}

void MediaPlayerDLNA::rmf_setNetworkBufferSize(int bufferSize) {
  m_networkBufferSize = bufferSize;
  return;
}

void MediaPlayerDLNA::rmf_seek(float time)
{
    LOG_INFO("SEEK(%.2f)", time);

    if (m_isEndReached) {
        LOG_INFO("Ignore the seek() as it is being called after receiving EOS.. Duration = %f", rmf_getDuration());
        return;
    }

#ifdef ENABLE_DIRECT_QAM
    /* Trick mode is allowed only for VOD and mDVR */
    if (!(m_isDVRAsset | m_isVODAsset))
      return;
#endif /* ENABLE_DIRECT_QAM */

    // Avoid useless seeking.
    if (time == playbackPosition())
        return;

    if (m_VODKeepPipelinePlaying) {
      if (m_isVODAsset && (m_paused == true)) {
        m_pausedInternal = true;
      }
    }

    m_paused = false;

    if (!m_source) {
        LOG_INFO("No source, not seeking");
        return;
    }

    if (m_errorOccured) {
        LOG_INFO("Error occurred, not seeking");
        return;
    }

    if (m_VODKeepPipelinePlaying && m_isVODAsset) {
      if (m_EOSPending) {
        LOG_INFO("Seek cannot be done now as the VOD Server posted that it is sending EOS shortly");
        return;
      }

      if (!m_isAOnlyOnce)  {
        /* Below is not needed as the m_isVODSeekRequested flag will handle the stuff in the Header Response*/
        if (!m_isVOD5XHackEnabled) {
           fetchHeadersForTrickMode(1.0f, time);
        } else {
           if (m_isVODPauseRequested && m_isVODRateChangeRequested)
             m_isVODSeekRequested = true;
           else
             m_isVODSeekRequested = false;

           fetchHeadersForTrickMode(1.0f, time);
        }
      }
    } else {
      m_currentPosition = time;
      m_allowPositionQueries = false; // Will ensure that redundant position requests are avoided
    }

#ifdef USE_VODSRC
    double fTime = time;
    if (m_isVODAsset)
        fTime = time * 1000;
    if (m_source->setMediaTime(fTime) == RMF_RESULT_SUCCESS)
#else
    if (m_source->setMediaTime(time) == RMF_RESULT_SUCCESS)
#endif /* USE_VODSRC */
    {
        if (m_VODKeepPipelinePlaying) {
          if (m_isVODAsset) {
            m_currentPosition = rmf_getCurrentTime();
            m_seeking = false;
            m_playerClient->timeChanged();
            m_pausedInternal = false;
          } else {
            m_currentPosition = time;
            LOG_INFO("m_seeking = true");
            m_seeking = true;
            m_pausedInternal = false;
            m_currentProgressTime = 0;
            m_baseSeekTime = time;
            if (m_progressTimerHandler != 0)
              g_source_remove(m_progressTimerHandler);

            /* Post a Rate Change event */
            if (1.0f != m_playbackRate) {
              m_playbackRate = 1.0f;
              m_playerClient->rateChanged();
            }
          }
        } else {
          m_currentPosition = time;
          LOG_INFO("m_seeking = true");
          m_seeking = true;
          m_currentProgressTime = 0;
          m_baseSeekTime = time;
          m_pausedInternal = false;
          /* Post a Rate Change event */
          if (1.0f != m_playbackRate) {
            m_playbackRate = 1.0f;
            m_playerClient->rateChanged();
          }

#ifdef USE_VODSRC
          double npt = 0.00;
          m_source->getMediaTime(npt);
          m_baseSeekTime = (long long) npt/1000;
          LOG_INFO("BaseTime that we received is = %f", m_baseSeekTime);
          m_seeking = false;
          m_playerClient->timeChanged();
#else /* USE_VODSRC */
          if (m_progressTimerHandler != 0)
            g_source_remove(m_progressTimerHandler);
#endif /* USE_VODSRC */
        }
    } else {
        LOG_ERROR("Seek failed");
        updateStates();  // tell the HTML element we're not seeking anymore
    }
}

bool MediaPlayerDLNA::rmf_isPaused() const {
  if (m_isEndReached)
    return true;

  return m_paused;
}

bool MediaPlayerDLNA::rmf_isSeeking() const {
  return m_seeking;
}

void MediaPlayerDLNA::notifyPresenceOfVideo()
{
    /* Since Presence of Video is notified, this cannot be of Music Only channel. */
    m_isMusicOnlyAsset = false;
}

void MediaPlayerDLNA::notifyPlayerOfEISSData()
{
  if (m_onEISSDUpdateHandler)
    g_source_remove(m_onEISSDUpdateHandler);

  m_onEISSDUpdateHandler = g_timeout_add(0, [](gpointer data) -> gboolean {
    MediaPlayerDLNA& self = *static_cast<MediaPlayerDLNA*>(data);
    self.m_onEISSDUpdateHandler = 0;
    self.onEISSDUpdate();
    return G_SOURCE_REMOVE;
  }, this);
}

void MediaPlayerDLNA::notifyPlayerOfFirstVideoFrame() {
  if (m_isVOnlyOnce)
    LOG_INFO("Received First Video Frame for the playback of the URL=%s with "
            "play speed = %f time position = %f",
            m_url.c_str(),
            m_playbackRate,
            m_currentPosition);
  else
    LOG_INFO("Received First Video Frame for the trickmode in the URL=%s with "
            "play speed = %f time position = %f",
            m_url.c_str(),
            m_playbackRate,
            m_currentPosition);

  m_allowPositionQueries = true;
  m_EOSPending = false;
  if (m_onFirstVideoFrameHandler)
    g_source_remove(m_onFirstVideoFrameHandler);
  m_onFirstVideoFrameHandler = g_timeout_add(0, [](gpointer data) -> gboolean {
    MediaPlayerDLNA& self = *static_cast<MediaPlayerDLNA*>(data);
    self.m_onFirstVideoFrameHandler = 0;
    self.onFirstVideoFrame();
    return G_SOURCE_REMOVE;
  }, this);
}

void MediaPlayerDLNA::onFirstVideoFrame() {
  /** Post CC handled only for the first time video frame received */
  if (m_isVOnlyOnce) {
//    m_playerClient->videoDecoderHandleReceived();
    onFrameReceived();
    m_isVOnlyOnce = false;
  }

  if (m_VODKeepPipelinePlaying) {
    if (m_isVODAsset && !m_EOSPending && !m_fetchTrickModeHeaders) {
      fetchHeaders();
    }
  } else {
    if (m_isVODAsset)
      fetchHeaders();
  }

  /* now as it is started playing video, the time bar must be moving. So update it.. */
  updateStates();
}

void MediaPlayerDLNA::onEISSDUpdate()
{
  m_playerClient->eissDataReceived();
}

void MediaPlayerDLNA::handleCurrentAssetType() {
    if (RMF_CURRENTLY_PLAYING_AV_ASSET != m_sink->getCurrentAssetType()) {
        LOG_INFO("Noticed that we are playing low bit rate content now...");
        if (m_hnsource)
            m_hnsource->setLowBitRateMode(true);
    }
}


void MediaPlayerDLNA::notifyPlayerOfFirstAudioFrame() {
  if (m_isAOnlyOnce)
    LOG_INFO("Received First Audio Sample for the playback of URL=%s with play "
            "speed = %f time position = %f",
            m_url.c_str(),
            m_playbackRate,
            m_currentPosition);
  else
    LOG_INFO("Received First Audio Sample for the trickmode in URL=%s with play "
            "speed = %f time position = %f",
            m_url.c_str(),
            m_playbackRate,
            m_currentPosition);

    /* Check the Channel Type */
    if (m_currentAssetTypeHandler)
        g_source_remove(m_currentAssetTypeHandler);

    if (m_isAOnlyOnce && isOCAP())
        m_currentAssetTypeHandler = g_timeout_add(0, [](gpointer data) -> gboolean { MediaPlayerDLNA& self = *static_cast<MediaPlayerDLNA*>(data);
                                                                                      self.m_currentAssetTypeHandler = 0;
                                                                                      self.handleCurrentAssetType();
                                                                                      return G_SOURCE_REMOVE;
                                                                                    }, this);

    /* As the video being rendered is what matters, the possition query must be allowed after Video frame; But only for AV channels. For Music only, just set to true */
    if (m_isMusicOnlyAsset)
        m_allowPositionQueries = true; // Allow pipeline position queries now.

  if (m_onFirstAudioFrameHandler)
    g_source_remove(m_onFirstAudioFrameHandler);
  m_onFirstAudioFrameHandler = g_timeout_add(0, [](gpointer data) -> gboolean {
      MediaPlayerDLNA& self = *static_cast<MediaPlayerDLNA*>(data);
      self.m_onFirstAudioFrameHandler = 0;
      self.onFirstAudioFrame();
      return G_SOURCE_REMOVE;
  }, this);
}

void MediaPlayerDLNA::onFirstAudioFrame() {
  if (m_isAOnlyOnce) {
    onFrameReceived();
    m_isAOnlyOnce = false;
  }
}

void MediaPlayerDLNA::onFrameReceived() {
  // Report First Frame received only once in the beginning
  if (m_isVOnlyOnce && m_isAOnlyOnce)
  {
    /** Post CC handle only for the first frame received */
    m_playerClient->videoDecoderHandleReceived();
    m_playerClient->mediaFrameReceived();
  }
}

void MediaPlayerDLNA::notifyPlayerOfMediaWarning() {
  if (m_onMediaWarningReceivedHandler)
    g_source_remove(m_onMediaWarningReceivedHandler);
  m_onMediaWarningReceivedHandler = g_timeout_add(0, [](gpointer data) -> gboolean {
    MediaPlayerDLNA& self = *static_cast<MediaPlayerDLNA*>(data);
    self.m_onMediaWarningReceivedHandler = 0;
    self.onMediaWarningReceived();
    return G_SOURCE_REMOVE;
  }, this);
}

void MediaPlayerDLNA::onMediaWarningReceived() {
  if (m_sink) {
    m_rmfMediaWarnDesc = m_sink->getMediaWarningString();
    m_rmfMediaWarnData = m_sink->getMediaBufferSize();
    m_playerClient->mediaWarningReceived();
  }
}

void MediaPlayerDLNA::onPlay() {
  LOG_INFO("DLNA player: on play");
  m_isEndReached = false;

  // Adjust the video window to the same size once again..
  // There are platforms which is taking the rectangle param only at playing
  // state..
  m_sink->setVideoRectangle(m_lastKnownRect.x(),
                            m_lastKnownRect.y(),
                            m_lastKnownRect.width(),
                            m_lastKnownRect.height(),
                            true);

  // make sure that the AV is muted on the blocked content
  if (m_muted)
    m_sink->setMuted(true);

  /* Update the time when first frame is received */
  m_currentPosition = rmf_getCurrentTime();

  updateStates();
}

void MediaPlayerDLNA::onPause() {
  LOG_INFO("DLNA player: on pause");
  updateStates();
}

void MediaPlayerDLNA::onStop() {
  LOG_INFO("DLNA player: on stop");
  updateStates();
}

void MediaPlayerDLNA::rmf_setVolume(float volume) {
  if (!m_sink)
    return;

  m_sink->setVolume(volume);
}

float MediaPlayerDLNA::rmf_getVolume() const {
  if (!m_sink)
    return 0.0;

  return m_sink->getVolume();
}

void MediaPlayerDLNA::rmf_setRate(float rate) {
  LOG_INFO(
      "setRate called new speed=%f previous speed=%f pos=%f",
      rate,
      rmf_getRate(),
      playbackPosition());
  rmf_changeSpeed(rate, 0.0f);
}

void MediaPlayerDLNA::rmf_changeSpeed(float rate, short overShootTime) {
  LOG_INFO("changeSpeed called new speed=%f previous speed=%f pos=%f, overShootTime=%d", rate,
           rmf_getRate(), playbackPosition(), overShootTime);
  if (!m_source) {
    LOG_INFO("No source, skip changeSpeed.");
    return;
  }
#ifdef ENABLE_DIRECT_QAM
  /* Trick mode is allowed only for VOD and mDVR */
  if (!(m_isDVRAsset | m_isVODAsset))
    return;
#endif /* ENABLE_DIRECT_QAM */
  if (m_playbackRate == rate)
    return;

  m_pausedInternal = true;

  {
    m_currentPosition = rmf_getCurrentTime();
    m_currentPosition += overShootTime;

    if (m_currentPosition < 0)
      m_currentPosition = 0;

    double position = m_currentPosition;

    if (m_isVODAsset) {
      if (!m_VODKeepPipelinePlaying) {
        m_allowPositionQueries = false; // Will ensure that redundant position requests are avoided
      }
      /* DO NOT Update the m_playbackRate before getting the HEAD Response from RMFStreamer. */
      if (m_VODKeepPipelinePlaying) {
        if (m_EOSPending) {
          LOG_INFO("Rate Change is not accepted as VOD Server posted that it is sending EOS shortly");
          return;
        }
        if (!m_isAOnlyOnce) {
          fetchHeadersForTrickMode(rate, position);
          m_pausedInternal = false;
          if (rate != 1.0)
             m_isVODRateChangeRequested = true;
          else
             m_isVODRateChangeRequested = false;
        }
      } else {
        m_paused = false;
        m_playbackRate = rate;
        m_currentProgressTime = 0;
        m_baseSeekTime = m_currentPosition;
#ifndef USE_VODSRC
        if (m_progressTimerHandler != 0)
          g_source_remove(m_progressTimerHandler);
#endif /* USE_VODSRC */
      }
#ifdef USE_VODSRC
      m_source->setSpeed (rate);
      m_source->setMediaTime((m_baseSeekTime * 1000));

      double npt = 0.00;
      m_source->getMediaTime(npt);
      m_baseSeekTime = (long long) npt/1000;
      m_pausedInternal = false;
#else
      if (m_source->play (rate, position) == RMF_RESULT_SUCCESS)
        m_playerClient->rateChanged();
#endif /* USE_VODSRC */
    } else {
      m_paused = false;
      m_playbackRate = rate;
      m_allowPositionQueries = false; // Will ensure that redundant position requests are avoided
      if (m_source->play (rate, position) == RMF_RESULT_SUCCESS)
        m_playerClient->rateChanged();
      else
        LOG_ERROR("Requested rate change(%f) is failed...", m_playbackRate);
      m_playerClient->timeChanged();
    }
  }
}

MediaPlayer::RMFPlayerState MediaPlayerDLNA::rmf_playerState () const {
  return m_playerState;
}

MediaPlayer::RMFVideoBufferState MediaPlayerDLNA::rmf_videoState() const {
  return m_videoState;
}

float MediaPlayerDLNA::maxTimeLoaded() const {
  if (m_errorOccured)
    return 0.0f;

  return rmf_getDuration();
}

void MediaPlayerDLNA::cancelLoad() {
  if (m_playerState < MediaPlayer::RMF_PLAYER_LOADING ||
      m_playerState == MediaPlayer::RMF_PLAYER_LOADED)
    return;

  rmf_stop();
}

void MediaPlayerDLNA::updateStates() {
  if (!m_source)
    return;

  if (m_errorOccured) {
    LOG_INFO("Error occured");
    return;
  }

  MediaPlayer::RMFPlayerState oldNetworkState = m_playerState;
  MediaPlayer::RMFVideoBufferState oldReadyState = m_videoState;
  RMFState state;
  RMFState pending;

  RMFStateChangeReturn ret = m_source->getState(&state, &pending);
  LOG_VERBOSE("updateStates(): state: %s, pending: %s, ret = %s",
    gst_element_state_get_name((GstState) state),
    gst_element_state_get_name((GstState) pending),
    gst_element_state_change_return_get_name((GstStateChangeReturn) ret));

  bool shouldUpdateAfterSeek = false;
  switch (ret) {
    case RMF_STATE_CHANGE_SUCCESS:
      LOG_VERBOSE("State: %s, pending: %s",
        gst_element_state_get_name((GstState) state),
        gst_element_state_get_name((GstState) pending));

      // Try to figure out ready and network states.
      if (state == RMF_STATE_READY) {
        m_videoState = MediaPlayer::RMF_VIDEO_BUFFER_HAVEMETADATA;
        m_playerState = MediaPlayer::RMF_PLAYER_EMPTY;
        // Cache the duration without emiting the durationchange
        // event because it's taken care of by the media element
        // in this precise case.
        cacheDuration();
#ifdef ENABLE_DIRECT_QAM
        /* The QAMSrc and all its child class will post no-prerolling.
         * But when exit from mDVR, it will find the tuner tuned that freq already and try to reuse it.
         * Thus we will get only this event arrive until we call play(). But play will not be
         * invoked by WebKit until we report we have EnoughData.
         */
        if (!m_isDVRAsset)
          m_videoState = MediaPlayer::RMF_VIDEO_BUFFER_HAVEENOUGHDATA;
#endif /* ENABLE_DIRECT_QAM */
      } else if (maxTimeLoaded() == rmf_getDuration()) {
        m_playerState = MediaPlayer::RMF_PLAYER_LOADED;
        m_videoState = MediaPlayer::RMF_VIDEO_BUFFER_HAVEENOUGHDATA;
      } else {
        m_videoState = rmf_getCurrentTime() < maxTimeLoaded() ? MediaPlayer::RMF_VIDEO_BUFFER_HAVEFUTUREDATA : MediaPlayer::RMF_VIDEO_BUFFER_HAVECURRENTDATA;
        m_playerState = MediaPlayer::RMF_PLAYER_LOADING;
      }
      // Now let's try to get the states in more detail using
      // information from GStreamer, while we sync states where
      // needed.
      if (state == RMF_STATE_PAUSED) {
        if (rmf_getCurrentTime() < rmf_getDuration()) {
          m_pausedInternal = true;
          m_videoState = MediaPlayer::RMF_VIDEO_BUFFER_HAVECURRENTDATA;
          m_playerState = MediaPlayer::RMF_PLAYER_LOADING;
          m_playerClient->timeChanged();
        }
      } else if (state == RMF_STATE_PLAYING) {
        m_videoState = MediaPlayer::RMF_VIDEO_BUFFER_HAVEENOUGHDATA;
        m_playerState = MediaPlayer::RMF_PLAYER_LOADED;
        m_pausedInternal = false;

        m_seeking = false;
        m_playerClient->timeChanged();
      } else {
        m_pausedInternal = true;
      }

      if (rmf_isSeeking()) {
        shouldUpdateAfterSeek = true;
        m_seeking = false;
      }

      break;
    case RMF_STATE_CHANGE_ASYNC:
      LOG_VERBOSE("Async: State: %s, pending: %s",
          gst_element_state_get_name((GstState)state),
          gst_element_state_get_name((GstState)pending));
      // Change in progress

      if (!m_isStreaming)
        return;

      if (rmf_isSeeking()) {
        shouldUpdateAfterSeek = true;
        m_seeking = false;
      }
      break;
    case RMF_STATE_CHANGE_FAILURE:
      LOG_VERBOSE("Failure: State: %s, pending: %s",
                  gst_element_state_get_name((GstState)state),
                  gst_element_state_get_name((GstState)pending));
      // Change failed
      return;
    case RMF_STATE_CHANGE_NO_PREROLL:
      LOG_VERBOSE("No preroll: State: %s, pending: %s",
                  gst_element_state_get_name((GstState)state),
                  gst_element_state_get_name((GstState)pending));

      if (state == RMF_STATE_READY) {
        m_videoState = MediaPlayer::RMF_VIDEO_BUFFER_HAVENOTHING;
      } else if (state == RMF_STATE_PAUSED) {
        m_videoState = MediaPlayer::RMF_VIDEO_BUFFER_HAVEENOUGHDATA;
        m_pausedInternal = true;
        // Live pipelines go in PAUSED without prerolling.
        m_isStreaming = true;
      } else if (state == RMF_STATE_PLAYING) {
        m_pausedInternal = false;
      }

      if (rmf_isSeeking()) {
        shouldUpdateAfterSeek = true;
        m_seeking = false;
        if (!m_pausedInternal)
          m_source->play();
      } else if (!m_pausedInternal) {
        m_source->play();
      }

      m_playerState = MediaPlayer::RMF_PLAYER_LOADING;
      break;
    default:
      LOG_VERBOSE("Else : %d", ret);
      break;
  }

  if (rmf_isSeeking())
    m_videoState = MediaPlayer::RMF_VIDEO_BUFFER_HAVENOTHING;

  if (shouldUpdateAfterSeek)
    m_playerClient->timeChanged();

  if (m_playerState != oldNetworkState) {
    LOG_INFO("Network State Changed from %u to %u",
             oldNetworkState, m_playerState);
    m_playerClient->playerStateChanged();
  }
  if (m_videoState != oldReadyState) {
    LOG_INFO("Ready State Changed from %u to %u",
             oldReadyState, m_videoState);
    m_playerClient->videoStateChanged();
  }
}

void MediaPlayerDLNA::timeChanged() {
  updateStates();
  m_playerClient->timeChanged();
}

void MediaPlayerDLNA::ended() {
  if (m_progressTimerHandler != 0)
    g_source_remove(m_progressTimerHandler);

  m_isEndReached = true;

  if (m_playbackRate < 0)
  {
    LOG_INFO("MediaPlayerDLNA: We have hit starting point when rewinding; Start from beginning\n");
    m_playbackRate = 1.0f;
    m_playerClient->rateChanged();
    rmf_seekToStartPosition();
    return;
  }
  else if (m_isInProgressRecording && (m_playbackRate >= 1))
  {
    LOG_INFO("MediaPlayerDLNA: We have got EOS for the asset and lets switch to Live/LivePoint\n");
    m_playbackRate = 1.0f;
    m_playerClient->rateChanged();
    rmf_seekToLivePosition();
    return;
  }

  // EOS was reached but in case of reverse playback the position is
  // not always 0. So to not confuse the HTMLMediaElement we
  // synchronize position and duration values.
  float now = rmf_getCurrentTime();
  if (now > 0) {
    m_mediaDuration = now;
    m_mediaDurationKnown = true;
    m_playerClient->durationChanged();
  }
  m_playerClient->timeChanged();
  m_playerClient->mediaPlaybackCompleted();
}

void MediaPlayerDLNA::cacheDuration() {
  if (0 == m_mediaDuration)
    m_mediaDuration = rmf_getCurrentTime();

  m_mediaDurationKnown = !std::isinf(m_mediaDuration);
}

void MediaPlayerDLNA::durationChanged() {
  float previousDuration = m_mediaDuration;

  cacheDuration();
  // Avoid emiting durationchanged in the case where the previous
  // duration was 0 because that case is already handled by the
  // HTMLMediaElement.
  if (previousDuration && m_mediaDuration != previousDuration)
    m_playerClient->durationChanged();
}

void MediaPlayerDLNA::rmf_setMute(bool muted) {
  m_muted = muted;
  if (m_sink)
    m_sink->setMuted(muted);
}

void MediaPlayerDLNA::loadingFailed(MediaPlayer::RMFPlayerState error) {
  LOG_ERROR("error=%s", StateString(error));
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

void MediaPlayerDLNA::rmf_setVideoRectangle(
  unsigned x, unsigned y, unsigned w, unsigned h) {
  IntRect temp(x, y, w, h);
  if (m_lastKnownRect != temp) {
    m_lastKnownRect = temp;
    if (m_sink) {
      LOG_VERBOSE("setVideoRectangle: %d,%d %dx%d", x, y, w, h);
      m_sink->setVideoRectangle(x, y, w, h, true);
    }
  }
}

// static
bool MediaPlayerDLNA::supportsUrl(const std::string& urlStr)
{
    return
        has_substring(urlStr, "/hnStreamStart") ||
        has_substring(urlStr, "/vldms/") ||
        has_substring(urlStr, "ocap://") ||
        has_substring(urlStr, "vod://") ||
        has_substring(urlStr, "ippv://") ||
        has_substring(urlStr, "profile=MPEG_TS") ||
        has_substring(urlStr, ".ts") ||
        has_substring(urlStr, ".m2ts");
}

