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

/**
 * @file hnsource.cpp
 * @brief HN Source connects the media devices in the home network using the http protocol layer with a Virtual address/URI.
 */

#include "hnsource.h"
#include "rmfprivate.h"

#include <cassert>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <glib.h>
#include <gst/gst.h>
#include <gst/gstutils.h>
#include <unistd.h>
#include <limits>
#include <string>
#include <sys/time.h>

#include "config.h"

using namespace std;
#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

#define HNSRCLOG_ERROR(format...)       RDK_LOG(RDK_LOG_ERROR,  "LOG.RDK.HNSRC", format)
#define HNSRCLOG_WARN(format...)        RDK_LOG(RDK_LOG_WARN,   "LOG.RDK.HNSRC", format)
#define HNSRCLOG_INFO(format...)        RDK_LOG(RDK_LOG_INFO,   "LOG.RDK.HNSRC", format)
#define HNSRCLOG_DEBUG(format...)       RDK_LOG(RDK_LOG_DEBUG,  "LOG.RDK.HNSRC", format)
#define HNSRCLOG_TRACE(format...)       RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.HNSRC", format)

#define HN_BIN getSource()
#define HN_SOURCE_IMPL static_cast<HNSourcePrivate*>(getPrivateSourceImpl())
#define DTCP_SERVER_PORT 5000
#define GST_ELEMENT_GET_STATE_RETRY_CNT_MAX 5

#define VOD_PLAYBACK_EOS_TRAILER_ERROR_CODE         3
#define VOD_LIVE_TUNE_FAIL_TOUT_TRAILER_ERROR_CODE  24
#define VOD_PLAYBACK_SRM_26_TRAILER_ERROR_CODE      26
#define VOD_PLAYBACK_COMPLETED_TRAILER_ERROR_CODE   66

#define RMF_LIVE_TSB_GUARDBAND  3


#ifdef USE_GST1
static GstPadProbeReturn onPadBlocked(GstPad* pad, GstPadProbeInfo* info, gpointer userdata);
static GstPadProbeReturn onPadBlockedTricMode(GstPad* pad, GstPadProbeInfo* info, gpointer userdata);
#endif

static void time_to_hms(float time, unsigned& h, unsigned& m, float& s)
{
    if (time < 0)
        time = 0;

    h = time / 3600;
    m = (time - h * 3600) / 60;
    s = time - h * 3600 - m * 60;
}

//-- HNSourcePrivate -----------------------------------------------------------

/**
 * @class HNSourcePrivate
 * @brief Class extending RMFMediaSourcePrivate to implement HNSourcePrivate variables and functions.
 * @ingroup hnsrcclass
 */
class HNSourcePrivate : public RMFMediaSourcePrivate
{
public:
    HNSourcePrivate(HNSource* parent);
    void* createElement();
    RMFResult getBufferedRanges(HNSource::range_list_t& ranges);

    // from IRMFMediaSource
    /*virtual*/ RMFResult open(const char* url, char* mimetype);
    /*virtual*/ RMFResult term();
    /*virtual*/ RMFResult play();
    /*virtual*/ RMFResult play(float& speed, double& time);
    /*virtual*/ RMFResult getSpeed(float& speed);
    /*virtual*/ RMFResult setSpeed(float speed);
    /*virtual*/ RMFResult getMediaTime(double& t);
    /*virtual*/ RMFResult setMediaTime(double time);
    /*virtual*/ RMFResult playAtLivePosition(double duration);
                RMFResult setVideoLength (double length);
                RMFResult setLowBitRateMode (bool isYes);

#ifdef USE_GST1
    void onPadBlocked();
    void onPadBlockedTricMode();
#endif

private:
    void onEOS();
    void onPlaying();
    void onError(RMFResult err, const char* msg);
    RMFResult populateBin(GstElement* bin);
    static void setGhostPadTarget(GstElement* bin, GstPad* target, bool unref_target);
    static RMFResult setSourceHeaders(GstElement* source, double speed, double pos);
    RMFResult handleElementMsg (GstMessage *msg);
    GstState validateStateWithMsTimeout (GstState stateToValidate, guint msTimeOut);
    RMFResult getMediaInfo(RMFMediaInfo& info);
#ifdef USE_GST1
    RMFResult handleTricMode(GstState transitionToState, GstPadProbeType tricModeProbeType = GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM);
#else
    RMFResult handleTricMode(GstState transitionToState);
#endif

    RMFResult pausePlayback();

    RMFResult getRMFTrailerError(char *errorMsg, int maxLen);
    bool isTrailerErrorAvailable(void);
    bool getValidTrailerReceived(void);
    void retrieveAndStoreTrailer(void);
    void setValidTrailerReceived (bool validTrailerReceived);
    int getErrorCode(gchar *trailerData);
    double getSystemTimeNow();

    std::string m_url;
    float m_speed;
    GstElement* m_source;
    GstElement* m_dtcp;
    float m_seekTime;
    std::string m_dtcpServerIp;
    bool m_isPaused;
    bool m_isVODAsset;
    bool m_isLIVEAsset;
    bool m_isPPVAssetPur;
    double m_videoLength;
    bool m_isErrorReported;
    bool m_gotMediaLengthRespose;
    bool m_isPausedAtLive;
    bool m_isPausedAtLiveAfterTSBLimit;
    double m_tsbMaxLength;
    double m_timeWhenPausedAtLive;

    bool    m_validTrailerReceived;
    std::string m_trailerData;

    gint64  m_currPosBkup;
    bool    m_VODKeepPipelinePlaying;
    bool    m_VODAssetStartup;
    bool    m_configureLowBitRate;

#ifdef USE_GST1
    /* used for pad blocking via probes */
    GMutex  padBlockTricModeMutex;
    GMutex  padBlockMutex;
    GMutex  gstStateMutex;
    GCond   padBlockCond;
    bool    padBlockDone;
#endif
};

HNSourcePrivate::HNSourcePrivate(HNSource* parent)
:   RMFMediaSourcePrivate(parent)
,   m_speed(1.0)
,   m_source(NULL)
,   m_dtcp(NULL)
,   m_seekTime(0)
,   m_isPaused(false)
,   m_isVODAsset (false)
,   m_isLIVEAsset (false)
,   m_isPPVAssetPur(false)
,   m_videoLength (0.0)
,   m_isErrorReported(false)
,   m_gotMediaLengthRespose(false)
,   m_isPausedAtLive (false)
,   m_isPausedAtLiveAfterTSBLimit(false)
,   m_tsbMaxLength(0.0)
,   m_timeWhenPausedAtLive(0.0)
,   m_trailerData("")
,   m_currPosBkup(0)
,   m_VODKeepPipelinePlaying(true)
{
    char *pVODKeepPipelinePlaying = NULL;

    pVODKeepPipelinePlaying = getenv("RMF_VOD_KEEP_PIPELINE");
    if ((pVODKeepPipelinePlaying != NULL) && (strcasecmp(pVODKeepPipelinePlaying, "FALSE") == 0)) {
        m_VODKeepPipelinePlaying = false;
    }

    /* Init trailer processing variables */
    m_validTrailerReceived = false;

    if (m_VODKeepPipelinePlaying) {
        m_VODAssetStartup = false;
    }

    m_configureLowBitRate = false;
#ifdef USE_GST1
    g_mutex_init (&padBlockTricModeMutex);
    g_mutex_init (&padBlockMutex);
    g_mutex_init (&gstStateMutex);
    g_cond_init (&padBlockCond);
#endif
}

/**
 * @fn void* HNSourcePrivate::createElement()
 * @brief This function is used to create gstreamer element.
 * It creates an empty bin as "hn_source_bin". Adds a ghost pad (link point) to element.
 * Pads are not automatically activated so elements should perform the needed steps to activate the pad
 * in case this pad is added in the PAUSED or PLAYING state.
 * The pad and the element should be unlocked when calling this function.
 * Create a new ghostpad without a target with the given direction.
 *
 * @param None
 * @retval bin Pointer of hn_source_bin.
 */
void* HNSourcePrivate::createElement()
{
    // Create an empty bin. It will be populated in open().
    GstElement* bin = gst_bin_new("hn_source_bin");
    RMF_ASSERT(bin);

    // Create a ghost source pad with no target.
    // It will be connected to a child element in open().
    gst_element_add_pad(bin, gst_ghost_pad_new_no_target("src", GST_PAD_SRC));

    return bin;
}

RMFResult HNSourcePrivate::getMediaInfo(RMFMediaInfo& info)
{
    if (!getPipeline())
        return RMF_RESULT_FAILURE;

    info.m_startTime = 0.0;
    info.m_duration = m_videoLength;

    return RMF_RESULT_SUCCESS;
}


