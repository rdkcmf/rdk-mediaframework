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

#ifndef RMFPRIVATE_H
#define RMFPRIVATE_H

#include "rmfcore.h"
#include "rmf_osal_sync.h"

#include <gst/gst.h>
#include <cassert>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

#ifdef NDEBUG
#    define RMF_ABORT()
#else
#    define RMF_ABORT() abort()
#endif
#define RMF_PRINT_ASSERT_ERR(msg)    { g_print("RMF: ASSERTION FAILED in %s:%d: %s\n", __FILE__, __LINE__, msg); }
#define RMF_ASSERT(x)                { if (!(x))    { RMF_PRINT_ASSERT_ERR(#x);  RMF_ABORT(); } }
#define RMF_RETURN_UNLESS(cond, ret) { if (!(cond)) { RMF_PRINT_ASSERT_ERR(#cond); RMF_ABORT(); return ret; } }

class RMFMediaSourcePrivate : public IRMFMediaSource
{
public:
    RMFMediaSourcePrivate(RMFMediaSourceBase* parent);
    virtual ~RMFMediaSourcePrivate();

    // from IRMFMediaSource
    RMFResult init();
    RMFResult term();
    RMFResult open(const char* uri, char* mimetype);
    RMFResult close();
    RMFResult play();
    RMFResult play(float& speed, double& time);
    RMFResult pause();
    RMFResult stop();
    RMFResult frameSkip(int n);
    RMFResult getTrickPlayCaps(RMFTrickPlayCaps& caps);
    RMFResult getSpeed(float& speed);
    RMFResult setSpeed(float speed);
    RMFResult getMediaTime(double& t);
    RMFResult setMediaTime(double t);
    RMFResult getMediaInfo(RMFMediaInfo& info);
    RMFResult waitForEOS();
    RMFResult setEvents(IRMFMediaEvents* events);
    IRMFMediaEvents* getEvents();
    RMFResult addEventHandler(IRMFMediaEvents* events);
    RMFResult removeEventHandler(IRMFMediaEvents* events);
    std::vector<IRMFMediaEvents*> getEventHandler();
    RMFStateChangeReturn getState(RMFState* current, RMFState* pending);
    RMFMediaSourcePrivate* getPrivateSourceImpl() const { RMF_PRINT_ASSERT_ERR("This should never be reached!"); return NULL;}

    void setQueueSize(int queueSize);
    int getQueueSize();
    void setQueueLeaky( bool leaky);
    bool getQueueLeaky();
    GstElement* getTeeElement();
    RMFResult addSink(RMFMediaSinkBase* sink);
    RMFResult removeSink(RMFMediaSinkBase* sink);
    GstElement* getPipeline();
    GstElement* getSource();
    GstElement* getSink(const std::string& name);
    virtual RMFResult getCCIInfo( char& cciInfo);  // override to create a derived implementation
    virtual void sendCCIChangedEvent( char cciInfo );
    virtual RMFResult getRMFError(int error_code); // to convert events from gst elements or pipeline to RMFResult type	
    bool isLiveSource (void);
    RMFResult playAtLivePosition(double duration);
    RMFResult setVideoLength (double length);
    unsigned int getSinkCount(void);
    void notifyBusMessagingEnded(void);

    virtual void getPATBuffer(std::vector<uint8_t>& buf, uint32_t* length);
    virtual void getPMTBuffer(std::vector<uint8_t>& buf, uint32_t* length);
    virtual void getCATBuffer(std::vector<uint8_t>& buf, uint32_t* length);
    virtual bool getAudioPidFromPMT(uint32_t *pid, const std::string& currentLang);
    virtual void setFilter(uint16_t pid, char* filterParam, uint32_t *pFilterId);
    virtual void getSectionData(uint32_t *filterId, std::vector<uint8_t>& buf, uint32_t* length);
    virtual void releaseFilter(uint32_t filterId);
    virtual void resumeFilter(uint32_t filterId);
    virtual void pauseFilter(uint32_t filterId);

protected:
    RMFResult setState(GstState state);
    RMFResult flushPipeline (void);
    RMFResult notifySpeedChange(float new_speed);
    virtual void onError(RMFResult err, const char* msg);
    virtual void onCaStatus(const void* data_ptr, unsigned int data_size);
    virtual void onSection(const void* data_ptr, unsigned int data_size);
    virtual void onEOS();
    virtual void onPlaying();
    void onStatusChange(RMFStreamingStatus& status);

