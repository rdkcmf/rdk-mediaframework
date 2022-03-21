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
#include "smoothsource.h"
#include "smoothvideoplayer.h"
#include "rmfprivate.h"
#include <gst/base/gstbasesrc.h>

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

#define USE_GSTREAMER_BIN 0

static void OnFakesrcHandoff(GstElement *pFakesrc, GstBuffer *pBuffer, GstPad *pPad, gpointer pUserData);

class SmoothEvents : public SmoothVideoEvents
{
public:
    SmoothEvents(IRMFMediaEvents* mediaEvents) : m_mediaEvents(mediaEvents)
    {
    }
	void metadata(int duration, int width, int height)
    {
        printf("SmoothVideoEvents %s\n", __FUNCTION__); 
    }
	void playing()
    {
        printf("SmoothVideoEvents %s\n", __FUNCTION__); 
        m_mediaEvents->playing();
    }
	void paused()
    {
        printf("SmoothVideoEvents %s\n", __FUNCTION__);
        m_mediaEvents->paused();
    }
	void stopped()
    {
        printf("SmoothVideoEvents %s\n", __FUNCTION__);
        m_mediaEvents->stopped();
    }
	void complete()
    {
        printf("SmoothVideoEvents %s\n", __FUNCTION__);
        m_mediaEvents->complete();
    }
	void progress(int position, int duration)
    {
        printf("SmoothVideoEvents %s\n", __FUNCTION__);
        m_mediaEvents->progress(position, duration);
    }
	void buffering(int seconds, float percent)
    { 
        printf("SmoothVideoEvents %s\n", __FUNCTION__); 
    }
	void status(const SmoothVideoStatus& status)
    {
        printf("SmoothVideoEvents %s\n", __FUNCTION__);
    }
	void dynamicTransitionComplete()
    {
        printf("SmoothVideoEvents %s\n", __FUNCTION__);
    }
	void connectionError(const char* err)
    {
        printf("SmoothVideoEvents %s %s\n", __FUNCTION__, err);
        RMFResult e;
        m_mediaEvents->error(e, "");
    }
	void playbackError(const char* err)
    {
        printf("SmoothVideoEvents %s %s\n", __FUNCTION__, err);
        RMFResult e;
        m_mediaEvents->error(e, "");
    }
private:
    IRMFMediaEvents* m_mediaEvents;
};

SmoothSource::SmoothSource() : m_player(0)
{
}

SmoothSource::~SmoothSource()
{
    term();
}

RMFResult SmoothSource::init()
{
    RMFMediaSourceBase::init();//this will call createElement

    createPlayer();
}

RMFResult SmoothSource::term()
{
    destroyPlayer();
}

RMFResult SmoothSource::open(const char* uri, char* mimetype)
{
    if(!m_player)
        init();

    if(m_player)
        m_player->open(uri);

    return RMF_RESULT_SUCCESS;
}

RMFResult SmoothSource::close()
{
    //if(m_player)
    //    m_player->close();

    return RMF_RESULT_SUCCESS;
}

RMFResult SmoothSource::play()
{
    //if(m_player)
    //    m_player->play();

    return RMF_RESULT_SUCCESS;
}

RMFResult SmoothSource::pause()
{
    //if(m_player)
    //    m_player->pause();

    return RMF_RESULT_SUCCESS;
}

RMFResult SmoothSource::getTrickPlayCaps(RMFTrickPlayCaps& caps)
{
    return RMF_RESULT_NOT_IMPLEMENTED;
}

RMFResult SmoothSource::setSpeed(float speed)
{
    return RMF_RESULT_NOT_IMPLEMENTED;
}

RMFResult SmoothSource::getMediaTime(double& t)
{
    return RMF_RESULT_NOT_IMPLEMENTED;
}

RMFResult SmoothSource::setMediaTime(double t)
{
    (void) t;
    return RMF_RESULT_NOT_IMPLEMENTED;
}

RMFResult SmoothSource::getMediaInfo(RMFMediaInfo& info)
{
    (void) info;
    return RMF_RESULT_NOT_IMPLEMENTED;
}

RMFResult SmoothSource::waitForEOS()
{
    return RMF_RESULT_NOT_IMPLEMENTED;
}

/*
    createElement()
    called from inside RMFMediaSourceBase::init so that our source gstreamer binary can be added to the pipeline
    I copied the code that creates the fakesrc from inside Smooth to here
    I could have left that code inside Smooth and just add a method such as createElement on the Smooth player and let smooth continue to do this internally and just return me the element to use
    But I pulled it into here for illustrative purposes
*/
void* SmoothSource::createElement()
{
    GstElement* pFakeSrc;
    //Create fakesrc
    pFakeSrc = gst_element_factory_make("fakesrc","myfakesrc");
    g_object_set(G_OBJECT(pFakeSrc), "signal-handoffs", TRUE, NULL);
    g_object_set(G_OBJECT(pFakeSrc), "sizetype", 1, NULL);
    g_object_set(G_OBJECT(pFakeSrc), "silent", TRUE, NULL);//MARKR - this works around a random seg fault occuring inside gst_fake_src_create when it calls g_object_notify to send the message
    //RMF - move gst_version to rmfbase so all source/sinks can know the version
    static guint s_uiMajorVersion = 0, s_uiMinorVersion = 0, s_uiMicroVersion = 0, s_uiNanoVersion = 0;
    gst_version (&s_uiMajorVersion, &s_uiMinorVersion, &s_uiMicroVersion, &s_uiNanoVersion);
    if(s_uiMinorVersion > 10 || (s_uiMinorVersion == 10 && s_uiMicroVersion >= 20))
        g_object_set(G_OBJECT(pFakeSrc), "format", GST_FORMAT_TIME, NULL);
    g_signal_connect(pFakeSrc, "handoff", G_CALLBACK(OnFakesrcHandoff), m_player);
    gst_base_src_set_format(GST_BASE_SRC(pFakeSrc), GST_FORMAT_TIME);

#if USE_GSTREAMER_BIN
    GstElement* pBin  = gst_bin_new("h264bin");
    gst_bin_add_many(GST_BIN(pBin), pFakeSrc, NULL);
    gst_element_link_many(pFakeSrc, NULL);
    return pBin;
#else
    return pFakeSrc;
#endif
}

void SmoothSource::createPlayer()
{
    destroyPlayer();

    g_print("creating smooth video player\n");
    createSmoothVideoPlayer(&m_player);
    g_print("done creating smooth video player\n");

    m_smoothEvents = new SmoothEvents(getPrivateSourceImpl()->getEvents());

    g_print("setting smooth pipeline\n");
    if(m_player)
        m_player->rmfSetPipeline(getPrivateSourceImpl()->getPipeline());
    g_print("done setting smooth pipeline\n");
}

void SmoothSource::destroyPlayer()
{
    if(m_player)
    {
        destroySmoothVideoPlayer(m_player);
        m_player = 0;
    }

    if(m_smoothEvents)
    {
        delete m_smoothEvents;
        m_smoothEvents = 0;
    }
}

static void OnFakesrcHandoff(
    GstElement *pFakesrc,
    GstBuffer *pBuffer,
    GstPad *pPad,
    gpointer pUserData)
{
    ((SmoothVideoPlayer*)pUserData)->rmfGetNextBuffer(pBuffer);
}