#ifdef USE_GST1
RMFResult HNSourcePrivate::handleTricMode (GstState transitionToState, GstPadProbeType tricModeProbeType)
#else
RMFResult HNSourcePrivate::handleTricMode (GstState transitionToState)
#endif
{
    GstState gst_current;
    GstState gst_pending;

#ifdef USE_GST1
    float timeout = 10.0;
    if (transitionToState == GST_STATE_NULL) {

        g_mutex_lock (&gstStateMutex);
        gst_element_set_state(getPipeline(), transitionToState);
        g_mutex_unlock (&gstStateMutex);

        if (GST_STATE_CHANGE_FAILURE  == gst_element_get_state(getPipeline(), &gst_current, &gst_pending, timeout * GST_MSECOND))
            HNSRCLOG_ERROR("HNSource::handleTricMode - gst_element_get_state - FAILURE : State = %d, Pending = %d\n", gst_current, gst_pending);
    }
    else {
        GstPad* padToBlock = gst_element_get_static_pad(m_source, "src");

        if (padToBlock) {
            if ((tricModeProbeType != GST_PAD_PROBE_TYPE_BLOCKING) && GST_PAD_IS_EOS(padToBlock)) {
                // Want to avoid this. But it looks like downstream elements in the pipeline expect
                // the transition to Paused. In the near future modify downstream elements to handle tricmode
                // when the pipeline is in Playing state
                g_mutex_lock (&gstStateMutex);
                gst_element_set_state(getPipeline(), transitionToState);
                g_mutex_unlock (&gstStateMutex);

                if (GST_STATE_CHANGE_FAILURE  == gst_element_get_state(getPipeline(), &gst_current, &gst_pending, timeout * GST_MSECOND))
                    HNSRCLOG_WARN("HNSource::Play - gst_element_get_state - FAILURE : State = %d, Pending = %d\n", gst_current, gst_pending);

                flushPipeline();

                HNSRCLOG_WARN("HNSource::handleTricMode - HNBIN to NULL\n");

                g_mutex_lock (&gstStateMutex);
                gst_element_set_state(HN_BIN, GST_STATE_NULL);
                g_mutex_unlock (&gstStateMutex);

                if (GST_STATE_CHANGE_FAILURE  == gst_element_get_state(HN_BIN, &gst_current, &gst_pending, timeout * GST_MSECOND))
                    HNSRCLOG_WARN("HNSource::handleTricMode - gst_element_get_state HN_BIN - FAILURE : State = %d, Pending = %d\n", gst_current, gst_pending);
            }
            else {
                gulong padProbeId;

                g_mutex_lock (&padBlockTricModeMutex);

                if (tricModeProbeType == GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM) {
                    gint64 padBlockCondTimeout;

                    g_mutex_lock (&padBlockMutex);
                    padBlockDone = false;
                    padProbeId = gst_pad_add_probe (padToBlock, tricModeProbeType, &::onPadBlockedTricMode, this, NULL);

                    do {
                        padBlockCondTimeout = g_get_monotonic_time () + 500 * G_TIME_SPAN_MILLISECOND;
                        if (!g_cond_wait_until(&padBlockCond, &padBlockMutex, padBlockCondTimeout)) {
                            if(gst_pad_is_blocked(padToBlock) || gst_pad_is_blocking(padToBlock)) {
                                // Safe to assume that is blocked or will be blocked if data is pushed
                                // and the installed probe will kick in preventing data flow
                                padBlockDone = TRUE;
                                HNSRCLOG_WARN("HNSource::handleTricMode - padBlockDone Timeout GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM\n");
                            }
                        }
                    } while (!padBlockDone);

                    g_mutex_unlock (&padBlockMutex);
                }
                else if (tricModeProbeType == GST_PAD_PROBE_TYPE_BLOCKING) {
                    gint64 padBlockCondTimeout;

                    padBlockDone = false;
                    padProbeId = gst_pad_add_probe (padToBlock, tricModeProbeType, &::onPadBlockedTricMode, this, NULL);

                    g_mutex_lock (&padBlockMutex);

                    do {
                        padBlockCondTimeout = g_get_monotonic_time () + 5 * G_TIME_SPAN_MILLISECOND;
                        if (!g_cond_wait_until(&padBlockCond, &padBlockMutex, padBlockCondTimeout)) {
                            if (m_isPaused) {
                                if(gst_pad_is_blocked(padToBlock) || gst_pad_is_blocking(padToBlock)) {
                                    // Safe to assume that is blocked or will be blocked if data is pushed
                                    // and the installed probe will kick in preventing data flow
                                    padBlockDone = TRUE;
                                    HNSRCLOG_WARN("HNSource::handleTricMode - padBlockDone Timeout GST_PAD_PROBE_TYPE_BLOCKING\n");
                                }
                            }
                        }
                    } while (!padBlockDone);

                    g_mutex_unlock (&padBlockMutex);
                }

                // Want to avoid this. But it looks like downstream elements in the pipeline expect
                // the transition to Paused. In the near future modify downstream elements to handle tricmode
                // when the pipeline is in Playing state
                g_mutex_lock (&gstStateMutex);
                gst_element_set_state(getPipeline(), transitionToState);
                g_mutex_unlock (&gstStateMutex);

                if (GST_STATE_CHANGE_FAILURE  == gst_element_get_state(getPipeline(), &gst_current, &gst_pending, timeout * GST_MSECOND))
                    HNSRCLOG_WARN("HNSource::Play - gst_element_get_state - FAILURE : State = %d, Pending = %d\n", gst_current, gst_pending);

                flushPipeline();

                HNSRCLOG_WARN("HNSource::handleTricMode - HNBIN to NULL\n");

                g_mutex_lock (&gstStateMutex);
                gst_element_set_state(HN_BIN, GST_STATE_NULL);
                g_mutex_unlock (&gstStateMutex);

                if (GST_STATE_CHANGE_FAILURE  == gst_element_get_state(HN_BIN, &gst_current, &gst_pending, timeout * GST_MSECOND))
                    HNSRCLOG_WARN("HNSource::handleTricMode - gst_element_get_state HN_BIN - FAILURE : State = %d, Pending = %d\n", gst_current, gst_pending);

                gst_pad_remove_probe (padToBlock, padProbeId);
                g_mutex_unlock (&padBlockTricModeMutex);
                HNSRCLOG_WARN("HNSource::handleTricMode - onPadBlockedTricMode - Pad UNBLOCKED\n");
            }

            gst_object_unref (padToBlock);
        }
    }
#else
    float timeout = 100.0;
    gst_element_set_state(getPipeline(), transitionToState);

    if (GST_STATE_CHANGE_FAILURE  == gst_element_get_state(getPipeline(), &gst_current, &gst_pending, timeout * GST_MSECOND))
        HNSRCLOG_WARN("HNSource::handleTricMode - gst_element_get_state - FAILURE : State = %d, Pending = %d\n", gst_current, gst_pending);

    if (gst_current != GST_STATE_NULL) {
        HNSRCLOG_WARN("HNSource::handleTricMode - Calling : flushPipeline - PIPELINE STATE %d\n", gst_current);
        flushPipeline();
        gst_element_set_state(HN_BIN, GST_STATE_NULL);
    }
#endif

    return RMF_RESULT_SUCCESS;
}

/**
 * @fn RMFResult HNSourcePrivate::getBufferedRanges(HNSource::range_list_t& ranges)
 * @brief This function is used to get the bufferedrange of the TSB.
 * When the function is called, it will return the total video duration for the current TSB buffer.
 * This function returns a range_list which is a list and it stores the start and stop values of the buffered ranges array.
 * Gstreamer constructs a new query object for querying the buffering status of stream with percent format.
 * Retrieves the number of values currently stored in the buffered_ranges array of the query structure.
 * Parse an available query and get the start and stop values stored at the index of the buffered ranges array.
 * Ranges pushes back the element that inserts the given element at the end of the queue.
 *
 * @param[out] ranges Holds the start and stop values of media duration.
 *
 * @return RMFResult
 * @retval RMF_RESULT_SUCCESS If successfully gets the bufferrange value.
 * @retval RMF_RESULT_FAILURE If failed to get the status of the buffering stream and Gstreamer version is not supported.
 */
RMFResult HNSourcePrivate::getBufferedRanges(HNSource::range_list_t& ranges)
{
#if GST_CHECK_VERSION(0, 10, 31)
    RMFMediaInfo info;
    memset(&info, 0, sizeof(info));
    RMF_ASSERT(getMediaInfo(info) == RMF_RESULT_SUCCESS);
    const float& media_duration  = info.m_duration;

    if (!media_duration || isinf(media_duration))
        return RMF_RESULT_FAILURE;

    GstQuery* query = gst_query_new_buffering(GST_FORMAT_PERCENT);

    if (!gst_element_query(getPipeline(), query)) {
        gst_query_unref(query);
        return RMF_RESULT_FAILURE;
    }

    gint64 rangeStart = 0, rangeStop = 0;
    for (guint index = 0; index < gst_query_get_n_buffering_ranges(query); index++) {
        if (gst_query_parse_nth_buffering_range(query, index, &rangeStart, &rangeStop))
            ranges.push_back(
                std::pair<float, float>((rangeStart * media_duration) / 100,
                                        (rangeStop * media_duration) / 100));
    }
    gst_query_unref(query);
    return RMF_RESULT_SUCCESS;
#else
    return RMF_RESULT_FAILURE; // not supported by GStreamer
#endif
}

/**
 * @fn RMFResult HNSourcePrivate::open(const char* url, char* mimetype)
 * @brief This function is used to open HNSource instance for the passed uri which is in the format http://.
 * Function populates the gstreamer bin.
 * Gstreamer creates the "souphttpsrc or httpsrc" and "dtcpdec" gstreamer plugins and added these plugins to bin element called "hn_source_bin".
 * Gstreamer sets the "location" property of httpsrc plugin from the url.
 * If url format is ocap://, it sets into live playback.
 * If url format is vod://, vod playback has set in pipeline.
 * Else,live playback is set as false.
 *
 * @param[in] URI URI in the format http://
 * @param[in] mimetype Not used.
 *
 * @return RMFResult (int)
 * @retval RMF_RESULT_SUCCESS If successfully opened the url.
 * @retval RMF_RESULT_INVALID_ARGUMENT If invalid url is used.
 * @retval RMF_RESULT_FAILURE If failed to open the url.
 * @retval RMF_RESULT_INTERNAL_ERROR If failed to instantiate httpsrc element, failed to create dtcp filter and failed to link source and filter.
 */
RMFResult HNSourcePrivate::open(const char* url, char* mimetype)
{
    HNSRCLOG_WARN("HNSourcePrivate::open(%s, %s)\n", url, mimetype);
    (void) mimetype;

    if (!url || !*url)
        return RMF_RESULT_INVALID_ARGUMENT;

    std::size_t isFound;
    string testUrl =  url;
    isFound = testUrl.find("http://");
    if( isFound == string::npos)
        return RMF_RESULT_INVALID_ARGUMENT;

    m_url = url;
    {
        isFound = testUrl.find("ocap://");
        if( isFound != string::npos)
        {
            m_isLIVEAsset = true;
            updateSourcetoLive (true);

            if ((string::npos != testUrl.find("&token=")) && (string::npos != testUrl.find("&ppv=true"))) {
                m_isPPVAssetPur = true;
            }
        }
        else if (string::npos != testUrl.find("vod://"))
        {
            updateSourcetoLive (true);
            m_isVODAsset = true;

            if (m_VODKeepPipelinePlaying) {
                m_VODAssetStartup = true;
            }
        }
        else
            updateSourcetoLive (false);
    }

    /* Find TSB max length */
    if (m_isLIVEAsset)
    {
        static int tsbLen = std::string("&tsb=").length();
        std::size_t tsbPos = testUrl.find ("&tsb=");
        if( tsbPos != string::npos)
        {
            std::string str = testUrl.substr (tsbPos+tsbLen);
            m_tsbMaxLength = 60 * strtod (str.c_str(), NULL);
        }
    }

    HNSRCLOG_WARN("HNSourcePrivate::open(): Calling populateBin()\n");
    return populateBin(HN_BIN);
}

/**
 * @fn RMFResult HNSourcePrivate::term()
 * @brief This funtion is used to terminate the HNSource instance, removes all the sink from the pipeline and terminated the playback.
 * Function clears the url which gets from source and set the speed and video length to 0.
 * Gstreamer source and dtcp is set as NULL. VODAsset and LIVEAsset making it as False.
 * Function sets gstreamer state to NULL and unreferenced the gstBus also.
 *
 * @param None
 *
 * @return RMFResult
 * @retval RMF_RESULT_SUCCESS If succesfully terminated the HNSource instance.
 * @retval RMF_RESULT_FAILURE If failed to terminate the HNSource instance.
 * @retval RMF_RESULT_NOT_INITIALIZED If gstreamer pipeline is not initialized.
 */