	virtual RMFResult handleElementMsg (GstMessage *msg);
    RMFResult updateSourcetoLive (bool);
private:
    void dumpQueueMap();
    static gboolean busCall(GstBus*, GstMessage* msg, gpointer data);
    static GstBusSyncReply syncHandler( GstBus *bus, GstMessage *msg, gpointer userData );
    bool handleBusMessage(GstMessage* msg);
    bool handleSinkMessage(GstMessage* msg);
    void onStateChange(GstState new_state);

private:
    typedef std::map<RMFMediaSinkBase*, std::string> queue_map_t;
    typedef std::map<GstElement*, RMFMediaSinkBase*> sink_map_t;
    rmf_osal_Cond m_bus_messaging_disabled;
    RMFMediaSourceBase* m_parent;
    int m_queueSize;
    bool m_leakyQueue;
    bool m_isLive;
    GstElement* m_pipeline;
    GstElement* m_source;
    GstElement* m_tee;
    queue_map_t m_queues;
    sink_map_t m_sinks;
    GMainLoop* m_loop;// Optional loop for the case the caller doesn't have one.
    IRMFMediaEvents* m_events;
    std::vector<IRMFMediaEvents*> m_event_handler;
    GStaticRecMutex m_mutex;
    guint m_bus_event_source_id;
};


class RMFMediaSinkPrivate : public IRMFMediaSink
{
public:
    RMFMediaSinkPrivate(RMFMediaSinkBase* parent);
    virtual ~RMFMediaSinkPrivate();

    // from IRMFMediaSink
    RMFResult init();
    RMFResult term();
    RMFResult setSource(IRMFMediaSource* source);
    RMFResult setEvents(IRMFMediaEvents* events);
    RMFResult addEventHandler(IRMFMediaEvents* events);
    RMFResult removeEventHandler(IRMFMediaEvents* events);
    void handleBusMessage(GstMessage* msg);
    RMFMediaSinkPrivate* getPrivateSinkImpl() const { RMF_PRINT_ASSERT_ERR("This should never be reached!"); return NULL;}

    GstElement* getSink() const;
    IRMFMediaSource* getSource() const {return m_source; }
    virtual void handleCCIChange(char cciInfo); //override to handle CCI data notification.
    virtual RMFResult getRMFError(int error_code);	// to convert events from gst elements or pipeline to RMFResult type

private:
    RMFMediaSinkBase* m_parent;
    IRMFMediaSource* m_source;
    IRMFMediaEvents* m_events;
    std::vector<IRMFMediaEvents*> m_event_handler;
    GStaticRecMutex m_mutex;
    GstElement* m_sink;
};

class RMFMediaFilterPrivate : public RMFMediaSourcePrivate, public RMFMediaSinkPrivate
{
public:
    RMFMediaFilterPrivate(RMFMediaFilterBase* parent);
//    virtual ~RMFMediaFilterPrivate();

    void* createElement();

    RMFResult init(bool enableSource = true);
    RMFResult term();

    RMFResult play();
    RMFResult play(float& speed, double& time);
    RMFResult getSpeed(float& speed);
    RMFResult setSpeed(float speed);
    RMFResult getMediaTime(double& time);
    RMFResult setMediaTime(double time);
    RMFResult getMediaInfo(RMFMediaInfo& info);

    RMFResult addSink(RMFMediaSinkBase* sink);
    RMFResult removeSink(RMFMediaSinkBase* sink);

    RMFResult setSource(IRMFMediaSource* source);
    bool handleSinkMessage(GstMessage* msg);

private:
    typedef std::map<RMFMediaSinkBase*, std::string> queue_map_t;
    typedef std::map<GstElement*, RMFMediaSinkBase*> sink_map_t;

    RMFMediaFilterBase* m_parent;
    GstElement *m_filter;
    GstElement *m_tee;
    queue_map_t m_queues;
    sink_map_t m_sinks;
    GStaticRecMutex m_mutex;
    GStaticRecMutex m_teeMutex;
};

#endif