RMFResult HNSourcePrivate::term()
{
    RMFResult res = RMFMediaSourcePrivate::term();

#ifdef USE_GST1
    g_cond_clear(&padBlockCond);
    g_mutex_clear(&padBlockMutex);
    g_mutex_clear(&padBlockTricModeMutex);
#endif

    m_url.clear();
    m_speed = 0;
    m_source = NULL;
    m_dtcp = NULL;

    m_trailerData = "";

    m_isVODAsset = false;
    m_isLIVEAsset = false;
    m_isPPVAssetPur = false;
    m_videoLength = 0.0;
    m_isPausedAtLive = false;
    m_isPausedAtLiveAfterTSBLimit = false;
    m_timeWhenPausedAtLive = 0.0;
    m_tsbMaxLength = 0.0;
    m_configureLowBitRate = false;
    return res;
}

RMFResult HNSourcePrivate::pausePlayback()
{
    HNSRCLOG_WARN("HNSource: Pause Called....\n");
    RMFResult res = RMF_RESULT_SUCCESS;

    if (m_isErrorReported)
        return res;

    if (m_isPaused)
        return res;

    if ((m_isVODAsset) || (m_isLIVEAsset && isLiveSource()))
    {
        double cur_pos = m_seekTime;
        getMediaTime(cur_pos);

        m_isPausedAtLive = true;

        /* Calculate the position */
        if (m_isLIVEAsset && ((cur_pos + RMF_LIVE_TSB_GUARDBAND) >= m_tsbMaxLength))
            m_isPausedAtLiveAfterTSBLimit = true;

        if (m_isLIVEAsset)
        {
            if (cur_pos >= m_tsbMaxLength)
                cur_pos = m_tsbMaxLength - RMF_LIVE_TSB_GUARDBAND;
            else if (cur_pos <= m_videoLength)
                cur_pos = m_videoLength - RMF_LIVE_TSB_GUARDBAND;
            else
                cur_pos = cur_pos - RMF_LIVE_TSB_GUARDBAND;
        }

        m_seekTime = cur_pos;

        /* Get Current System Time to know how long this asset was paused at live after 1 hour of watching. */
        m_timeWhenPausedAtLive = getSystemTimeNow();
        m_currPosBkup = 0;

        /* When Paused, the position does not mater at Streamer side. It just sets the pipeline to READY state. so the cur_pos can be 0;
           But in 2.0.2, pipeline is differntly maintained by Streamer; so lets not mess up) */
        //cur_pos = 0.0;

        if (m_VODKeepPipelinePlaying) {
            if (!m_isVODAsset) {
                handleTricMode(GST_STATE_PAUSED); /* Pause the Pipeline - Handle the Trick Mode */
            }
        }
        else {
            handleTricMode(GST_STATE_PAUSED); /* Pause the Pipeline - Handle the Trick Mode */
        }

        if (m_isVODAsset)
            updateSourcetoLive (true);
        else
            updateSourcetoLive (false);

        g_object_set (G_OBJECT (m_source), "timeout", 0, NULL);

        /* It is configured to be handle Low Bit Rate */
        if(m_configureLowBitRate && isLiveSource())
            g_object_set (G_OBJECT (m_source), "low-bitrate-content", TRUE, NULL);

        if (m_VODKeepPipelinePlaying) {
            if (!m_isVODAsset) {
                res = setSourceHeaders(m_source, 1.0, cur_pos);

                if(m_dtcp)
                {
                    HNSRCLOG_INFO("%s:: Setting DTCP source IP: '%s' and port = '%d' properties to dtcpdec element\n", __FUNCTION__, m_dtcpServerIp.c_str(), DTCP_SERVER_PORT);
                    g_object_set(m_dtcp, "dtcp_src_ip", m_dtcpServerIp.c_str(), NULL);
                    g_object_set(m_dtcp, "dtcp_port", DTCP_SERVER_PORT, NULL);
                }

                notifySpeedChange(1.0);
            }
        }
        else {
            if (m_isVODAsset)
                res = setSourceHeaders(m_source, 0.0, cur_pos);
            else
                res = setSourceHeaders(m_source, 1.0, cur_pos);

            if(m_dtcp)
            {
                HNSRCLOG_INFO("%s:: Setting DTCP source IP: '%s' and port = '%d' properties to dtcpdec element\n", __FUNCTION__, m_dtcpServerIp.c_str(), DTCP_SERVER_PORT);
                g_object_set(m_dtcp, "dtcp_src_ip", m_dtcpServerIp.c_str(), NULL);
                g_object_set(m_dtcp, "dtcp_port", DTCP_SERVER_PORT, NULL);
            }

            notifySpeedChange(1.0);
        }

        if (m_VODKeepPipelinePlaying) {
            if (!m_isVODAsset) {
                gst_element_sync_state_with_parent(HN_BIN);

                if(GST_STATE_PAUSED != validateStateWithMsTimeout(GST_STATE_PAUSED, 10)) { // 10 ms
                    HNSRCLOG_ERROR("HNSource::handleElementMsg - validateStateWithMsTimeout - FAILED GstState %d\n", GST_STATE_PAUSED);
                }
            }
        }
        else {
            gst_element_sync_state_with_parent(HN_BIN);

            if(GST_STATE_PAUSED != validateStateWithMsTimeout(GST_STATE_PAUSED, 10)) { // 10 ms
                HNSRCLOG_ERROR("HNSource::handleElementMsg - validateStateWithMsTimeout - FAILED GstState %d\n", GST_STATE_PAUSED);
            }
        }
    }
    else {
        g_object_set (G_OBJECT (m_source), "timeout", 0, NULL);
        res = RMFMediaSourcePrivate::pause();
    }

    m_isPaused = true;
    return res;
}

/**
 * @fn RMFResult HNSourcePrivate::play()
 * @brief This function is used to change the state of bin to play.
 * Function  does the seek operations and puts video into playing state with the current position and speed.
 * Gstreamer sets the properties for an object with timeout of 10 seconds.
 *
 * @param None
 *
 * @return RMFResult
 * @retval RMF_RESULT_SUCCESS If successfully put video into playing mode.
 * @retval RMF_RESULT_INTERNAL_ERROR If RMF state is failed.
 * @retval RMF_RESULT_FAILURE If failed to put video into playing state.
 * @retval RMF_RESULT_NOT_INITIALIZED If gstreamer pipeline is not initialized.
 */
RMFResult HNSourcePrivate::play()
{
    HNSRCLOG_WARN("HNSource: Play Called....\n");
    if (m_isErrorReported)
        return RMF_RESULT_SUCCESS;

    if ((m_isPaused) && (m_isVODAsset | m_isLIVEAsset) && (m_isPausedAtLive))
    {
        RMFResult lrmfResultRet =  RMF_RESULT_FAILURE;
        double cur_pos = m_seekTime;
        double howLongWasPaused = 0;
        /* Calculate how long it was paused */
        howLongWasPaused = getSystemTimeNow() - m_timeWhenPausedAtLive;

        if (m_isVODAsset)
            cur_pos = m_seekTime;
        /* if TSB was Paused after 'tsbMaxLength' and played, we should use 'howLongWasPaused' as is */
        else if (m_isPausedAtLiveAfterTSBLimit)
            cur_pos = m_seekTime - howLongWasPaused;
        /* if TSB was paused within 'tsbMaxLength' but played after 'tsbMaxLength', we should use the 'howLongWasPaused' to deduct from maxlength */
        else if (m_videoLength  >= (m_tsbMaxLength - RMF_LIVE_TSB_GUARDBAND))
            cur_pos = m_tsbMaxLength - howLongWasPaused;
        /* if TSB was paused & Played within 'tsbMaxLength', we can reuse that 3s guard band */
        else if (howLongWasPaused >= RMF_LIVE_TSB_GUARDBAND)
            cur_pos = m_seekTime + RMF_LIVE_TSB_GUARDBAND;

        /* Reset m_timeWhenPausedAtLive as it is used now */
        m_timeWhenPausedAtLive = 0.0;
        m_isPausedAtLiveAfterTSBLimit = false;

        lrmfResultRet = play (m_speed, cur_pos);

        m_isPausedAtLive = false;
        return lrmfResultRet;
    }
    else
    {
        m_isPaused = false;

        if (m_VODKeepPipelinePlaying) {
            if (m_isVODAsset && m_VODAssetStartup) {
                m_VODAssetStartup = false;
            }
        }

        /* Enabled the timeout. But kept higher value  */
        /* Setting 30s timeout for VOD and 10s for all other videos */
        if (m_isVODAsset || m_isPPVAssetPur)
            g_object_set (G_OBJECT (m_source), "timeout", 30, NULL);
        else
            g_object_set (G_OBJECT (m_source), "timeout", 10, NULL);


        return RMFMediaSourcePrivate::play();
    }
}

/**
 * @fn RMFResult HNSourcePrivate::play(float& speed, double& time)
 * @brief This function is used to change the state of bin to play.
 * Function does the trick modes also with current speed and time.
 * If speed is 0, function puts video into paused state and resets the trailer processing variables.
 * If fast forward[i.e., speed is greater than 1.0] requested while video is already in live mode, it will reset the trailer processing variables.
 * Function updating the time always with guardband using video length value.
 * Suppose if VOD is keep on playing  in pipeline, the following operations will be performed:-
 * <ul>
 * <li> If current RMF state is NULL or READY, it sets the gstreamer state into NULL.
 * <li> handletricMode gets the current gstreamer state and if current gstreamer state is NULL, it flushes the pipeline.
 * <li> By default, a gst pipeline will automatically flush the pending Gst Bus messages when going to the NULL state to ensure
 * that no circular references exit when no messages are read from the Gst Bus.
 * <li> Else, It sets the gstreamer state into PAUSED state and handletricModes function will be performed.
 * </ul>
 * Suppose if VOD is not playing in pipeline, following operation will be performed:-
 * <ul>
 * <li> If current RMF state is in Paused state, it sets the gstreamer state into NULL.
 * <li> Else, it sets the gstreamer state into PAUSED state and handletricModes function will be performed.
 * </ul>
 * If VODAsset flag is set as False, it formats the speed and position according to the DLNA standard.
 * Gstreamer sets the appropriate request headers with the play speed and time seek range for DLNA standard.
 * Function sets the gstreamer properties like dtcp source ip and dtcp port to dtcpdec element.
 * It sets the gstreamer properties on an object with timeout of 30 seconds if it VODAsset is true. Else timeout has set to 10 seconds.
 * Gstreamer tries to change the state of the gstreamer element to the same as its parent.
 *
 * @param[in] speed Speed level to performm trick modes.
 * @param[in] time current position time to perform trick modes.
 *
 * @return RMFResult
 * @retval RMF_RESULT_SUCCESS If successfully put video in playing state.
 * @retval RMF_RESULT_INTERNAL_ERROR If RMF state is failed.
 * @retval RMF_RESULT_FAILURE If failed to put video into playing state.
 * @retval RMF_RESULT_NOT_INITIALIZED If gstreamer pipeline is not initialized.
 */
RMFResult HNSourcePrivate::play(float& speed, double& time)
{
    RMFResult res = RMF_RESULT_SUCCESS;

    /* Call player Pause */
    if (speed == 0.0)
    {
        pausePlayback();
        setValidTrailerReceived(false); /* Reset trailer processing variables */
        return res;
    }

    HNSRCLOG_WARN("HNSource: Play with Speed (%f) & time(%f) Called....\n", speed, time);

    /* Return if the FF requested when the video is already in live */
    if (m_isLIVEAsset && isLiveSource() && (speed > 1.0)) {
        setValidTrailerReceived(false); /* Reset trailer processing variables */
        return res;
    }

    /* update the time with guardband */
    if ((!m_isVODAsset) && (m_videoLength != 0) && (time >= (m_videoLength - RMF_LIVE_TSB_GUARDBAND)))
        time = m_videoLength - RMF_LIVE_TSB_GUARDBAND;

    /* The code below may contradict code above; but it is logical and it will execution will never fall to both the condition */
    /* Added RMF_LIVE_TSB_GUARDBAND back to position to compensate the initial Pause adjustment */
    if ((m_isLIVEAsset) && (m_isPausedAtLive) && ((speed == 0.5) || (speed == -4.0)))
        time += RMF_LIVE_TSB_GUARDBAND;

    m_isPausedAtLiveAfterTSBLimit = false;
    /* Reset m_timeWhenPausedAtLive as it is invalid now */
    m_timeWhenPausedAtLive = 0.0;
    m_currPosBkup = 0;

    RMFState cur_state;
    if (getState(&cur_state, NULL) == RMF_STATE_CHANGE_FAILURE)
        return RMF_RESULT_INTERNAL_ERROR;

    if (m_VODKeepPipelinePlaying) {
        if (cur_state < RMF_STATE_PAUSED) {
            handleTricMode(GST_STATE_NULL); /* NULL the Pipeline - Handle the Trick Mode */
        }
        else if (!m_isVODAsset || m_VODAssetStartup) {
#ifdef USE_GST1
            if(getValidTrailerReceived() || m_isPausedAtLive || m_isPaused) {
                handleTricMode(GST_STATE_PAUSED, GST_PAD_PROBE_TYPE_BLOCKING); /* Pause the Pipeline - Handle the Trick Mode */
            }
            else {
                handleTricMode(GST_STATE_PAUSED); /* Pause the Pipeline - Handle the Trick Mode */
            }
#else
            handleTricMode(GST_STATE_PAUSED); /* Pause the Pipeline - Handle the Trick Mode */
#endif
        }
    }
    else {
        if (cur_state < RMF_STATE_PAUSED) {
            handleTricMode(GST_STATE_NULL); /* NULL the Pipeline - Handle the Trick Mode */
        }
        else {
            handleTricMode(GST_STATE_PAUSED); /* Pause the Pipeline - Handle the Trick Mode */
        }
    }

    m_isPausedAtLive = false;
    m_seekTime = time;
    m_speed = speed;

    if (m_isVODAsset)
        updateSourcetoLive (true);
    else
        updateSourcetoLive (false);

    /* It is configured to be handle Low Bit Rate */
    if(m_configureLowBitRate && isLiveSource())
        g_object_set (G_OBJECT (m_source), "low-bitrate-content", TRUE, NULL);

    if (m_VODKeepPipelinePlaying) {
        if (!m_isVODAsset || m_VODAssetStartup) {
            res = setSourceHeaders(m_source, speed, time);

            if(m_dtcp)
            {
                HNSRCLOG_INFO("%s:: Setting DTCP source IP: '%s' and port = '%d' properties to dtcpdec element\n", __FUNCTION__, m_dtcpServerIp.c_str(), DTCP_SERVER_PORT);
                g_object_set(m_dtcp, "dtcp_src_ip", m_dtcpServerIp.c_str(), NULL);
                g_object_set(m_dtcp, "dtcp_port", DTCP_SERVER_PORT, NULL);
            }

                notifySpeedChange(speed);
        }
    }
    else {
        res = setSourceHeaders(m_source, speed, time);

        if(m_dtcp)
        {
            HNSRCLOG_INFO("%s:: Setting DTCP source IP: '%s' and port = '%d' properties to dtcpdec element\n", __FUNCTION__, m_dtcpServerIp.c_str(), DTCP_SERVER_PORT);
            g_object_set(m_dtcp, "dtcp_src_ip", m_dtcpServerIp.c_str(), NULL);
            g_object_set(m_dtcp, "dtcp_port", DTCP_SERVER_PORT, NULL);
        }

            notifySpeedChange(speed);
    }

    /* Enabled the timeout. But kept higher value  */
    /* Setting 30s timeout for VOD and PPV Purchase 10s for all other videos */
    if (m_isVODAsset || m_isPPVAssetPur)
        g_object_set (G_OBJECT (m_source), "timeout", 30, NULL);
    else
        g_object_set (G_OBJECT (m_source), "timeout", 10, NULL);


    if (m_VODKeepPipelinePlaying) {
        if (!m_isVODAsset || m_VODAssetStartup) {
            gst_element_sync_state_with_parent(HN_BIN);
        }
    }
    else {
        gst_element_sync_state_with_parent(HN_BIN);
    }

    m_isPaused = false;

#ifdef USE_GST1
    g_mutex_lock (&gstStateMutex);
    gst_element_set_state(getPipeline(), GST_STATE_PLAYING);
    g_mutex_unlock (&gstStateMutex);
#else
    gst_element_set_state(getPipeline(), GST_STATE_PLAYING);
#endif

    if (m_VODKeepPipelinePlaying) {
        if (m_isVODAsset && m_VODAssetStartup) {
            m_VODAssetStartup = false;
        }
    }

    setValidTrailerReceived(false); /* Reset trailer processing variables */
    return res;
}

/**
 * @fn RMFResult HNSourcePrivate::setVideoLength (double length)
 * @brief This function is used to set the video length of the TSB buffer size.
 * The application calls this api periodically and updates the video length size.
 *
 * @param[in] duration video length
 * @return RMFResult
 * @retval RMF_RESULT_SUCCESS If video length has set successfully.
 */
RMFResult HNSourcePrivate::setVideoLength (double length)
{
    if (length >= 0)
        m_videoLength = length;
    else
        m_videoLength = 0;

    return RMF_RESULT_SUCCESS;
}

/**
 * @fn RMFResult HNSourcePrivate::playAtLivePosition(double duration)
 * @brief This function is mainly for linear video playing and it plays at live position.
 * When the function is called, it will update the state from  live TSB to live playback. And sets the trickplay speed to normal (1.0).
 * If current RMF state is in paused state, gstreamer sets the state into NULL.
 * Else, gstreamer sets the state into paused state.
 * Gstreamer sets the dtcp properties as dtcp source ip and dtcp port into dtcp dec element.
 * It sets the gstreamer properties on an object with timeout of 30 seconds if it VODAsset is true. Else timeout has set to 10 seconds.
 * Gstreamer tries to change the state of the gstreamer element to the same as its parent.
 *
 * @param[in] duration Length of the TSB.
 * @return RMFResult
 * @retval RMF_RESULT_SUCCESS If successfully started the pipeline to play .
 * @retval RMF_RESULT_NOT_INITIALIZED If gstreamer pipeline is not intialized.
 * @retval RMF_RESLUT_INTERNAL_ERROR If RMF state is failed.
 */
RMFResult HNSourcePrivate::playAtLivePosition(double duration)
{
    if (!getPipeline())
        return RMF_RESULT_NOT_INITIALIZED;

    if (m_isErrorReported)
        return RMF_RESULT_SUCCESS;

    HNSRCLOG_WARN("HNSource: playAtLivePosition Called....\n");

    RMFState cur_state = RMF_STATE_NULL;

    if (getState(&cur_state, NULL) == RMF_STATE_CHANGE_FAILURE)
        return RMF_RESULT_INTERNAL_ERROR;

    m_seekTime = duration;
    m_speed = 1.0;
    m_timeWhenPausedAtLive = 0.0;
    m_currPosBkup = 0;

    if (cur_state < RMF_STATE_PAUSED) {
        handleTricMode(GST_STATE_NULL); /* NULL the Pipeline - Handle the Trick Mode */
    }
    else {
#ifdef USE_GST1
        if(getValidTrailerReceived() || m_isPausedAtLive || m_isPaused) {
            handleTricMode(GST_STATE_PAUSED, GST_PAD_PROBE_TYPE_BLOCKING); /* Pause the Pipeline - Handle the Trick Mode */
        }
        else {
            handleTricMode(GST_STATE_PAUSED); /* Pause the Pipeline - Handle the Trick Mode */
        }
#else
        handleTricMode(GST_STATE_PAUSED); /* Pause the Pipeline - Handle the Trick Mode */
#endif
    }

    m_isPaused = false;
    m_isPausedAtLive = false;
    m_isPausedAtLiveAfterTSBLimit = false;


    if(m_dtcp)
    {
        HNSRCLOG_INFO("%s:: Setting DTCP source IP: '%s' and port = '%d' properties to dtcpdec element\n", __FUNCTION__, m_dtcpServerIp.c_str(), DTCP_SERVER_PORT);
        g_object_set(m_dtcp, "dtcp_src_ip", m_dtcpServerIp.c_str(), NULL);
        g_object_set(m_dtcp, "dtcp_port", DTCP_SERVER_PORT, NULL);
    }

    updateSourcetoLive (true);

    /* It is configured to be handle Low Bit Rate */
    if(m_configureLowBitRate && isLiveSource())
        g_object_set (G_OBJECT (m_source), "low-bitrate-content", TRUE, NULL);

    notifySpeedChange(m_speed);

    if (m_isVODAsset)
        g_object_set (G_OBJECT (m_source), "timeout", 30, NULL);
    else
        g_object_set (G_OBJECT (m_source), "timeout", 10, NULL);


    gst_element_sync_state_with_parent(HN_BIN);

#ifdef USE_GST1
    g_mutex_lock (&gstStateMutex);
    gst_element_set_state(getPipeline(), GST_STATE_PLAYING);
    g_mutex_unlock (&gstStateMutex);
#else
    gst_element_set_state(getPipeline(), GST_STATE_PLAYING);
#endif

    return RMF_RESULT_SUCCESS;
}

/**
 * @fn RMFResult HNSourcePrivate::getSpeed(float& speed)
 * @brief This function is used to fetch the speed which has been set .
 *
 * @param[in] speed Play speed with passed value.
 *
 * @return RMFResult
 * @retval RMF_RESULT_SUCCESS If successfully fetch the speed.
 */
RMFResult HNSourcePrivate::getSpeed(float& speed)
{
    speed = m_speed;
    return RMF_RESULT_SUCCESS;
}

/**
 * @fn RMFResult HNSourcePrivate::setSpeed(float speed)
 * @brief When the video is playing or in paused state, the rate at which video is played can be altered through this function.
 * Function gets the current position from media time and does the seek operations.
 * Default speed is 1.0 (normal speed) through this trickplay can be achieved.
 *
 * @param[in] speed Set the play speed with passed value.
 *
 * @return RMFResult
 * @retval RMF_RESULT_SUCCESS If successfully gstreamer pipeline has set and play speed has been set.
 * @retval RMF_RESULT_FAILURE Failed to set the play speed.
 */
RMFResult HNSourcePrivate::setSpeed(float speed)
{
    if (!getPipeline())
        return RMF_RESULT_SUCCESS;

    if (m_speed == speed)
        return RMF_RESULT_SUCCESS;

    if (m_isErrorReported)
        return RMF_RESULT_SUCCESS;

    HNSRCLOG_WARN("HNSource: SetSpeed Called....\n");
    double cur_pos = m_seekTime;
    getMediaTime(cur_pos);

    return play (speed, cur_pos);
}

/**
 * @fn RMFResult HNSourcePrivate::getMediaTime(double& t)
 * @brief When video is in playing or paused state, this function fetches the current time position of the total time and assigns it to variable t.
 * Gstreamer gets the properties of content-length to set the TSB length.
 * Gstreamer query an element (playbin element) for the stream position in nanoseconds when
 * the pipeline is in prerolled state (i.e.PAUSED or PLAYING state).
 * This will be a value of between 0 and the stream duration (if the stream duration is known).
 *
 * @param[out] time Returns the current media time.
 *
 * @return RMFResult
 * @retval RMF_RESULT_SUCCESS If current media time received successfully.
 * @retval RMF_RESULT_FAILURE If failed to get the curremt media time.
 * @retval RMF_RESLUT_INTERNAL_ERROR If RMF state is failed.
 * @retval RMF_RESULT_NOT_INITIALIZED If gstreamer pipeline is not initialized.
 */
RMFResult HNSourcePrivate::getMediaTime(double& t)
{
    if (!getPipeline())
    {
        t = 0;
        return RMF_RESULT_NOT_INITIALIZED;
    }

    RMFState cur_state = RMF_STATE_NULL;
    if (getState(&cur_state, NULL) == RMF_STATE_CHANGE_FAILURE)
        return RMF_RESULT_INTERNAL_ERROR;

    if (cur_state < RMF_STATE_PAUSED)
    {
        t = m_seekTime;
        return RMF_RESULT_SUCCESS;
    }

    if (!m_gotMediaLengthRespose)
    {
        long tempLength = 0;
        g_object_get (G_OBJECT (m_source), "content-length", &tempLength, NULL);
        if (tempLength >= 0)
        {
            m_gotMediaLengthRespose = true;
            HNSRCLOG_WARN ("HNSource: Update the baseTime to %ld as TSB Length\n", tempLength);
            m_seekTime = tempLength;
        }
    }

    gint64 position = 0;
    static guint64 lastSysTime = 0;

#ifdef USE_GST1
    if (!gst_element_query_position(getPipeline(), GST_FORMAT_TIME, &position))
#else
    GstFormat format = GST_FORMAT_TIME;
    if (!gst_element_query_position(getPipeline(), &format, &position))
#endif
    {
        HNSRCLOG_WARN ("HNSource: gst_element_query_position FAILED using m_currPosBkup %" G_GINT64_FORMAT " \n", m_currPosBkup);
        position = m_currPosBkup;
    }
    else
    {
        struct timespec tv;
        guint64 sysTime = 0;

        /* clock_gettime() is impervious to settimeofday() changes, gettimeofday() isn't */
        clock_gettime(CLOCK_MONOTONIC, &tv);
        sysTime= (((guint64)tv.tv_sec) * 1000 + ((guint64)tv.tv_nsec) / 1000000);

        guint64 temp = abs((position - m_currPosBkup)/1000000);
        /* In WebKit case, this query is made every 300ms; so difference of 1.5 sec is what is being logged as warning. */
        if ((temp > 1500) && (m_currPosBkup != 0))
        {
            guint64 tempTimeDiff = (sysTime - lastSysTime);
            gint64 diffPosTime = temp - tempTimeDiff;

            /* empirical test shows typical expected time diff between queries is 250ms; 
            /* warning is logged if difference is 1250ms beyond that AND timer was not blocked an equal amount of time */
            if (abs(diffPosTime) > 1250)
            {
                HNSRCLOG_WARN ("HNSOURCE_POSITION_JUMP Difference in Last Reported & "
                          "Current is higher; Position = %" G_GINT64_FORMAT
                          " Last_Reported = %" G_GINT64_FORMAT
                          " & diff = %" G_GINT64_FORMAT 
                          " sys time diff = %" G_GINT64_FORMAT 
                          " ms \n", position, m_currPosBkup, temp, (sysTime - lastSysTime));
            }
        }

        lastSysTime = sysTime;
        m_currPosBkup = position;
    }

    /** For PTS Restamping, Rough Calculation. It should be, MTbase+(MTpts – MTbase) X play_rate  */
    t = (double) GST_TIME_AS_NSECONDS(position);
    t = t/1000000000.0L;
    t = std::max(0., t); // prevent negative offsets
    t = m_seekTime + t * m_speed;
    t = std::max(0., t); // prevent negative offsets

    if ((t == 0) && (m_speed != 1.0) && !isTrailerErrorAvailable())
    {
        HNSRCLOG_WARN ("HNSource::getMediaTime returns position as 0; User might possibly notice positioning issue..\n");
    }

    return RMF_RESULT_SUCCESS;
}

/**
 * @fn RMFResult HNSourcePrivate::setMediaTime(double time)
 * @brief This API is used to set the media time according to the input parameter.
 * Function is used to set the play mode from the given time and speed.
 *
 * @param[in] time Set the media time with the passed value .
 *
 * @return RMFResult
 * @retval RMF_RESULT_SUCCESS If media time has set successfully.
 * @retval RMF_RESULT_FAILURE If failed to set the media time.
 * @retval RMF_RESULT_INTERNAL_ERROR If RMF state is failed.
 * @retval RMF_RESULT_NOT_INITIALIZED If gstreamer pipeline is not initialized.
 */
RMFResult HNSourcePrivate::setMediaTime(double time)
{
    float speed = 1.0;
    if (!getPipeline())
        return RMF_RESULT_NOT_INITIALIZED;

    if (m_isErrorReported)
        return RMF_RESULT_SUCCESS;

    HNSRCLOG_WARN("HNSource: setMediaTime Called....\n");

    return play (speed, time);
}

RMFResult HNSourcePrivate::populateBin(GstElement* bin)
{
    int blocksize = 128*1024;
    char *pHttpSrc = NULL;
    GstElement* source = NULL;

    pHttpSrc = getenv("RMF_USE_SOUPHTTPSRC");
    if ((pHttpSrc != NULL) && (strcasecmp(pHttpSrc, "TRUE") == 0))
        source = gst_element_factory_make("souphttpsrc", "soup");
    else
        source = gst_element_factory_make("httpsrc", "curl");

    if (!source)
    {
        // replace with error callback
        HNSRCLOG_ERROR("HNSource: failed to instantiate 'httpsrc' element\n");
        RMF_ASSERT(source);
        return RMF_RESULT_INTERNAL_ERROR;
    }

    gst_bin_add(GST_BIN(bin), source);
    g_object_set(G_OBJECT (source), "blocksize", blocksize,  NULL);

    GstElement* last_el = source;
    bool isDtcpIpEnabled = m_url.find("DTCP1HOST") != std::string::npos;
    if (isDtcpIpEnabled)
    {
        // Export source host IP.
        static int http_len = std::string("http://").length();
        int ip_start_pos = m_url.find("http://") + http_len;
        int ip_end_pos = m_url.find(":",ip_start_pos);
        std::string ip_addr = m_url.substr(ip_start_pos, ip_end_pos-ip_start_pos);

        // Create DTCP filter.
        //GstElement* dtcp = gst_element_factory_make("vldtcpip", "dtcp");
        GstElement* dtcp = gst_element_factory_make("dtcpdec", "dtcpdecrypt");
        if (!dtcp)
        {
            HNSRCLOG_ERROR("HNSource: Failed to create DTCP filter\n");
            RMF_ASSERT(dtcp);
            return RMF_RESULT_INTERNAL_ERROR;
        }
        HNSRCLOG_INFO("%s::Created DTCP decrypt element\n", __FUNCTION__);

        m_dtcpServerIp = ip_addr;
        HNSRCLOG_INFO("%s:: Setting DTCP source IP: '%s' and port = '%d' properties to dtcpdec element\n", __FUNCTION__, m_dtcpServerIp.c_str(), DTCP_SERVER_PORT);
        g_object_set(dtcp, "dtcp_src_ip", ip_addr.c_str(), NULL);
        g_object_set(dtcp, "dtcp_port", DTCP_SERVER_PORT, NULL);
        g_object_set(G_OBJECT (dtcp), "buffersize", blocksize,  NULL);

        gst_bin_add(GST_BIN(bin), dtcp);

        // Link the filter to the soup source.
        if (!gst_element_link(source, dtcp))
        {
            HNSRCLOG_ERROR("HNSource: Failed to link source and filter\n");
            return RMF_RESULT_INTERNAL_ERROR;
        }

        m_dtcp = dtcp;
        last_el = dtcp;
    }
    else
    {
        m_dtcp = NULL;
    }

    // Point the bin ghost pad to the last bin's element src pad.
    setGhostPadTarget(bin, gst_element_get_static_pad(last_el, "src"), true);
    g_object_set (G_OBJECT (source), "location", m_url.c_str(), NULL);

    if (m_isLIVEAsset)
    {
        // Set the appropriate request headers.
        GstStructure *headers;
        headers = gst_structure_new("extra-headers",
                                    "getAvailableSeekRange.dlna.org", G_TYPE_STRING, "1",
                                     NULL);
        g_object_set(source, "extra-headers", headers, NULL);
        gst_structure_free(headers);
    }
    else
        m_gotMediaLengthRespose = true;

    m_source = source;
    return RMF_RESULT_SUCCESS;
}

// static
void HNSourcePrivate::setGhostPadTarget(GstElement* bin, GstPad* target, bool unref_target)
{
    GstPad* bin_src_pad = gst_element_get_static_pad(bin, "src");
    RMF_ASSERT(gst_ghost_pad_set_target((GstGhostPad*) bin_src_pad, target) == true);
    gst_object_unref(bin_src_pad);

    if (unref_target && target)
        gst_object_unref(target);
}

// static
RMFResult HNSourcePrivate::setSourceHeaders(GstElement* source, double speed, double pos)
{
    // Format speed and position according to the DLNA standard.
    char speed_str[64];
    char time_str[64];

    snprintf(speed_str, sizeof(speed_str), "speed=%f", speed);
    HNSRCLOG_WARN("DLNA speed: [%s]\n", speed_str);

    unsigned h = 0, m = 0;
    float s = 0;
    time_to_hms(pos, h, m, s);

    snprintf(time_str, sizeof(time_str), "npt=%02u:%02u:%.2f-", h, m, s);
    HNSRCLOG_WARN("DLNA position: [%s] (%f)\n", time_str, pos);

    // Set the appropriate request headers.
    GstStructure *headers;
    headers = gst_structure_new("extra-headers",
        "PlaySpeed.dlna.org",       G_TYPE_STRING,  speed_str,
        "TimeSeekRange.dlna.org",   G_TYPE_STRING,  time_str,
        NULL);
    RMF_ASSERT(source);
    g_object_set(source, "extra-headers", headers, NULL);
    gst_structure_free(headers);

    return RMF_RESULT_SUCCESS;
}

void HNSourcePrivate::onEOS()
{
    if (m_speed < 0)
        m_seekTime = 0.0;

    if (isTrailerErrorAvailable()) {
        char errorMsg[256] = {'\0'};
        if (getRMFTrailerError(errorMsg, sizeof(errorMsg) - 1) == RMF_RESULT_SUCCESS) {
            std::string tempStr = errorMsg;
            std::size_t isFound;

            if ((isFound = tempStr.find("Authorization failure")) != std::string::npos) {
                m_isErrorReported = true;
                RMFMediaSourcePrivate::onError(RMF_RESULT_HNSRC_TRAILER_ERROR, errorMsg);
                return;
            }
            else if ((isFound = tempStr.find("TRM cancel")) != std::string::npos) {
                m_isErrorReported = true;
                HNSRCLOG_WARN("%s - TRM cancel trailer received\n", __FUNCTION__ );
                RMFMediaSourcePrivate::onError(RMF_RESULT_HNSRC_TRAILER_ERROR, errorMsg);
                return;
            }
            else if (m_isLIVEAsset && ((isFound = tempStr.find("003:Playback completed")) == std::string::npos)) {
                m_isErrorReported = true;
                RMFMediaSourcePrivate::onError(RMF_RESULT_HNSRC_TRAILER_ERROR, errorMsg);
                return;
            }
            else if (m_isVODAsset && ((isFound = tempStr.find("003:Playback completed")) == std::string::npos)) {

                if ((getErrorCode((gchar*)m_trailerData.c_str()) != VOD_PLAYBACK_COMPLETED_TRAILER_ERROR_CODE) || !m_isPaused) {
                    m_isErrorReported = true;
                    RMFMediaSourcePrivate::onError(RMF_RESULT_HNSRC_TRAILER_ERROR, errorMsg);
                    return;
                }
                else if ((getErrorCode((gchar*)m_trailerData.c_str()) == VOD_PLAYBACK_COMPLETED_TRAILER_ERROR_CODE) && m_isPaused) {
                    HNSRCLOG_WARN("HNSource: VOD asset in Paused state for 10 min Exit With Warning!\n");
                    if (!m_isErrorReported) {
                        char warningMsg[64]= {'\0'};
                        strcpy(warningMsg, "VOD-Pause-10Min-Timeout");
                        m_isErrorReported = true;
                        RMFMediaSourcePrivate::onError(RMF_RESULT_HNSRC_VOD_PAUSE_TIMEOUT, warningMsg);
                        return;
                    }
                }
            }
        }
    }

    HNSRCLOG_WARN ("HNSource:: Send EOS event..!\n");
    RMFMediaSourcePrivate::onEOS();
    return;
}

void HNSourcePrivate::onPlaying()
{
    HNSRCLOG_WARN("Reset Trailer Processing\n");
    setValidTrailerReceived(false);
    RMFMediaSourcePrivate::onPlaying();
    return;
}

void HNSourcePrivate::onError(RMFResult err, const char* msg)
{
    std::string tempStr = msg;
    std::size_t isFound = tempStr.find("CA_ERROR");

    if (m_isErrorReported)
        return;

    if (isFound != std::string::npos) {
        m_isErrorReported = true;
        RMFMediaSourcePrivate::onError(RMF_RESULT_HNSRC_LEGACY_CA_ERROR, msg);
        return;
    }
    /* The Mitigation added for XITHREE-3659 has been removed. (Broadcom made the fix in DTCPMgr) */
    else if (tempStr.find("determine type of stream") != std::string::npos)
    {
        m_isErrorReported = true;
        HNSRCLOG_WARN ("typefind is not able to identify the stream. It could be DTCP decryption not proper..!\n");
        RMFMediaSourcePrivate::onError(RMF_RESULT_HNSRC_FORMAT_ERROR, msg);
        return;
    }
    else if (tempStr.find("Cancelled") != std::string::npos)
    {
        /* Just Ignore the user cancellation */
        return;
    }
    else if (isTrailerErrorAvailable())
    {
        char errorMsg[256] = {'\0'};
        if (getRMFTrailerError(errorMsg, sizeof(errorMsg) - 1) == RMF_RESULT_SUCCESS)
        {
            std::string errStr = errorMsg;
            HNSRCLOG_WARN ("HNSrc: ERROR MESSAGE - %s\n", errorMsg);

            if (!m_isLIVEAsset &&
                !m_isVODAsset  &&
                (tempStr.find("DTCP_PCP_HEADER_DECODE_FAILURE") != std::string::npos) &&
                (errStr.find("003:Playback completed") != std::string::npos)) {

                if (m_speed < 0)
                    m_seekTime = 0.0;
#ifdef USE_GST1
                HNSRCLOG_WARN ("HNSource::DTCP_PCP_ERROR for DVR/mDVR; do not post Error as it is EOS case..!\n");
#else
                HNSRCLOG_WARN ("HNSource:: Send Fake EOS event on DTCP PCP Error for DVR/mDVR..!\n");
                RMFMediaSourcePrivate::onEOS();
#endif /* USE_GST1 */
                return;
            }
            else {
                m_isErrorReported = true;
                RMFMediaSourcePrivate::onError(RMF_RESULT_HNSRC_TRAILER_ERROR, errorMsg);
                return;
            }
        }
    }
    else if (tempStr.find("DTCP_ERR_SERVER_NOT_REACHABLE") != std::string::npos)
    {
        m_isErrorReported = true;
        RMFMediaSourcePrivate::onError(RMF_RESULT_HNSRC_DTCP_CONN_ERROR, msg);
        return;
    }
    else if (tempStr.find("Cannot connect to destination") != std::string::npos)
    {
        m_isErrorReported = true;
        RMFMediaSourcePrivate::onError(RMF_RESULT_HNSRC_NETWORK_ERROR, msg);
        return;
    }
    /* For VOD, when the 30s timeout is happening and no TS data received (may be RMFStreamer is still trying to get talk to VOD Server by multiple redirect),
     * The Player Sends Connection Closed as error msg and the eventually 'Resource Not Available' as error string; But for VOD we have dedicated error code for this as SRM-521.
     * This below change is to send the right errorMsg; Sending it in trailer format for easy parsing/syncing
     */
    else if (m_isVODAsset && (tempStr.find("server closed the connection") != std::string::npos))
    {
        char errorMsg[256] = "130:ErrorCode:SRM-521";
        m_isErrorReported = true;
        RMFMediaSourcePrivate::onError(RMF_RESULT_HNSRC_VOD_TUNE_TIMEOUT, errorMsg);
        return;
    }
    else
    {
        m_isErrorReported = true;
        /* We will reach here only when Trailer Header is not available */
        RMFMediaSourcePrivate::onError(RMF_RESULT_HNSRC_GENERIC_ERROR, msg);
    }
}

#ifdef USE_GST1
void HNSourcePrivate::onPadBlocked()
{
    double cur_pos = 0;

    g_mutex_lock (&padBlockMutex);

    HNSRCLOG_WARN("HNSource::handleElementMsg Received PSI Update message taking pipeline to PAUSED\n");

    g_mutex_lock (&gstStateMutex);
    setState(GST_STATE_PAUSED);
    g_mutex_unlock (&gstStateMutex);

    if(GST_STATE_PAUSED != validateStateWithMsTimeout(GST_STATE_PAUSED, 50)) { // 50 ms
        HNSRCLOG_WARN("HNSource::handleElementMsg - validateStateWithMsTimeout - FAILED GstState %d\n", GST_STATE_PAUSED);
    }
    getMediaTime(cur_pos);
    m_seekTime = cur_pos;
    HNSRCLOG_WARN("HNSource::handleElementMsg Received PSI Update current Position = %f\n", m_seekTime);

    HNSRCLOG_WARN("HNSource::handleElementMsg Received PSI Update FLUSH PIPELINE\n");
    flushPipeline();

    HNSRCLOG_WARN("HNSource::handleElementMsg Received PSI Update message taking pipeline to PLAYING\n");

    g_mutex_lock (&gstStateMutex);
    setState(GST_STATE_PLAYING);
    g_mutex_unlock (&gstStateMutex);

    if(GST_STATE_PLAYING != validateStateWithMsTimeout(GST_STATE_PLAYING, 50)) { // 50 ms
        HNSRCLOG_WARN("HNSource::handleElementMsg - validateStateWithMsTimeout - FAILED GstState %d\n", GST_STATE_PLAYING);
    }

    padBlockDone = true;

    g_cond_signal (&padBlockCond);
    g_mutex_unlock (&padBlockMutex);
}

static GstPadProbeReturn onPadBlocked(GstPad* pad, GstPadProbeInfo* info, gpointer userdata)
{
    (void) pad;
    (void) info;
    HNSourcePrivate *hnsp=(HNSourcePrivate*)userdata;
    HNSRCLOG_WARN("HNSource::handleElementMsg - onPadBlocked - Pad BLOCKED\n");
    hnsp->onPadBlocked();
    return GST_PAD_PROBE_REMOVE;
}

void HNSourcePrivate::onPadBlockedTricMode()
{
    g_mutex_lock (&padBlockMutex);

    padBlockDone = true;

    g_cond_signal (&padBlockCond);
    g_mutex_unlock (&padBlockMutex);
}


static GstPadProbeReturn onPadBlockedTricMode(GstPad* pad, GstPadProbeInfo* info, gpointer userdata)
{
    (void) pad;
    (void) info;
    HNSourcePrivate *hnsp=(HNSourcePrivate*)userdata;
    HNSRCLOG_WARN("HNSource::handleTricMode - onPadBlockedTricMode - Pad BLOCKED\n");
    hnsp->onPadBlockedTricMode();
    return GST_PAD_PROBE_OK;
}
#endif

RMFResult HNSourcePrivate::handleElementMsg (GstMessage *msg)
{
    if (msg == NULL) {
        return RMF_RESULT_FAILURE;
    }

    const GstStructure  *gstStructure = gst_message_get_structure (msg);
    const gchar         *gstStructureName = gst_structure_get_name (gstStructure);
    const GValue        *value;

    if (!strcmp(gst_object_get_name(GST_OBJECT(msg->src)), gst_object_get_name(GST_OBJECT(m_source)))) {
        gboolean validTrailer = FALSE;
        HNSRCLOG_WARN("HANDLE GST_MESSAGE_ELEMENT FOR TRAILER\n");

        if (strcmp(gstStructureName, "http/trailer") == 0) {
            value = gst_structure_get_value(gstStructure, "valid");
            /* Get if a valid trailer has been received in this layer */
            validTrailer = g_value_get_boolean(value);
        }

        if (validTrailer == TRUE) {
            /* Valid trailer is receivd, set it in the application */
            HNSRCLOG_WARN("Received VALID TRAILER\n");
            setValidTrailerReceived(true);

            if (m_isVODAsset) {
                /* Since the we switched to keep the pipeline PLAYING for VOD on PAUSE, the below is code is not needed; The below mitigation was added when we had GST_PAUSE when pause() was called for VOD Assets */
                if (m_isPaused && !m_VODKeepPipelinePlaying) {
                    HNSRCLOG_WARN("HNSource: VOD asset in Paused state for 10 min!\n");
                    retrieveAndStoreTrailer();
                    if (getErrorCode((gchar*)m_trailerData.c_str()) == VOD_PLAYBACK_COMPLETED_TRAILER_ERROR_CODE) {
                        HNSRCLOG_WARN("HNSource: VOD asset in Paused state for 10 min Exit With Warning!\n");
                        if (!m_isErrorReported) {
                            char warningMsg[64]= {'\0'};
                            strcpy(warningMsg, "VOD-Pause-10Min-Timeout");
                            m_isErrorReported = true;
                            RMFMediaSourcePrivate::onError(RMF_RESULT_HNSRC_VOD_PAUSE_TIMEOUT, warningMsg);
                        }
                    }
                }
                // Abrupt termination - EOS should be propagated to sink element which should generate the Event.
                // But as we dont receive any data for the new VOD URL request at the position downstream elements seem to miss the EOS event generated at gstsoup
                else if (m_speed == 1.0) {
                    GstState gst_current;
                    GstState gst_pending;
                    float timeout = 100.0;
                    double cur_pos = 0.0;

                    getMediaTime(cur_pos);

                    if (GST_STATE_CHANGE_FAILURE  == gst_element_get_state(getPipeline(), &gst_current, &gst_pending, timeout * GST_MSECOND)) {
                        HNSRCLOG_WARN("HNSource::handleElementMsg - PIPELINE gst_element_get_state - FAILURE : State = %d, Pending = %d\n", gst_current, gst_pending);
                    }

                    if (!m_isErrorReported ) {
                        HNSRCLOG_WARN("HNSource::handleElementMsg - PIPELINE gst_element_get_state - SUCCESS : State = %d, Pending = %d\n", gst_current, gst_pending);
                        HNSRCLOG_WARN("HNSource: VOD asset Received EOS\n");
                        retrieveAndStoreTrailer();
                        if ((cur_pos == m_seekTime) && ((gst_current == GST_STATE_PAUSED) || (gst_current == GST_STATE_PLAYING))) {
                            if (getErrorCode((gchar*)m_trailerData.c_str()) == VOD_PLAYBACK_EOS_TRAILER_ERROR_CODE) {
                                HNSRCLOG_WARN("HNSource:: Send EOS event for VOD..!\n");
                                m_isErrorReported = true;
                                RMFMediaSourcePrivate::onEOS();
                            }
                        }
                        else if (getErrorCode((gchar*)m_trailerData.c_str()) == VOD_PLAYBACK_SRM_26_TRAILER_ERROR_CODE) {
                            HNSRCLOG_WARN("HNSource: TRAILER ERROR = %s\n", m_trailerData.c_str());
                            m_isErrorReported = true;
                            RMFMediaSourcePrivate::onError(RMF_RESULT_HNSRC_TRAILER_ERROR, m_trailerData.c_str());
                        }
                    }
                }
                else if (!m_isErrorReported) {
                    retrieveAndStoreTrailer();
                    if (getErrorCode((gchar*)m_trailerData.c_str()) == VOD_PLAYBACK_SRM_26_TRAILER_ERROR_CODE) {
                        HNSRCLOG_WARN("HNSource: TRAILER ERROR = %s\n", m_trailerData.c_str());
                        m_isErrorReported = true;
                        RMFMediaSourcePrivate::onError(RMF_RESULT_HNSRC_TRAILER_ERROR, m_trailerData.c_str());
                    }
                    else if (getErrorCode((gchar*)m_trailerData.c_str()) == VOD_LIVE_TUNE_FAIL_TOUT_TRAILER_ERROR_CODE) {
                        HNSRCLOG_WARN("HNSource: TRAILER ERROR = %s\n", m_trailerData.c_str());
                        m_isErrorReported = true;
                        RMFMediaSourcePrivate::onError(RMF_RESULT_HNSRC_TRAILER_ERROR, m_trailerData.c_str());
                    }
                }
            }
            else if (m_isLIVEAsset) {
                if (!m_isErrorReported) {
                    if (getErrorCode((gchar*)m_trailerData.c_str()) == VOD_LIVE_TUNE_FAIL_TOUT_TRAILER_ERROR_CODE) {
                        HNSRCLOG_WARN("HNSource: TRAILER ERROR = %s\n", m_trailerData.c_str());
                        m_isErrorReported = true;
                        RMFMediaSourcePrivate::onError(RMF_RESULT_HNSRC_TRAILER_ERROR, m_trailerData.c_str());
                    }
                }
            }
        }
        else {
            /* Trailer is received in the lower layer, but its format does not meet the one sent across in the header */
            HNSRCLOG_WARN("Received INVALID TRAILER\n");
            setValidTrailerReceived(false);
        }

        return RMF_RESULT_SUCCESS;
    }

    if (strcmp(gstStructureName, "demux/pmtUpdate") == 0) { /* Handle PSI update message from playersinkbin for dynamic pid change*/
        gboolean validPMT = FALSE;
        HNSRCLOG_WARN("HANDLE GST_MESSAGE_ELEMENT FOR PMT UPDATE\n");

        value = gst_structure_get_value(gstStructure, "valid");
        /* Get if a valid PMT update has been received in this layer */
        validPMT = g_value_get_boolean(value);

        if (validPMT == TRUE) {
            double cur_pos = 0;

            GstPad* padToBlock = gst_element_get_static_pad(m_source, "src");
#ifdef USE_GST1
            /* gst1 code untested, the test case apparently needs some particular hardware */
            if (padToBlock) {
                gint64 padBlockCondTimeout;

                g_mutex_lock (&padBlockMutex);
                padBlockDone = false;
                gst_pad_add_probe (padToBlock, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, &::onPadBlocked, this, NULL);

                do {
                    padBlockCondTimeout = g_get_monotonic_time () + 500 * G_TIME_SPAN_MILLISECOND;
                    if (!g_cond_wait_until(&padBlockCond, &padBlockMutex, padBlockCondTimeout)) {
                        if(gst_pad_is_blocked(padToBlock) || gst_pad_is_blocking(padToBlock)) {
                            // Safe to assume that is blocked or will be blocked if data is pushed
                            // and the installed probe will kick in preventing data flow
                            padBlockDone = TRUE;
                            HNSRCLOG_WARN("HNSource::handleElementMsg - padBlockDone Timeout GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM\n");
                        }
                    }
                } while (!padBlockDone);

                g_mutex_unlock (&padBlockMutex);
                HNSRCLOG_WARN("HNSource::handleElementMsg - Pad UNBLOCKED Element - %x Pad - %x\n", m_source, padToBlock);
            }
#else
            if (gst_pad_set_blocked (padToBlock, TRUE) == TRUE) {
                HNSRCLOG_WARN("HNSource::handleElementMsg - Pad BLOCKED Element - %x Pad - %x\n", m_source, padToBlock);
            }

            HNSRCLOG_WARN("HNSource::handleElementMsg Received PSI Update message taking pipeline to PAUSED\n");
            setState(GST_STATE_PAUSED);

            if(GST_STATE_PAUSED != validateStateWithMsTimeout(GST_STATE_PAUSED, 50)) { // 50 ms
                HNSRCLOG_WARN("HNSource::handleElementMsg - validateStateWithMsTimeout - FAILED GstState %d\n", GST_STATE_PAUSED);
            }

            getMediaTime(cur_pos);
            m_seekTime = cur_pos;
            HNSRCLOG_WARN("HNSource::handleElementMsg Received PSI Update current Position = %f\n", m_seekTime);

            HNSRCLOG_WARN("HNSource::handleElementMsg Received PSI Update FLUSH PIPELINE\n");
            flushPipeline();

            HNSRCLOG_WARN("HNSource::handleElementMsg Received PSI Update message taking pipeline to PLAYING\n");
            setState(GST_STATE_PLAYING);

            if(GST_STATE_PLAYING != validateStateWithMsTimeout(GST_STATE_PLAYING, 50)) { // 50 ms
                HNSRCLOG_WARN("HNSource::handleElementMsg - validateStateWithMsTimeout - FAILED GstState %d\n", GST_STATE_PLAYING);
            }

            if (gst_pad_set_blocked (padToBlock, FALSE) == TRUE) {
                HNSRCLOG_WARN("HNSource::handleElementMsg - Pad UNBLOCKED Element - %x Pad - %x\n", m_source, padToBlock);
            }
#endif
            gst_object_unref (padToBlock);
        }
        else {
            HNSRCLOG_WARN("HNSource::handleElementMsg Received PSI Update message INVALID PMT valid flag IGNORE\n");
        }

        return RMF_RESULT_SUCCESS;
	}

    return RMF_RESULT_SUCCESS;
}


GstState
HNSourcePrivate::validateStateWithMsTimeout (
    GstState    stateToValidate,
    guint       msTimeOut
) {
    GstState    gst_current;
    GstState    gst_pending;
    float       timeout = 100.0;
    gint        gstGetStateCnt = GST_ELEMENT_GET_STATE_RETRY_CNT_MAX;

    do {
        if ((GST_STATE_CHANGE_SUCCESS == gst_element_get_state(getPipeline(), &gst_current, &gst_pending, timeout * GST_MSECOND)) && (gst_current == stateToValidate)) {
            HNSRCLOG_WARN("HNSource::validateStateWithMsTimeout - PIPELINE gst_element_get_state - SUCCESS : State = %d, Pending = %d\n", gst_current, gst_pending);
            return gst_current;
        }
        usleep(msTimeOut * 1000); // Let pipeline safely transition to required state
    } while ((gst_current != stateToValidate) && (gstGetStateCnt-- != 0)) ;

    HNSRCLOG_WARN("HNSource::validateStateWithMsTimeout - PIPELINE gst_element_get_state - FAILURE : State = %d, Pending = %d\n", gst_current, gst_pending);
    return gst_current;
}


bool HNSourcePrivate::isTrailerErrorAvailable (void)
{
    return getValidTrailerReceived();
}

RMFResult HNSourcePrivate::getRMFTrailerError (char *errorMsg, int maxLen)
{
    RMFResult     rmfResult = RMF_RESULT_FAILURE;

    if ((errorMsg == NULL) || !getValidTrailerReceived()) {
        return rmfResult;
    }

    HNSRCLOG_WARN("getRMFTrailerError - FETCH TRAILER HERE\n");
    /* Retrieve the received trailer and store it */
    retrieveAndStoreTrailer();
    strncpy(errorMsg, m_trailerData.c_str(), maxLen);
    return RMF_RESULT_SUCCESS;
}


/* Return whether a valid trailer has been received or not */
bool HNSourcePrivate::getValidTrailerReceived (void)
{
    return m_validTrailerReceived;
}

/* Set if a valid trailer is receievd */
void HNSourcePrivate::setValidTrailerReceived (bool validTrailerReceived)
{
    m_validTrailerReceived = validTrailerReceived;
}

/* Get trailer from the plugin and store it for future use */
void HNSourcePrivate::retrieveAndStoreTrailer (void)
{
    gchar* pTrailerData = NULL;

    g_object_get (G_OBJECT (m_source), "trailer", &pTrailerData, NULL);

    if (pTrailerData)
    {
        HNSRCLOG_WARN("TRAILER AT APPLICATION = %s\n", pTrailerData);
        m_trailerData = pTrailerData;
        g_free (pTrailerData);
    }

    HNSRCLOG_INFO("STORED TRAILER\n");
}

int HNSourcePrivate::getErrorCode (gchar *trailerData)
{
    if ((trailerData == NULL)) {
        return RMF_RESULT_FAILURE;
    }

    char errorCode[16] = {'\0'};
    std::string trailerStr(trailerData);
    std::string trailerKey("ErrorCode-");
    std::size_t found;

    found = trailerStr.find(trailerKey);
    strncpy(errorCode, trailerData + found + trailerKey.length(), 3);
    HNSRCLOG_WARN("ERROR CODE EXTRACTED - %s\n", errorCode);
    HNSRCLOG_WARN("ERROR CODE INT - %d\n", atoi(errorCode));    

    return atoi(errorCode);
}

double HNSourcePrivate::getSystemTimeNow()
{
   struct timeval tv;
   guint64 currentTime;
   gettimeofday( &tv, 0 );
   currentTime= (((guint64)tv.tv_sec) * 1000 + ((guint64)tv.tv_usec) / 1000);

   return ((double) (currentTime/1000.0));
}

RMFResult HNSourcePrivate::setLowBitRateMode (bool isYes)
{
    if (isYes)
    {
        m_configureLowBitRate = isYes;
        g_object_set (G_OBJECT (m_source), "low-bitrate-content", TRUE, NULL);
    }
    return RMF_RESULT_SUCCESS;
}

//-- HNSource ------------------------------------------------------------------

/**
 * @fn HNSource::HNSource()
 * @brief A Constructor of class HNSource.
 *
 * @param None
 *
 * @return None
 */
HNSource::HNSource()
{
}

/**
 * @fn HNSource::~HNSource()
 * @brief A Destructor of class HNSource.
 *
 * @param None
 *
 * @return None
 */
HNSource::~HNSource()
{
}

/**
 * @fn HNSource::getFileSize() const
 * @brief This API is obsolute.
 *
 * @param None
 *
 * @return Unsigned integer
 */
unsigned HNSource::getFileSize() const
{
    return 0;
}

/**
 * @addtogroup hnsrcapi
 * @{
 * @fn HNSource::getBufferedRanges(range_list_t& ranges)
 * @brief This API is used to get the bufferedrange of the TSB.
 * When the API is called, it will return the total video duration for the current TSB buffer.
 * This function returns a range_list which is a list and it stores the start and stop values of the buffered ranges array.
 *
 * @param[out] ranges Holds the start and stop values of media duration
 *
 * @return RMFResult
 * @retval RMF_RESULT_SUCCESS If successfully gets the bufferrange value.
 * @retval RMF_RESULT_FAILURE If failed to get the status of the buffering stream and Gstreamer version is not supported.
 */
RMFResult HNSource::getBufferedRanges(range_list_t& ranges)
{
    return HN_SOURCE_IMPL->getBufferedRanges(ranges);
}

/**
 * @fn HNSource::open(const char* url, char* mimetype)
 * @brief This API is used to open HNSource instance for the passed uri which is in the format http://.
 * Gstreamer creates the "souphttpsrc or httpsrc" and "dtcpdec" gstreamer plugins and added these plugins to bin element called "hn_source_bin".
 * Gstreamer sets the "location" property of httpsrc plugin from the url.
 *
 * @param[in] URI URI in the format http://.
 * @param[in] mimetype Not used.
 *
 * @return RMFResult
 * @retval RMF_RESULT_SUCCESS If successfully opened the url.
 * @retval RMF_RESULT_INVALID_ARGUMENT If invalid url is used.
 * @retval RMF_RESULT_FAILURE If failed to open the url.
 * @retval RMF_RESULT_INTERNAL_ERROR If failed to instantiate httpsrc element, failed to create dtcp filter and failed to link source and filter.
 */
RMFResult HNSource::open(const char* url, char* mimetype)
{
    HNSRCLOG_WARN("HNSource::open(%s, %s)\n", url, mimetype);
    return HN_SOURCE_IMPL->open(url, mimetype);
}

/**
 * @fn HNSource::term()
 * @brief This API is used to terminates the HNsource instance.
 * Function clears the url and sets the speed,source to NULL. This function removes all the sink from the pipeline and terminates the playback.
 * Sets the gstreamer state to NULL and unreferenced the gstBus also.
 *
 * @param None
 *
 * @return  RMFResult
 * @retval RMF_RESULT_SUCCESS If successfully terminated the HNsource instance.
 * @retval RMF_RESULT_FAILURE If failed to terminated the Hnsource instance.
 * @retval RMF_RESULT_NOT_INITIALIZED If gstreamer pipeline is not initialized.
 */
RMFResult HNSource::term()
{
    return HN_SOURCE_IMPL->term();
}

/**
 * @fn HNSource::getMediaTime(double& time)
 * @brief This API will fetch the current time position in the total media length.
 *
 * @param[out] time Returns the current media time in double.
 *
 * @return RMFResult
 * @retval RMF_RESULT_SUCCESS If current media time received successfully.
 * @retval RMF_RESULT_FAILURE If failed to get the curremt media time.
 * @retval RMF_RESLUT_INTERNAL_ERROR If RMF state is failed.
 * @retval RMF_RESULT_NOT_INITIALIZED If gstreamer pipeline is not initialized.
 */
RMFResult HNSource::getMediaTime(double& time)
{
    return HN_SOURCE_IMPL->getMediaTime(time);
}

/**
 * @fn HNSource::setMediaTime(double time)
 * @brief This API is used to set the media time according to the input parameter.
 * Function is used to set the play mode from the given time and speed.
 *
 * @param[in] time Set the media time with the passed value.
 *
 * @return RMFResult
 * @retval RMF_RESULT_SUCCESS If media time has set successfully.
 * @retval RMF_RESULT_FAILURE If failed to set the media time.
 * @retval RMF_RESULT_INTERNAL_ERROR If RMF state is failed.
 * @retval RMF_RESULT_NOT_INITIALIZED If gstreamer pipeline is not initialized.
 * @}
 */
RMFResult HNSource::setMediaTime(double time)
{
    return HN_SOURCE_IMPL->setMediaTime(time);
}

void* HNSource::createElement()
{
    return HN_SOURCE_IMPL->createElement();
}

// virtual
RMFMediaSourcePrivate* HNSource::createPrivateSourceImpl()
{
    return new HNSourcePrivate(this);
}

/**
 * @addtogroup hnsrcapi
 * @{
 * @fn HNSource::setSpeed(float speed)
 * @brief This API is used to set the speed for handling trick modes.
 * The rate at which video is played can be altered through this API.
 * Default speed is 1.0 (normal speed) through this trickplay can be achieved.
 *
 * @param[in] speeed Set the play speed with passed value.
 *
 * @return RMFResult
 * @retval RMF_RESULT_SUCCESS If play speed has set successfully.
 * @retval RMF_RESULT_FAILURE If failed to set the play speed.
 */
RMFResult HNSource::setSpeed(float speed)
{
    return HN_SOURCE_IMPL->setSpeed(speed);
}

/**
 * @fn HNSource::close()
 * @brief  This API is used to close the HNSource instance.
 *
 * @param None
 *
 * @return RMFResult
 * @retval RMF_RESULT_SUCCESS Closed the HNsource instance successfully
 */
RMFResult HNSource::close()
{
    return RMF_RESULT_SUCCESS;
}

/**
 * @fn HNSource::playAtLivePosition(double duration)
 * @brief This API is mainly for linear video playing and it plays at live position.
 * When the API is called, it will update the state from  live TSB to live playback. And sets the trickplay speed to normal (1.0).
 *
 * @param[in] duration Length of the TSB.
 *
 * @return RMFResult
 * @retval RMF_RESULT_SUCCESS If successfully started the pipeline into play state.
 * @retval RMF_RESULT_FAILURE If gstreamer pipeline is not intialized.
 * @retval RMF_RESLUT_INTERNAL_ERROR If RMF state is failed.
 */
RMFResult HNSource::playAtLivePosition(double duration)
{
    return HN_SOURCE_IMPL->playAtLivePosition(duration);
}

/**
 * @fn HNSource::setVideoLength (double duration)
 * @brief This API is used to set the video length of the TSB buffer size.
 * The application calls this api periodically and updates the video length size.
 *
 * @param[in] duration video length.
 *
 * @return RMFResult
 * @retval RMF_RESULT_SUCCESS Video Length has set successfully.
 @}
 */
RMFResult HNSource::setVideoLength (double duration)
{
    return HN_SOURCE_IMPL->setVideoLength(duration);
}

RMFResult HNSource::setLowBitRateMode (bool isYes)
{
    return HN_SOURCE_IMPL->setLowBitRateMode(isYes);
}

