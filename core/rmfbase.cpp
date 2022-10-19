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

#include "rmfbase.h"
#include "rmfprivate.h"
#include <cassert>
#include <gst/gst.h>
#include <limits>
#include <list>
#include <map>
#include <string>
#include <string.h>
#include <unistd.h>

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

#define MAX_GST_ELEMENTS 20

//  Remove application-specific code from RMF core!
#ifdef RMF_STREAMER
#include "rmf_osal_resreg.h"

#include "config.h"

void RMF_GstLogFunction(GstDebugCategory *category,
                        GstDebugLevel level,
                        const gchar *file,
                        const gchar *function,
                        gint line,
                        GObject *object,
                        GstDebugMessage *message,
                        gpointer user_data) G_GNUC_NO_INSTRUMENT; 

typedef struct __gstCategoryMap_s
{
	char gst_module[128];
	char rmf_module[128];
}gstCategoryMap_s;

gstCategoryMap_s g_gstRegisteredElements[MAX_GST_ELEMENTS];
int g_noOfRegisteredElements = 0;

void RMF_registerGstElementDbgModule(char *gst_module,  char *rmf_module)
{
	int ix=0;
	for( ix=0; ix<g_noOfRegisteredElements; ix++ )
	{
		if( 0 == strcmp(g_gstRegisteredElements[ix].gst_module, gst_module) )
			return;
	}
	memset( g_gstRegisteredElements[g_noOfRegisteredElements].gst_module, '\0', sizeof(g_gstRegisteredElements[g_noOfRegisteredElements].gst_module) );
	memset( g_gstRegisteredElements[g_noOfRegisteredElements].rmf_module, '\0', sizeof(g_gstRegisteredElements[g_noOfRegisteredElements].rmf_module) );
	strncpy(g_gstRegisteredElements[g_noOfRegisteredElements].gst_module,gst_module,sizeof(g_gstRegisteredElements[g_noOfRegisteredElements].gst_module)-1 );
	strncpy(g_gstRegisteredElements[g_noOfRegisteredElements].rmf_module,rmf_module,sizeof(g_gstRegisteredElements[g_noOfRegisteredElements].rmf_module)-1 );
	g_noOfRegisteredElements++;
}

void RMF_GstLogFunction(GstDebugCategory *category,
                        GstDebugLevel level,
                        const gchar *file,
                        const gchar *function,
                        gint line,
                        GObject *object,
                        GstDebugMessage *message,
                        gpointer user_data)
{
	int ix;
	rdk_LogLevel rmf_level;
	for( ix=0; ix<g_noOfRegisteredElements; ix++ )
	{
		if( 0 == strcmp(g_gstRegisteredElements[ix].gst_module, gst_debug_category_get_name(category)) )
		{
			char rmf_dbg_message[1024];
			switch (level )
			{
				case GST_LEVEL_ERROR:
					rmf_level = RDK_LOG_ERROR;
					break;
				case GST_LEVEL_WARNING:
					rmf_level = RDK_LOG_WARN;
					break;
				case GST_LEVEL_FIXME:
					rmf_level = RDK_LOG_NOTICE;
					break;
				case GST_LEVEL_INFO:
					rmf_level = RDK_LOG_INFO;
					break;
				case GST_LEVEL_DEBUG:
					rmf_level = RDK_LOG_DEBUG;
					break;
				case GST_LEVEL_TRACE:
					rmf_level = RDK_LOG_TRACE1;
					break;
				default:
					rmf_level = RDK_LOG_TRACE1;
					break;
			}
			snprintf(rmf_dbg_message, 1024, "%s[%d]-%s :: %s\n", file, line, function, gst_debug_message_get(message));
			rmf_dbg_message[1023]=0;
			RDK_LOG ( (rdk_LogLevel)rmf_level,  g_gstRegisteredElements[ix].rmf_module, rmf_dbg_message );	
			break;
		}	
	}
}

#include "rmf_osal_util.h"
#include "rmf_osal_thread.h"
#else
#define RDK_LOG(level, module, format...) g_print(format)
#endif

void RMFInit()
{
    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "%s:%d - Enter.\n", __FUNCTION__, __LINE__);
    if (!gst_is_initialized())
    {
//  Remove application-specific code from RMF core!
#ifdef RMF_STREAMER
	g_noOfRegisteredElements = 0;
#endif
        char* argv[] = {(char*)"RMF", 0};
        char** gst_argv = argv;
        int argc = 1;
	int rmf_log_level = 6;
	char *env_log_value = NULL;
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "%s:%d Profiling.\n", __FUNCTION__, __LINE__); //Added to track large delays in RMFInit().
        gst_init(&argc,&gst_argv);
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "%s:%d Profiling.\n", __FUNCTION__, __LINE__);
#ifdef RMF_STREAMER
	gst_debug_remove_log_function(gst_debug_log_default);
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "%s:%d Profiling.\n", __FUNCTION__, __LINE__);
#ifdef USE_GST1
	gst_debug_add_log_function(RMF_GstLogFunction, NULL, NULL);
#else
	gst_debug_add_log_function(RMF_GstLogFunction, NULL);
#endif
	/*Enable GST_LEVEL ERROR, WARNING, DEBUG, INFO, LOG */
    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "%s:%d Profiling.\n", __FUNCTION__, __LINE__);
        env_log_value = getenv("RMF_GST_LOG_LEVEL");
	if(env_log_value){
		rmf_log_level = atoi(env_log_value);
	}
	gst_debug_set_default_threshold((GstDebugLevel)rmf_log_level);
#endif
/** setting new variable because gst_is_initialized( is not defined in broadcom platform
  */
    }
}

#define RETURN_UNLESS(x) RMF_RETURN_UNLESS(x, RMF_RESULT_INTERNAL_ERROR)
#define RETURN_IF_NO_IMPL(ret) RMF_RETURN_UNLESS(m_pimpl, ret)
#define CHECK_INIT() RETURN_IF_NO_IMPL(RMF_RESULT_NOT_INITIALIZED)
#define CHECK_IMPL() RETURN_IF_NO_IMPL(NULL)

#ifdef ENABLE_RMF_TRACE
#   define RMF_TRACE(format...) RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", format)
#else
#   define RMF_TRACE(...) {}
#endif

//-----------------------------------------------------------------------------
//-- RMFMediaSinkBase ---------------------------------------------------------
//-----------------------------------------------------------------------------

RMFMediaSinkBase::RMFMediaSinkBase()
:   m_pimpl(0)
{
}

RMFMediaSinkBase::~RMFMediaSinkBase()
{
    if ( m_pimpl )
		delete m_pimpl;
	m_pimpl = 0;
}

// virtual
RMFResult RMFMediaSinkBase::init()
{
    if (!m_pimpl) {
      m_pimpl = createPrivateSinkImpl();
    }
    return m_pimpl->init();
}

// virtual
RMFResult RMFMediaSinkBase::term()
{
    CHECK_INIT();
    return m_pimpl->term();
}

// virtual
RMFResult RMFMediaSinkBase::setSource(IRMFMediaSource* source)
{
    CHECK_INIT();
    return m_pimpl->setSource(source);
}

// virtual
RMFResult RMFMediaSinkBase::setEvents(IRMFMediaEvents* events)
{
    CHECK_INIT();
    return m_pimpl->setEvents(events);
}

// virtual
RMFResult RMFMediaSinkBase::addEventHandler(IRMFMediaEvents* events)
{
    RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSinkBase::addEventHandler :: %p\n", (uintptr_t)events);
    CHECK_INIT();
    return m_pimpl->addEventHandler(events);
}

// virtual
RMFResult RMFMediaSinkBase::removeEventHandler(IRMFMediaEvents* events)
{
    RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSinkBase::removeEventHandler :: %p\n", (uintptr_t)events);
    CHECK_INIT();
    return m_pimpl->removeEventHandler(events);
}

// virtual
RMFStateChangeReturn RMFMediaSourceBase::getState(RMFState* current, RMFState* pending)
{
    RETURN_IF_NO_IMPL(RMF_STATE_CHANGE_FAILURE);
    return m_pimpl->getState(current, pending);
}


RMFMediaSinkPrivate* RMFMediaSinkBase::getPrivateSinkImpl() const
{
    CHECK_IMPL(); // abort if not initialized
    return m_pimpl;
}

RMFMediaSinkPrivate* RMFMediaSinkBase::getPrivateSinkImplUnchecked() const
{
    return m_pimpl;
}

// virtual
RMFMediaSinkPrivate* RMFMediaSinkBase::createPrivateSinkImpl()
{
    return new RMFMediaSinkPrivate(this);
}

//-----------------------------------------------------------------------------
//-- RMFMediaSinkPrivate ------------------------------------------------------
//-----------------------------------------------------------------------------

RMFMediaSinkPrivate::RMFMediaSinkPrivate(RMFMediaSinkBase* parent)
:   m_parent(parent)
,   m_source(NULL)
,   m_events(NULL)
,   m_sink(NULL)
{
    g_static_rec_mutex_init (&m_mutex);
}

RMFMediaSinkPrivate::~RMFMediaSinkPrivate()
{
    if (m_sink)
    {
        int __refcnt=GST_OBJECT_REFCOUNT_VALUE(m_sink);
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "~RMFMediaSinkPrivate: ref_count: %d\n", __refcnt );
        gst_object_unref(GST_OBJECT(m_sink));
    }
    g_static_rec_mutex_free (&m_mutex);
}

// virtual
RMFResult RMFMediaSinkPrivate::init()
{
    if (m_sink)
    {
        return RMF_RESULT_SUCCESS; // already initialized
    }

    RMFInit();

    //g_static_rec_mutex_init (&m_mutex);

    if (!(m_sink = (GstElement*)m_parent->createElement()))
        return RMF_RESULT_INTERNAL_ERROR;

    gst_object_ref_sink(m_sink); // take ownership
    return RMF_RESULT_SUCCESS;
}

// virtual
RMFResult RMFMediaSinkPrivate::term()
{

    RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSinkPrivate::term :: Entry\n");
    RMFResult result= setSource(NULL);
    
    if (gst_element_set_state(m_sink, GST_STATE_NULL) == GST_STATE_CHANGE_ASYNC)
    {
        if ((result = gst_element_get_state(m_sink, NULL, NULL, GST_CLOCK_TIME_NONE)) != GST_STATE_CHANGE_SUCCESS)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.RMFBASE", "RMFMediaSinkPrivate::term :: FAILED to change the state. ret = %d\n", result);
        }
    }

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSinkPrivate::term :: Mutex Free\n");
    //g_static_rec_mutex_free (&m_mutex);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSinkPrivate::term :: Exit\n");

    return result;
}

// virtual
RMFResult RMFMediaSinkPrivate::setSource(IRMFMediaSource* source)
{
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSinkPrivate::setSource :: Entry\n");
    if (!m_sink)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.RMFBASE", "RMFMediaSinkPrivate::setSource: not initialized\n");
        return RMF_RESULT_NOT_INITIALIZED;
    }

    if (m_source)
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSinkPrivate::setSource :: Calling removeSink\n");
        RMFResult res = ((RMFMediaSourceBase*)m_source)->removeSink(m_parent);
	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSinkPrivate::setSource :: Out of removeSink\n");
        m_source = NULL;
        if (res != RMF_RESULT_SUCCESS)
            return RMF_RESULT_INTERNAL_ERROR;
    }

    if (source)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.RMFBASE", "RMFMediaSinkPrivate::setSource :: Calling addSink\n");
        RMFResult res =  ((RMFMediaSourceBase*)source)->addSink(m_parent);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.RMFBASE", "RMFMediaSinkPrivate::setSource :: Out of addSink\n");
        if (res != RMF_RESULT_SUCCESS)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.RMFBASE", "RMFMediaSinkPrivate::setSource: failed to connect to source %p\n", source);
            return res;
        }
        m_source = source;
    }
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSinkPrivate::setSource :: Exit\n");

    return RMF_RESULT_SUCCESS;
}

// virtual
RMFResult RMFMediaSinkPrivate::setEvents(IRMFMediaEvents* events)
{ 
    m_events = events;    
    return RMF_RESULT_SUCCESS;
}

// virtual ADDED
RMFResult RMFMediaSinkPrivate::addEventHandler(IRMFMediaEvents* events)
{
    g_static_rec_mutex_lock( &m_mutex);
    m_event_handler.insert(m_event_handler.begin(), events);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.RMFBASE", "RMFMediaSinkPrivate::addEventHandler :: %p :: size %d\n", (uintptr_t)events, m_event_handler.size());
    g_static_rec_mutex_unlock( &m_mutex);

    return RMF_RESULT_SUCCESS;
}

// virtual ADDED
RMFResult RMFMediaSinkPrivate::removeEventHandler(IRMFMediaEvents* events)
{
    g_static_rec_mutex_lock( &m_mutex);
    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "RMFMediaSinkPrivate::beginning removeEventHandler :: %p :: size %d\n", (uintptr_t)events, m_event_handler.size());
    for ( std::vector<IRMFMediaEvents*>::iterator it= m_event_handler.begin(); it != m_event_handler.end(); ++it )
    {
        if (*it == events)
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSinkPrivate::found event - deleting it \n");
            m_event_handler.erase(it);
            break;
        }
    }
    RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSinkPrivate::removeEventHandler :: %p :: size %d\n", (uintptr_t)events, m_event_handler.size());
    g_static_rec_mutex_unlock( &m_mutex);

    return RMF_RESULT_SUCCESS;
}

void RMFMediaSinkPrivate::handleBusMessage(GstMessage* msg)
{
     if ( m_events )
     {
        switch (GST_MESSAGE_TYPE(msg))
        {
            case GST_MESSAGE_ERROR:
            {
                GError *error;
                gst_message_parse_error(msg, &error, NULL);
                char* msgError = g_strdup_printf("Error: %s", error->message);
                m_events->error( getRMFError(error->code), msgError);
                g_error_free(error);
                g_free(msgError);
                break;
            }
            default:
                break;
        }
     }

    g_static_rec_mutex_lock( &m_mutex);
    for( std::vector<IRMFMediaEvents*>::iterator it= m_event_handler.begin(); it != m_event_handler.end(); ++it )
    {
        if(*it)
        {
            switch (GST_MESSAGE_TYPE(msg))
            {
                case GST_MESSAGE_ERROR:
                {
                    GError *error;
                    gst_message_parse_error(msg, &error, NULL);
                    char* msgError = g_strdup_printf("Error: %s", error->message);
                    (*it)->error( getRMFError(error->code), msgError);
                    g_error_free(error);
                    g_free(msgError);
                    break;
                }
                default:
                    break;
            }
        }
    }
    g_static_rec_mutex_unlock( &m_mutex);
}

GstElement* RMFMediaSinkPrivate::getSink() const
{
    return m_sink;
}

void RMFMediaSinkPrivate::handleCCIChange(char cciInfo)
{
    (void) cciInfo;
}


// virtual
RMFResult RMFMediaSinkPrivate::getRMFError(int error_code)
{
    RMFResult error_type = RMF_RESULT_FAILURE;
    return error_type;
}

//-----------------------------------------------------------------------------
//-- RMFMediaSourceBase -------------------------------------------------------
//-----------------------------------------------------------------------------

RMFMediaSourceBase::RMFMediaSourceBase()
:   m_pimpl(0)
{
}

RMFMediaSourceBase::~RMFMediaSourceBase()
{
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSourceBase::~RMFMediaSourceBase() [SOURCE] m_pimpl=%p\n", (void*)m_pimpl);
    if (m_pimpl)
       delete m_pimpl;
    m_pimpl = 0;
}

// virtual
RMFResult RMFMediaSourceBase::init()
{
    m_pimpl = createPrivateSourceImpl();
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSourceBase::init() [SOURCE] m_pimpl=%p\n", (void*)m_pimpl);
    return m_pimpl->init();
}

// virtual
RMFResult RMFMediaSourceBase::term()
{
    CHECK_INIT();
    return m_pimpl->term();
}

// virtual
RMFResult RMFMediaSourceBase::open(const char* uri, char* mimetype)
{
    CHECK_INIT();
    return m_pimpl->open(uri,mimetype);
}

// virtual
RMFResult RMFMediaSourceBase::close()
{
    CHECK_INIT();
    return m_pimpl->close();
}

// virtual
RMFResult RMFMediaSourceBase::play()
{
    CHECK_INIT();
    return m_pimpl->play();
}

RMFResult RMFMediaSourceBase::play(float& speed, double& time)
{
    CHECK_INIT();
    return m_pimpl->play(speed, time);
}

// virtual
RMFResult RMFMediaSourceBase::pause()
{
    CHECK_INIT();
    return m_pimpl->pause();
}

// virtual
RMFResult RMFMediaSourceBase::stop()
{
    CHECK_INIT();
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.RMFBASE", "RMFMediaSourceBase::stop() :: Calling m_pimpl->stop()\n");
    return m_pimpl->stop();
}

// virtual
RMFResult RMFMediaSourceBase::frameSkip(int n)
{
    CHECK_INIT();
    return m_pimpl->frameSkip(n);
}

// virtual
RMFResult RMFMediaSourceBase::getTrickPlayCaps(RMFTrickPlayCaps& caps)
{
    CHECK_INIT();
    return m_pimpl->getTrickPlayCaps(caps);
}

// virtual
RMFResult RMFMediaSourceBase::getSpeed(float& speed)
{
    CHECK_INIT();
    return m_pimpl->getSpeed(speed);
}

// virtual
RMFResult RMFMediaSourceBase::setSpeed(float speed)
{
    CHECK_INIT();
    return m_pimpl->setSpeed(speed);
}

// virtual
RMFResult RMFMediaSourceBase::getMediaTime(double& t)
{
    CHECK_INIT();
    return m_pimpl->getMediaTime(t);
}

// virtual
RMFResult RMFMediaSourceBase::setMediaTime(double t)
{
    CHECK_INIT();
    return m_pimpl->setMediaTime(t);
}

// virtual
RMFResult RMFMediaSourceBase::getMediaInfo(RMFMediaInfo& info)
{
    CHECK_INIT();
    return m_pimpl->getMediaInfo(info);
}

// virtual
RMFResult RMFMediaSourceBase::waitForEOS()
{
    CHECK_INIT();
    return m_pimpl->waitForEOS();
}

RMFResult RMFMediaSourceBase::setEvents(IRMFMediaEvents* events)
{
    CHECK_INIT();
    return m_pimpl->setEvents(events);
}

RMFResult RMFMediaSourceBase::addEventHandler(IRMFMediaEvents* events)
{
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSourceBase::addEventHandler :: %p\n", (uintptr_t)events);
    CHECK_INIT();
    return m_pimpl->addEventHandler(events);
}

RMFResult RMFMediaSourceBase::removeEventHandler(IRMFMediaEvents* events)
{
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSourceBase::removeEventHandler :: %p\n", (uintptr_t)events);
    CHECK_INIT();
    return m_pimpl->removeEventHandler(events);
}

// virtual
RMFMediaSourcePrivate* RMFMediaSourceBase::getPrivateSourceImpl() const
{
    CHECK_IMPL(); // abort if not initialized
    return m_pimpl;
}

RMFResult RMFMediaSourceBase::addSink(RMFMediaSinkBase* sink)
{
    CHECK_INIT();
    return m_pimpl->addSink(sink);
}

RMFResult RMFMediaSourceBase::removeSink(RMFMediaSinkBase* sink)
{
    CHECK_INIT();
    return m_pimpl->removeSink(sink);
}

// virtual
RMFMediaSourcePrivate* RMFMediaSourceBase::createPrivateSourceImpl()
{
    return new RMFMediaSourcePrivate(this);
}


//-----------------------------------------------------------------------------
//-- RMFMediaSourcePrivate ----------------------------------------------------
//-----------------------------------------------------------------------------

void busShutdownCallback(gpointer object)
{
	((RMFMediaSourcePrivate *)object)->notifyBusMessagingEnded();
}


RMFMediaSourcePrivate::RMFMediaSourcePrivate(RMFMediaSourceBase* parent)
:   m_parent(parent)
,   m_queueSize(0)
,   m_leakyQueue(false)
,   m_isLive(false)
,   m_pipeline(NULL)
,   m_source(NULL)
,   m_tee(NULL)
,   m_loop(NULL)
,   m_events(NULL)
,   m_bus_event_source_id(0)
{
}

RMFMediaSourcePrivate::~RMFMediaSourcePrivate()
{
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::~RMFMediaSourcePrivate\n");
    if (m_pipeline)
    {
        term();
    }

    assert(!m_pipeline); // must have been shut down already
    assert(m_queues.size() == 0);
}

// virtual
RMFResult RMFMediaSourcePrivate::init()
{
    if (m_pipeline) return RMF_RESULT_SUCCESS; // already initialized

    RMF_TRACE("RMFMediaSourcePrivate::init enter\n");

    RMFInit();

    g_static_rec_mutex_init (&m_mutex);

    RMF_TRACE("creating gstreamer pipeline\n");
    m_pipeline = gst_pipeline_new("pipeline");
    RMF_TRACE("done creating gstreamer pipeline\n");

    RMF_TRACE("creating source element\n");
    m_source = (GstElement*)m_parent->createElement();
    RMF_TRACE("done creating source element\n");

    RMF_TRACE("creating tee element\n");
    m_tee = gst_element_factory_make("tee", "source_tee");
    RMF_TRACE("done creating tee element\n");

    if (!m_pipeline || !m_source || !m_tee)
    {
        RDK_LOG(RDK_LOG_FATAL, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate:init m_pipeline=%p, m_source=%p, m_tee=%p\n", m_pipeline, m_source, m_tee);
        return RMF_RESULT_FAILURE;
    }

    RMF_TRACE("adding source and tee to pipeline\n");
    gst_bin_add_many(GST_BIN(m_pipeline), m_source, m_tee, NULL);
    RMF_TRACE("done adding source and tee to pipeline\n");

    RMF_TRACE("linking source and tee\n");
    if (!gst_element_link(m_source, m_tee))
    {
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate: failed to link source and tee\n");
        return RMF_RESULT_INTERNAL_ERROR;
    }
    RMF_TRACE("done linking source and tee\n");

    if (RMF_SUCCESS != rmf_osal_condNew( FALSE, FALSE, &m_bus_messaging_disabled))
    {
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.RMFBASE", "%s - rmf_osal_condNew failed.\n", __FUNCTION__);
        return RMF_RESULT_INTERNAL_ERROR;
    }

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(m_pipeline));

#ifdef USE_GST1
    m_bus_event_source_id = gst_bus_add_watch_full(bus, G_PRIORITY_HIGH, busCall, this, busShutdownCallback);
#else
    m_bus_event_source_id = gst_bus_add_watch_full(bus, G_PRIORITY_DEFAULT, busCall, this, busShutdownCallback);
#endif

#ifdef RMF_STREAMER
#ifdef USE_GST1
    gst_bus_set_sync_handler(bus, syncHandler, NULL, NULL);
#else
    gst_bus_set_sync_handler(bus, syncHandler, NULL);
#endif
#endif
    gst_object_unref(bus);

    RMF_TRACE("RMFMediaSourcePrivate::init exit\n");
    return RMF_RESULT_SUCCESS;
}

// virtual
RMFResult RMFMediaSourcePrivate::term()
{
	GstState	gst_current;
	GstState	gst_pending;
	float		nanoTimeOut = 50000000.0; //50ms
	gint		gstGetStateCnt = 5;
	guint		msTimeOut = 50;

	if (!m_pipeline)
	{
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::term Entry\n");
		return RMF_RESULT_NOT_INITIALIZED;
	}
	g_print( "CHANNEL CHANGE - RMFMediaSourcePrivate::term Calling  gst_element_get_state setState NULL to Pipeline\n");
	setState (GST_STATE_NULL);
	do
	{
		if ((GST_STATE_CHANGE_SUCCESS ==
			gst_element_get_state(m_pipeline, &gst_current,
						&gst_pending, nanoTimeOut))
						&& (gst_current == GST_STATE_NULL))
		{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE","Pipeline gst_element_get_state success"
						": State = %d, Pending = %d\n", gst_current, gst_pending );
			break;
		}
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.RMFBASE", "Waiting for state change completion\n" );
		usleep (msTimeOut*1000); // Let pipeline safely transition to required state
		gstGetStateCnt--;
	} while ((gst_current != GST_STATE_NULL) && (gstGetStateCnt != 0));

	if (!gstGetStateCnt)
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "CRITICAL Error: Abnormal Condition:"
				" Terminating pipeline without waiting for NULL state\n" );

	g_print("CHANNEL CHANGE - RMFMediaSourcePrivate::term Returned gst_element_get_state NULL from Pipeline Count = %d\n", gstGetStateCnt);

	RMFResult res = RMF_RESULT_SUCCESS;

	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::term Requesting Mutex Lock\n");
	g_static_rec_mutex_lock( &m_mutex);
	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::term Obtained Mutex Lock\n");
	for (queue_map_t::iterator it = m_queues.begin(); it != m_queues.end(); /*nop*/)
	{
		queue_map_t::iterator cur_it = it++;
		RMFMediaSinkBase* sink = cur_it->first;

		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::term Calling setSource\n");
		res = sink->setSource(NULL); // safer than removeSink() b/c it resets the source pointer in the sink
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::term Out of setSource\n");

		if (res != RMF_RESULT_SUCCESS)
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate: failed to remove sink\n");
	}

	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::term Releasing Mutex Lock\n");
	g_static_rec_mutex_unlock( &m_mutex);
	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::term Released Mutex Lock\n");

	if (m_pipeline)
	{
#ifdef RMF_STREAMER
		GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(m_pipeline));
#ifdef USE_GST1
		gst_bus_set_sync_handler( bus, NULL, NULL, NULL);
#else
		gst_bus_set_sync_handler( bus, NULL, NULL );
#endif
		gst_object_unref(bus);
#endif

		int __refcnt=GST_OBJECT_REFCOUNT_VALUE(m_pipeline);
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.RMFBASE", "m_pipeline ref_count=%d bus=%p m_bus_event_source_id=%d\n",
			__refcnt, bus, m_bus_event_source_id);
		gst_object_unref(GST_OBJECT(m_pipeline));
		m_pipeline = NULL;

		if ( 0 != m_bus_event_source_id )
		{
			if(g_source_remove(m_bus_event_source_id))
			{
				rmf_osal_condWaitFor(m_bus_messaging_disabled, 0);
			}
			else
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.RMFBASE", "g_source_remove() returned failure!\n");
			}
			m_bus_event_source_id = 0;
		}
	}

	RMF_ASSERT(m_queues.size() == 0);

	rmf_osal_condDelete(m_bus_messaging_disabled);

	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::term Mutex Free\n");
	g_static_rec_mutex_free (&m_mutex);

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::term Exit\n");

	return res;
}

// virtual
RMFResult RMFMediaSourcePrivate::open(const char* uri, char* mimetype)
{
    (void) mimetype;
    (void) uri;

    // Override to do something more useful.
    return init();
}

// virtual
RMFResult RMFMediaSourcePrivate::close()
{
    return setState(GST_STATE_NULL);
}

// virtual
RMFResult RMFMediaSourcePrivate::play()
{
    if (m_queues.size() == 0)
        return RMF_RESULT_NO_SINK;

    return setState(GST_STATE_PLAYING);
}

RMFResult RMFMediaSourcePrivate::play(float& speed, double& time)
{
    // Handle speed and time in subclass
    (void) speed;
    (void) time;
    
    return play();
}

// virtual
RMFResult RMFMediaSourcePrivate::pause()
{
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::pause() :: setState(GST_STATE_PAUSED)\n");
    return setState(GST_STATE_PAUSED);
}

// virtual
RMFResult RMFMediaSourcePrivate::stop()
{
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::stop() :: setState(GST_STATE_READY)\n");
    return setState(GST_STATE_READY);
}

RMFResult RMFMediaSourcePrivate::frameSkip(int n)
{
    gint64 position = 0;
#ifdef USE_GST1
    if (!gst_element_query_position (m_pipeline, GST_FORMAT_TIME, &position))
#else
    GstFormat format = GST_FORMAT_TIME;
    if (!gst_element_query_position (m_pipeline, &format, &position))
#endif
    {
        g_printerr ("Unable to retrieve current position.\n");
        return RMF_RESULT_INTERNAL_ERROR;
    }

    double rate = n < 0 ? -1.0 : 1.0;
    n = n < 0 ? -n : n;
    GstEvent* seek_event;
    if (rate > 0.0)
    {
      seek_event = gst_event_new_seek (rate, GST_FORMAT_TIME, GstSeekFlags(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE),
          GST_SEEK_TYPE_SET, position, GST_SEEK_TYPE_NONE, 0);
    }
    else
    {
      seek_event = gst_event_new_seek (rate, GST_FORMAT_TIME, GstSeekFlags(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE),
          GST_SEEK_TYPE_SET, 0, GST_SEEK_TYPE_SET, position);
    }

    for(sink_map_t::iterator it = m_sinks.begin(); it != m_sinks.end(); it++)
    {
        gst_element_send_event (it->first, seek_event);
    }
    for(sink_map_t::iterator it = m_sinks.begin(); it != m_sinks.end(); it++)
    {
        gst_element_send_event (it->first,
              gst_event_new_step (GST_FORMAT_BUFFERS, n, rate, TRUE, FALSE));
    }

    return RMF_RESULT_SUCCESS;
}

// virtual
RMFResult RMFMediaSourcePrivate::getTrickPlayCaps(RMFTrickPlayCaps& caps)
{
    (void) caps;
    return RMF_RESULT_NOT_IMPLEMENTED;
}

// virtual
RMFResult RMFMediaSourcePrivate::getSpeed(float& speed)
{
    (void) speed;
    return RMF_RESULT_NOT_IMPLEMENTED;
}

// virtual
RMFResult RMFMediaSourcePrivate::setSpeed(float speed)
{
    (void) speed;
    return RMF_RESULT_NOT_IMPLEMENTED;
}

// virtual
RMFResult RMFMediaSourcePrivate::getMediaTime(double& t)
{
    (void) t;
    return RMF_RESULT_NOT_IMPLEMENTED;
}

// virtual
RMFResult RMFMediaSourcePrivate::setMediaTime(double t)
{
    (void) t;
    return RMF_RESULT_NOT_IMPLEMENTED;
}

// virtual
RMFResult RMFMediaSourcePrivate::getMediaInfo(RMFMediaInfo& info)
{
    if (!m_pipeline)
        return 0.0f;

    // Get duration.
    gint64 time_length = 0;
#ifdef USE_GST1
    if (!gst_element_query_duration(m_pipeline, GST_FORMAT_TIME, &time_length) ||
        static_cast<guint64>(time_length) == GST_CLOCK_TIME_NONE)
#else
    GstFormat time_format = GST_FORMAT_TIME;
    if (!gst_element_query_duration(m_pipeline, &time_format, &time_length) ||
        time_format != GST_FORMAT_TIME ||
        static_cast<guint64>(time_length) == GST_CLOCK_TIME_NONE)
#endif
    {
        //RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "Duration query failed.\n");
        return std::numeric_limits<float>::infinity();
    }
    //RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "Duration: %" GST_TIME_FORMAT "\n", GST_TIME_ARGS(time_length));
    info.m_duration = ((guint64) time_length / 1000000000.0);

    return RMF_RESULT_SUCCESS;
}

// virtual
RMFResult RMFMediaSourcePrivate::waitForEOS()
{
    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "Main loop started\n");
    m_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(m_loop);
    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "Main loop finished\n");
    g_main_loop_unref(m_loop);
    m_loop = NULL;
    return RMF_RESULT_SUCCESS;
}

RMFResult RMFMediaSourcePrivate::setEvents(IRMFMediaEvents* events)
{
    m_events = events;
    return RMF_RESULT_SUCCESS;
}

//ADDED
RMFResult RMFMediaSourcePrivate::addEventHandler(IRMFMediaEvents* events)
{
    g_static_rec_mutex_lock( &m_mutex);
    m_event_handler.insert(m_event_handler.begin(), events);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::addEventHandler :: %p :: Size : %d\n", (uintptr_t)events, m_event_handler.size());
    g_static_rec_mutex_unlock( &m_mutex);
    return RMF_RESULT_SUCCESS;
}

//ADDED
RMFResult RMFMediaSourcePrivate::removeEventHandler(IRMFMediaEvents* events)
{
    g_static_rec_mutex_lock( &m_mutex);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::removeEventHandler :: %p :: Size : %d\n", (uintptr_t)events, m_event_handler.size());
    for (std::vector<IRMFMediaEvents*>::iterator it= m_event_handler.begin(); it != m_event_handler.end(); ++it)
    {
        if (*it == events)
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::found event - deleting it \n");
            m_event_handler.erase(it);
            break;
        }
    }
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::removeEventHandler :: %p :: Size : %d\n", (uintptr_t)events, m_event_handler.size());
    g_static_rec_mutex_unlock( &m_mutex);

    return RMF_RESULT_SUCCESS;
}

RMFResult RMFMediaSourcePrivate::getCCIInfo( char& cciInfo)
{
    (void) cciInfo;
    return RMF_RESULT_NOT_IMPLEMENTED;
}

void RMFMediaSourcePrivate::getPATBuffer(std::vector<uint8_t>& buf, uint32_t* length)
{
    (void) buf;
    *length = 0;
}

void RMFMediaSourcePrivate::getPMTBuffer(std::vector<uint8_t>& buf, uint32_t* length)
{
    (void) buf;
    *length = 0;
}

void RMFMediaSourcePrivate::getCATBuffer(std::vector<uint8_t>& buf, uint32_t* length)
{
    (void) buf;
    *length = 0;
}

bool RMFMediaSourcePrivate::getAudioPidFromPMT(uint32_t*, const std::string&)
{
    return false;
}

void RMFMediaSourcePrivate::setFilter(uint16_t pid, char* filterParam, uint32_t *pFilterId)
{
    (void) pid;
    (void) filterParam;
    *pFilterId = 0;
}

void RMFMediaSourcePrivate::getSectionData(uint32_t *filterId, std::vector<uint8_t>& buf, uint32_t* length)
{
    (void) buf;
    *filterId = 0;
    *length = 0;
}

void RMFMediaSourcePrivate::releaseFilter(uint32_t filterId)
{
    (void)filterId;
}

void RMFMediaSourcePrivate::resumeFilter(uint32_t )
{
}

void RMFMediaSourcePrivate::pauseFilter(uint32_t )
{
}

void RMFMediaSourcePrivate::sendCCIChangedEvent( char cciInfo )
{
    sink_map_t::iterator it;
    g_static_rec_mutex_lock( &m_mutex);
    for(it= m_sinks.begin(); it != m_sinks.end(); ++it)
    {
        RMFMediaSinkBase *rmfSink= it->second;

        if ( rmfSink )
        {
            RMFMediaSinkPrivate *impl= rmfSink->getPrivateSinkImpl();
            if ( impl )
            {
                impl->handleCCIChange( cciInfo);
            }
        }
    }
    g_static_rec_mutex_unlock( &m_mutex);
}

// virtual
RMFResult RMFMediaSourcePrivate::getRMFError(int error_code)
{
    RMFResult error_type = RMF_RESULT_FAILURE;
    return error_type;

}

RMFStateChangeReturn RMFMediaSourcePrivate::getState(RMFState* current, RMFState* pending)
{
    if (!m_pipeline)
        return RMF_STATE_CHANGE_FAILURE;

    GstState gst_current;
    GstState gst_pending;

    float timeout = 0.0;
    GstStateChangeReturn ret = gst_element_get_state(
        m_pipeline, &gst_current, &gst_pending, timeout * GST_SECOND);

    if (current)
        *current = (RMFState) gst_current;
    if (pending)
        *pending = (RMFState) gst_pending;
    return (RMFStateChangeReturn) ret;
}

IRMFMediaEvents* RMFMediaSourcePrivate::getEvents()
{
    return m_events;
}

//ADDED
std::vector<IRMFMediaEvents*> RMFMediaSourcePrivate::getEventHandler()
{
    return m_event_handler;
}

// virtual
void RMFMediaSourcePrivate::setQueueSize(int queueSize)
{
   m_queueSize= queueSize;
}

int RMFMediaSourcePrivate::getQueueSize()
{
   return m_queueSize;
}

void RMFMediaSourcePrivate::setQueueLeaky( bool leaky)
{
   m_leakyQueue= leaky;
}

bool RMFMediaSourcePrivate::getQueueLeaky()
{
   return m_leakyQueue;
}

GstElement* RMFMediaSourcePrivate::getTeeElement()
{
   char *pTeeSrc = NULL;

   pTeeSrc = getenv("USE_GENERIC_GSTREAMER_TEE");
   if ((pTeeSrc != NULL) && (strcasecmp(pTeeSrc, "TRUE") == 0))
   {
      return NULL;
   }

   if (m_tee)
      return m_tee;
   else
      return NULL;
}

unsigned int RMFMediaSourcePrivate::getSinkCount(void)
{
    return m_sinks.size();
}

// virtual
RMFResult RMFMediaSourcePrivate::addSink(RMFMediaSinkBase* sink)
{
    RMF_TRACE("RMFMediaSourcePrivate::addSink enter\n");
    if (!m_pipeline)
        return RMF_RESULT_NOT_INITIALIZED;

    if (m_queues.find(sink) != m_queues.end())
        return RMF_RESULT_INVALID_ARGUMENT;

    // Create a new queue for the sink and connect it to the tee.
    RMF_TRACE("creating queue element\n");
    GstElement* sink_el = sink->getPrivateSinkImpl()->getSink();
    #if (defined(RMF_STREAMER) && !defined(RMF_FCC_ENABLED))
    char customQueueName[256];
    char* const source_el_name= gst_element_get_name(m_source);     
    char* const sink_el_name = gst_element_get_name(sink_el);
    memset(customQueueName,0,256);
    if (NULL == source_el_name)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.RMFBASE", "%s : Source element name found NULL.Asserting\n",__FUNCTION__);
        assert( false );
    }
    if (NULL == sink_el_name)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.RMFBASE", "%s : Sink element name found NULL.Asserting\n",__FUNCTION__);
        assert( false );
    }
    snprintf( customQueueName,256, "%s-queue-%s", source_el_name, sink_el_name );
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.RMFBASE", "CustomQueue=%s\n", customQueueName);
    if (source_el_name)
    {
    	g_free(source_el_name);
    }
    if (sink_el_name)
    {
        g_free(sink_el_name);
    }
    GstElement* queue = gst_element_factory_make("queue", customQueueName);
    #else
    GstElement* queue = gst_element_factory_make("queue", NULL); // a unique name will be generated
    #endif
    if (!queue)
    {
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.RMFBASE", "Error instantiating queue\n");
        return RMF_RESULT_INTERNAL_ERROR;
    }
    else
    {
        RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "Queue instantiated successfully.\n");
    }
    #ifdef RMF_STREAMER
    //RMF_registerGstElementDbgModule( (char*)"queue_datadrop",  (char*)"LOG.RDK.RMFBASE");
    RMF_registerGstElementDbgModule((char*)"basesrc", (char*)"LOG.RDK.RMFBASE");
    #endif
    if ( m_queueSize > 0 )
    {
       g_object_set(queue, "max-size-buffers", m_queueSize, NULL);
    }
    else if( m_queueSize == 0)
    {
       g_object_set(queue, "max-size-buffers", 0, NULL);
    }

    if ( m_leakyQueue )
    {
       g_object_set(queue, "leaky", 2, NULL);
    }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.RMFBASE", "QueueSize=%d QueueType=%s\n", m_queueSize,
        m_leakyQueue ? "leaky" : "non-leaky");

    RMF_TRACE("adding queue element\n");
    if ((m_pipeline != NULL) && (queue != NULL))
    {
       RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "m_pipeline=%p queue=%p\n", m_pipeline, queue );
       gst_bin_add_many(GST_BIN(m_pipeline), queue, NULL);
    }
    else
    {
       RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "pipeline or queue is NULL\n");
       assert(0);
    }
    RMF_TRACE("done adding queue element\n");
    RETURN_UNLESS(gst_element_sync_state_with_parent(queue) == true);


    // Append the sink element to the pipeline and link it to the newly created queue.
    RMF_TRACE("adding sink to pipeline\n");
    if ((m_pipeline != NULL) && (sink_el != NULL))
    {
       RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "m_pipeline=%p sink=%p\n", m_pipeline, sink_el );
       gst_bin_add_many(GST_BIN(m_pipeline), sink_el, NULL);
    }
    else
    {
       RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "pipeline or queue is NULL\n");
       assert(0);
    }
    RMF_TRACE("done adding sink to pipeline\n");

    RETURN_UNLESS(gst_element_link(queue, sink_el) == true);
    RETURN_UNLESS(gst_element_sync_state_with_parent(sink_el) == true);
    RETURN_UNLESS(gst_element_link(m_tee, queue));

    // Remember the queue name so that we can later look the queue element up.
    char* const queue_name = gst_element_get_name(queue);

    g_static_rec_mutex_lock( &m_mutex);
    m_queues[sink] = queue_name;
    RMF_TRACE("%s: added sink: %s\n", GST_ELEMENT_NAME(m_source), GST_ELEMENT_NAME(sink_el));
    g_free(queue_name);
    
    // Add sink to map for event source identification
    m_sinks[sink_el]= sink;
    g_static_rec_mutex_unlock( &m_mutex);

    RMF_TRACE("RMFMediaSourcePrivate::addSink exit\n");
    return RMF_RESULT_SUCCESS;
}

struct probe_context_t {
    unsigned int signature;
    GstElement* tee;
    GstElement* queue;
    GstElement* sink_el;
};

#if USE_GST1
static GstPadProbeReturn event_probe_cb(GstPad * pad, GstPadProbeInfo *info, gpointer data)
#else
static gboolean event_probe_cb(GstPad * pad, GstEvent *event, gpointer data)
#endif
{
	probe_context_t* context  = (probe_context_t *)data;
#ifndef USE_GST1
	if (!GST_EVENT_IS_DOWNSTREAM(event))
	{
		return 1;
	}
#else
	RDK_LOG( RDK_LOG_WARN, "LOG.RDK.RMFBASE", "user_data=%p probe_id=%ld\n",
		data, GST_PAD_PROBE_INFO_ID (info) );
	if (NULL == context || GST_EVENT_TYPE (GST_PAD_PROBE_INFO_DATA (info)) != GST_EVENT_CUSTOM_DOWNSTREAM
		|| 0xb0adcafe != context->signature)
	{
		RDK_LOG( RDK_LOG_WARN, "LOG.RDK.RMFBASE", "signature=0x%x\n", context->signature );
		return GST_PAD_PROBE_PASS;
	}
	gst_pad_remove_probe (pad, GST_PAD_PROBE_INFO_ID (info));
#endif
	gst_element_unlink(context->tee, context->queue);
	gst_element_release_request_pad(context->tee, pad);
#ifdef USE_GST1
	return GST_PAD_PROBE_OK;
#else
	return 0;
#endif
}

// virtual
RMFResult RMFMediaSourcePrivate::removeSink(RMFMediaSinkBase* sink)
{
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::removeSink :: Entry\n");
	// Look up the queue element corresponding to the given sink.
	if (!m_pipeline)
		return RMF_RESULT_NOT_INITIALIZED;

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::removeSink :: Request Mutex Lock\n");
	g_static_rec_mutex_lock( &m_mutex);
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::removeSink :: Obtained Mutex Lock\n");
	queue_map_t::iterator queue_it = m_queues.find(sink);
	if (queue_it == m_queues.end())
	{
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::removeSink :: Mutex Unlock\n");
		g_static_rec_mutex_unlock( &m_mutex);
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::removeSink :: Mutex Unlocked\n");
		return RMF_RESULT_NO_SINK;
	}

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::removeSink :: Sinks Erase\n");
	// Remove sink from map
	GstElement* old_sink_el = static_cast<RMFMediaSinkPrivate*>(sink->getPrivateSinkImpl())->getSink();
	sink_map_t::iterator sink_it = m_sinks.find(old_sink_el);
	if (sink_it != m_sinks.end())
		m_sinks.erase(sink_it);

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::removeSink :: Queues Erase\n");
	const std::string queue_name = queue_it->second;
	m_queues.erase(queue_it);

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::removeSink :: Mutex Unlock\n");
	g_static_rec_mutex_unlock( &m_mutex);
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::removeSink :: Mutex Unlocked\n");
	GstElement* queue = gst_bin_get_by_name(GST_BIN(m_pipeline), queue_name.c_str());
	assert(queue);

	// Remove the queue and the sink element from the pipeline, destroy the queue.
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::removeSink :: Calling getSink\n");
	GstElement* sink_el = sink->getPrivateSinkImpl()->getSink();
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::removeSink :: Out of getSink\n");

	GstPad* queue_sink_pad = gst_element_get_static_pad(queue, "sink");
	GstPad* tee_src_pad  = gst_pad_get_peer(queue_sink_pad);
	GstPad* tee_sink_pad  = gst_element_get_static_pad(m_tee, "sink");
	assert(queue_sink_pad);
	assert(tee_src_pad);
	assert(tee_sink_pad);

	if ( GST_STATE(queue) == GST_STATE_PLAYING ) {
		//Inject GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM event in tee_sink_pad
		probe_context_t *c = (probe_context_t *) g_malloc(sizeof(probe_context_t));
		c->signature = 0xb0adcafe;
		c->queue = queue;
		c->sink_el = sink_el;
		c->tee = m_tee;
#ifdef USE_GST1
		gulong probe_id = gst_pad_add_probe(tee_src_pad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, event_probe_cb, c, NULL );
		RDK_LOG( RDK_LOG_WARN, "LOG.RDK.RMFBASE", "%s: probe_context_t=%p probe_id=%ld\n",
			__FUNCTION__, c, probe_id );
#else
		gst_pad_add_event_probe(tee_src_pad, G_CALLBACK(event_probe_cb), c );
#endif
		GstStructure *s = gst_structure_new("removesink", "msg", G_TYPE_STRING, "down", NULL);
		gst_pad_send_event(tee_sink_pad,
			gst_event_new_custom(GstEventType(GST_EVENT_CUSTOM_DOWNSTREAM), s));

		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "%s: after send_event\n", __FUNCTION__);
		g_free(c);
	} else {
		// Not in PLAYING state, unlink it here directly.
		gst_element_unlink(m_tee, queue);
		gst_element_release_request_pad(m_tee, tee_src_pad); 
	}

	gst_element_unlink(queue, sink_el);
	gst_element_set_state(sink_el, GST_STATE_NULL);
	gst_element_set_state(queue, GST_STATE_NULL);

	gst_object_unref(tee_src_pad);
	gst_object_unref(queue_sink_pad);
	gst_object_unref(tee_sink_pad);

	RETURN_UNLESS(gst_bin_remove(GST_BIN(m_pipeline), queue) == true);
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::removeSink :: Removed Queue\n");
	RETURN_UNLESS(gst_bin_remove(GST_BIN(m_pipeline), sink_el) == true);
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::removeSink :: Removed Sink\n");

	// Remove extra reference added by gst_bin_get_by_name
	gst_object_unref(queue);

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::removeSink :: Exit\n");

	return RMF_RESULT_SUCCESS;
}

GstElement* RMFMediaSourcePrivate::getPipeline()
{
    return m_pipeline;
}

GstElement* RMFMediaSourcePrivate::getSource()
{
    return m_source;
}

GstElement* RMFMediaSourcePrivate::getSink(const std::string& name)
{
    for (queue_map_t::const_iterator it = m_queues.begin(); it != m_queues.end(); ++it)
        if(it->second == name)
            return it->first->getPrivateSinkImpl()->getSink();
    return NULL; 
}

RMFResult RMFMediaSourcePrivate::notifySpeedChange(float new_speed)
{
    for (queue_map_t::const_iterator it = m_queues.begin(); it != m_queues.end(); ++it)
        it->first->onSpeedChange(new_speed);
    return RMF_RESULT_SUCCESS;
}

RMFResult RMFMediaSourcePrivate::flushPipeline (void)
{
    /** There is a bug in Gstreamer-0.10:
            When application sends flush_start and flush_stop to the pipeline which has the
            SOURCE element based on GSTBaseSrc, it does NOT pass it to the downstream element
            of the pipeline. (Bug 689113)
     */
    /* To avoid the bug, instead of sending to the m_pipeline, send it to the m_tee;
     * HNSource is going to NULL state; so m_tee is right place
     */
    if (m_pipeline)
    {
#ifdef USE_GST1
        g_print("RMFMediaSourcePrivate::flushPipeline - Send gst_element_send_event - gst_event_new_flush_start to m_pipeline\n");
        gst_element_send_event(m_pipeline, gst_event_new_flush_start());
        gst_element_send_event (m_pipeline, gst_event_new_flush_stop(TRUE));
        g_print("RMFMediaSourcePrivate::flushPipeline - Sent gst_element_send_event - gst_event_new_flush_stop to m_pipeline\n");
#else
        g_print("RMFMediaSourcePrivate::flushPipeline - Send gst_element_send_event - gst_event_new_flush_start to m_tee\n");
        gst_element_send_event(m_tee, gst_event_new_flush_start());
        gst_element_send_event (m_tee, gst_event_new_flush_stop());
        g_print("RMFMediaSourcePrivate::flushPipeline - Sent gst_element_send_event - gst_event_new_flush_stop to m_tee\n");
#endif
    }

    return RMF_RESULT_SUCCESS;
}

bool RMFMediaSourcePrivate::isLiveSource (void)
{
    return m_isLive;
}

RMFResult RMFMediaSourcePrivate::updateSourcetoLive (bool isLive)
{
    m_isLive = isLive;
    return RMF_RESULT_SUCCESS;
}

RMFResult RMFMediaSourcePrivate::playAtLivePosition(double duration)
{
    return RMF_RESULT_SUCCESS;
}

RMFResult RMFMediaSourcePrivate::setState(GstState state)
{
    if (!m_pipeline)
        return RMF_RESULT_NOT_INITIALIZED;

    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::setState() :: Calling gst_element_set_state(%s)\n", gst_element_state_get_name(state));
    GstStateChangeReturn ret = gst_element_set_state(m_pipeline, state);

	/*
 	The possible return values from a state change function such as gst_element_set_state(). 
	Only GST_STATE_CHANGE_FAILURE is a real failure.

	GST_STATE_CHANGE_FAILURE - the state change failed
	GST_STATE_CHANGE_SUCCESS - the state change succeeded
	GST_STATE_CHANGE_ASYNC - the state change will happen asynchronously
	GST_STATE_CHANGE_NO_PREROLL - the state change succeeded but the element cannot produce data in GST_STATE_PAUSED. 
	This typically happens with live sources. 
	*/

    if (ret != GST_STATE_CHANGE_ASYNC && ret != GST_STATE_CHANGE_SUCCESS && ret != GST_STATE_CHANGE_NO_PREROLL)
    {
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.RMFBASE", "Failed to change state to %s streaming: %s\n",
            gst_element_state_get_name(state), gst_element_state_change_return_get_name(ret));
        return RMF_RESULT_FAILURE;
    }

    RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.RMFBASE", "Changed state to %s\n",
         gst_element_state_get_name(state));

    return RMF_RESULT_SUCCESS;
}

void RMFMediaSourcePrivate::dumpQueueMap()
{
    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "<<< QUEUE DUMP <<<\n");
    for (queue_map_t::const_iterator it = m_queues.begin(); it != m_queues.end(); ++it)
    {
        GstElement* queue = gst_bin_get_by_name(GST_BIN(m_pipeline), it->second.c_str()); // increases queue's refcount
        GstPad* queue_src_pad = gst_element_get_static_pad(queue, "src");
        GstPad* sink_src_pad = gst_pad_get_peer(queue_src_pad);

        GstElement* sink = gst_pad_get_parent_element(sink_src_pad); // increases sink's refcount

        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "%p: %s (queue refcnt: %d, sink refcnt: %d)\n", it->first, it->second.c_str(),
            GST_OBJECT_REFCOUNT_VALUE(queue) - 1, GST_OBJECT_REFCOUNT_VALUE(sink) - 1);

        gst_object_unref(queue_src_pad);
        gst_object_unref(sink_src_pad);

        gst_object_unref(sink);
        gst_object_unref(queue);
    }
    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", ">>> QUEUE DUMP >>>\n");
}

// static
gboolean RMFMediaSourcePrivate::busCall(GstBus*, GstMessage* msg, gpointer data)
{
    return ((RMFMediaSourcePrivate*)data)->handleBusMessage(msg);
}

GstBusSyncReply RMFMediaSourcePrivate::syncHandler( GstBus *bus, GstMessage *msg, gpointer userData )
{
   switch( msg->type )
   {
      #ifdef RMF_STREAMER
      case GST_MESSAGE_STREAM_STATUS:
        {
           GstStreamStatusType streamStatusType;
           GstElement *owner;
           
           gst_message_parse_stream_status( msg, &streamStatusType, &owner );
           
           switch( streamStatusType )
           {
               case GST_STREAM_STATUS_TYPE_ENTER:
                 {
                   // register gstreamer thread with a thread name
#ifdef USE_GST1
                   const GValue *v;

                   v = gst_message_get_stream_status_object(msg);
                   if (G_VALUE_HOLDS_OBJECT (v)) {
                     gchar *threadName = gst_object_get_name( (GstObject*) g_value_get_object(v));

                     rmf_osal_ThreadRegSet( RMF_OSAL_RES_TYPE_THREAD, 0, threadName,  1024*1024, 0 );
                     g_free (threadName);
                   }
#else
                   char *threadName = gst_object_get_name( (GstObject*)g_value_get_object(gst_message_get_stream_status_object(msg)) );
                   rmf_osal_ThreadRegSet( RMF_OSAL_RES_TYPE_THREAD, 0, threadName,  1024*1024, 0 );
#endif
                 }
                 break;
               case GST_STREAM_STATUS_TYPE_LEAVE:
                 {
                   // remove gstreamer thread registration
                   rmf_osal_ThreadRegRemove( RMF_OSAL_RES_TYPE_THREAD, 0 );
                 }
                 break;
               default:
                 break;
           }
        }
        break;
      #endif
      default:
        break;
   }

   return GST_BUS_PASS;
}

/* Handle Messages received */
RMFResult RMFMediaSourcePrivate::handleElementMsg (GstMessage* msg)
{
   RDK_LOG( RDK_LOG_WARN, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::handleElementMsg\n");
   return RMF_RESULT_NOT_IMPLEMENTED;
}

bool RMFMediaSourcePrivate::handleBusMessage(GstMessage* msg)
{

    g_static_rec_mutex_lock( &m_mutex);

    if ( handleSinkMessage(msg) )
    {
        g_static_rec_mutex_unlock( &m_mutex);
        return true;
    }
    
    switch (GST_MESSAGE_TYPE(msg))
    {
    case GST_MESSAGE_EOS:
    case GST_MESSAGE_SEGMENT_DONE:
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "End of stream\n");
#if 0
       if (isTrailerErrorAvailable()) {
           char errorMsg[256] = {'\0'};
           RMFResult rmfErrorResult = RMF_RESULT_FAILURE;
           if (getRMFTrailerError(rmfErrorResult, errorMsg) == RMF_RESULT_SUCCESS) {
               RDK_LOG( RDK_LOG_WARN, "LOG.RDK.RMFBASE", "ERROR CODE - %d\n", rmfErrorResult);
               RDK_LOG( RDK_LOG_WARN, "LOG.RDK.RMFBASE", "ERROR MESSAGE - %s\n", errorMsg);
           }
       }
#endif
        onEOS();
        break;

    case GST_MESSAGE_ERROR:
    {
        GError *error;
        gchar *dbg_info = NULL;
        gst_message_parse_error(msg, &error, &dbg_info);
        char* errMsg = g_strdup_printf("Error: %s", error->message);
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "Debugging info: %s\n", (dbg_info) ? dbg_info : "none");        

        onError(getRMFError(error->code), errMsg);     

        g_error_free(error);
        g_free(errMsg);
        g_free (dbg_info);
        break;
    }

    case GST_MESSAGE_WARNING:
    {
        GError *error;
        gchar *dbg_info = NULL;
        gst_message_parse_warning(msg, &error, &dbg_info);

        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "Warning: %s (%s)\n", error->message, (dbg_info) ? dbg_info : "none");

        g_error_free(error);
        g_free (dbg_info);
        break;
    }

    case GST_MESSAGE_STATE_CHANGED:
        GstState old_state;
        GstState new_state;
        GstState pending_state;
        gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);

        if (new_state == pending_state) break;
#ifdef RMFBASE_DEBUG
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::handleBusMessage: Element %s changed state from %s to %s\n",
            GST_OBJECT_NAME(msg->src),
            gst_element_state_get_name(old_state),
            gst_element_state_get_name(new_state));
#endif

        if (GST_ELEMENT(GST_MESSAGE_SRC(msg)) == m_pipeline)
            onStateChange(new_state);
        break;

	case GST_MESSAGE_STREAM_STATUS:
	{
#ifdef RMFBASE_DEBUG
		GstStreamStatusType type;
		GstElement *owner;

		gst_message_parse_stream_status(msg, &type, &owner);

        std::string type_str;
		switch (type)
		{
		case GST_STREAM_STATUS_TYPE_CREATE:
			type_str = "CREATE"; break;
		case GST_STREAM_STATUS_TYPE_ENTER:
			type_str = "ENTER"; break;
		case GST_STREAM_STATUS_TYPE_LEAVE:
			type_str = "LEAVE"; break;
		case GST_STREAM_STATUS_TYPE_DESTROY:
			type_str = "DESTROY"; break;
		case GST_STREAM_STATUS_TYPE_START:
			type_str = "START"; break;
		case GST_STREAM_STATUS_TYPE_PAUSE:
			type_str = "PAUSE"; break;
		case GST_STREAM_STATUS_TYPE_STOP:
			type_str = "STOP"; break;
		}

		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "Stream status change: %s from [%s]\n",
			type_str.c_str(),
			GST_MESSAGE_SRC_NAME(msg));
#endif
		break;
	}

#ifdef USE_GST1
    case GST_MESSAGE_STREAM_START:
    {
#ifdef RMFBASE_DEBUG
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "Stream start\n");
#endif
        break;
    }
#endif

    case GST_MESSAGE_ASYNC_DONE:
    {
#ifdef RMFBASE_DEBUG
        RMFState new_state;
        getState(&new_state, NULL);
        
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::handleBusMessage: "
            "Element %s completed async status change to %s\n",
            GST_MESSAGE_SRC_NAME(msg), gst_element_state_get_name((GstState)new_state));
        if (GST_ELEMENT(GST_MESSAGE_SRC(msg)) == m_pipeline)
        {
            RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::handleBusMessage: signalling state change\n");
            onStateChange((GstState) new_state);
        }
        else
            RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::handleBusMessage: NOT signalling state change\n");
#endif
        break;
    }

    case GST_MESSAGE_DURATION:
    {
#ifdef RMFBASE_DEBUG
        GstFormat format = GST_FORMAT_BYTES;
        gint64 duration = 0;
        gst_message_parse_duration(msg, &format, &duration);
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::handleBusMessage: File size: %.3f MB\n", duration / 1024.0 / 1024.0);
#endif
        break;
    }

    case GST_MESSAGE_NEW_CLOCK:
    {
#ifdef RMFBASE_DEBUG
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "New clock\n");
#endif
        break;
    }

    // ignore these
    case GST_MESSAGE_ELEMENT:
#ifdef RMFBASE_DEBUG
		/* Handle elemental message - Pertaining to trailer */
		RDK_LOG( RDK_LOG_WARN, "LOG.RDK.RMFBASE", "RECEIVED GST ELEMENT MESSAGE\n");
#endif
		handleElementMsg(msg);
		break;
	case GST_MESSAGE_TAG:
	case GST_MESSAGE_QOS:
		break;

    default:
		RDK_LOG( RDK_LOG_WARN, "LOG.RDK.RMFBASE", "Unknown bus call (%s) occurred\n", GST_MESSAGE_TYPE_NAME(msg));
        break;
    }

    g_static_rec_mutex_unlock( &m_mutex);
    return true;
}

bool RMFMediaSourcePrivate::handleSinkMessage(GstMessage* msg)
{
    bool drop_message= false;
   
    GstObject *source= GST_MESSAGE_SRC(msg);
    if ( source )
    {
        if ( GST_IS_ELEMENT(source) )
        {
            GstElement *sourceEl= GST_ELEMENT(source);
            sink_map_t::iterator it= m_sinks.find( sourceEl );
            if ( it != m_sinks.end() )
            {
               RMFMediaSinkBase *rmfSink= m_sinks[sourceEl];
               
               if ( rmfSink )
               {
                  RMFMediaSinkPrivate *impl= rmfSink->getPrivateSinkImpl();
                  if ( impl )
                  {
                     impl->handleBusMessage(msg);
                     drop_message = true;
                  }
               }
            }
            else if ( m_sinks.size() )
            {
                for( sink_map_t::iterator it = m_sinks.begin(); it != m_sinks.end(); it++ )
                {
                    RMFMediaFilterBase *filter = dynamic_cast<RMFMediaFilterBase*>(it->second);
                    if ( filter )
                    {
                        RMFMediaSourcePrivate *srcImpl = filter->getPrivateSourceImpl();
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "%s : RMFMediaFilterBase=%p RMFMediaSourcePrivate=%p\n", __FUNCTION__, filter, srcImpl);
                        if ( srcImpl )
                        {
                            (static_cast<RMFMediaFilterPrivate*>(srcImpl))->handleSinkMessage(msg);
                            drop_message = true;
                        }
                    }
                }
            }
        }
    }
   
    return drop_message;
}

void RMFMediaSourcePrivate::onEOS()
{
    if (m_events)
        m_events->complete();

    g_static_rec_mutex_lock( &m_mutex);
    //ADDED 
    for( std::vector<IRMFMediaEvents*>::iterator it= m_event_handler.begin(); it != m_event_handler.end(); ++it )
    {
        if(*it)
          (*it)->complete();
    }
    g_static_rec_mutex_unlock( &m_mutex);

    if (m_loop)
        g_main_loop_quit(m_loop);
}

void RMFMediaSourcePrivate::onPlaying()
{
    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "GST Pipeline set to PLAYING state\n");
    return;
}

void RMFMediaSourcePrivate::onError(RMFResult err, const char* msg)
{
    (void)err;
    if (m_events)
        m_events->error(err, msg);
    else
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "%s\n", msg);

    //ADDED
    g_static_rec_mutex_lock( &m_mutex);
    for( std::vector<IRMFMediaEvents*>::iterator it= m_event_handler.begin(); it != m_event_handler.end(); ++it )
    {
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::onError :: %s\n", msg);
        if(*it)
          (*it)->error(err, msg);
        else
          RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "%s\n", msg);     
    }
    g_static_rec_mutex_unlock( &m_mutex);

    if (m_loop)
        g_main_loop_quit(m_loop);
}

void RMFMediaSourcePrivate::onStatusChange(RMFStreamingStatus& status)
{

    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::onStatusChange :: %p\n", (uintptr_t)status.m_status);
    if (m_events)
        m_events->status( status);

    g_static_rec_mutex_lock( &m_mutex);
    for( std::vector<IRMFMediaEvents*>::iterator it= m_event_handler.begin(); it != m_event_handler.end(); ++it )
    {
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "%s %d Iterator is %p size is %d\n",__FUNCTION__,__LINE__,(uintptr_t)*it,m_event_handler.size());
        if(*it)
          (*it)->status( status);
    }
    g_static_rec_mutex_unlock( &m_mutex);
}

void RMFMediaSourcePrivate::onCaStatus(const void* data_ptr, unsigned int data_size)
{
    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::onCaStatus(data_ptr: %p, data_size: %d)\n", data_ptr, data_size);
    if (data_ptr && data_size)
    {
      if (m_events)
      {
          RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "call caStatus()\n");
          m_events->caStatus(data_ptr, data_size);
      }
      else
      {
          RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "no m_events\n");
      }

      //ADDED
      g_static_rec_mutex_lock( &m_mutex);
      for( std::vector<IRMFMediaEvents*>::iterator it= m_event_handler.begin(); it != m_event_handler.end(); ++it )
      {
          RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::onCaStatus()\n");
          if(*it)
          {
            RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "call it->caStatus()\n");
            (*it)->caStatus(data_ptr, data_size);
          }
          else
          {
            RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "---\n");
          }
      }
      g_static_rec_mutex_unlock( &m_mutex);
    }
    if (m_loop)
        g_main_loop_quit(m_loop);
}

void RMFMediaSourcePrivate::onSection(const void* data_ptr, unsigned int data_size)
{
    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::onSection(data_ptr: %p, data_size: %d)\n", data_ptr, data_size);
    if (data_ptr && data_size)
    {
      if (m_events)
      {
          RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "call section event()\n");
          m_events->section(data_ptr, data_size);
      }
      else
      {
          RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "no m_events\n");
      }

      //ADDED
      g_static_rec_mutex_lock( &m_mutex);
      for( std::vector<IRMFMediaEvents*>::iterator it= m_event_handler.begin(); it != m_event_handler.end(); ++it )
      {
          RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate::section()\n");
          if(*it)
          {
            RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "call it->section()\n");
            (*it)->section(data_ptr, data_size);
          }
          else
          {
            RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "---\n");
          }
      }
      g_static_rec_mutex_unlock( &m_mutex);
    }
    if (m_loop)
        g_main_loop_quit(m_loop);
}

void RMFMediaSourcePrivate::onStateChange(GstState new_state)
{
    RMFState cur_state = RMF_STATE_NULL;

    getState(&cur_state, NULL);
    g_static_rec_mutex_lock( &m_mutex);

    switch (new_state)
    {
    case GST_STATE_PAUSED:
        if(m_events)
            m_events->paused();
        //ADDED     
        for( std::vector<IRMFMediaEvents*>::iterator it= m_event_handler.begin(); it != m_event_handler.end(); ++it )
        {
            if(*it)
              (*it)->paused();
        }
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "GST Pipeline set to PAUSED state\n");
        break;
    case GST_STATE_PLAYING:
        if(m_events)
            m_events->playing();
        //ADDED
        for( std::vector<IRMFMediaEvents*>::iterator it= m_event_handler.begin(); it != m_event_handler.end(); ++it )
        {
            if(*it)
              (*it)->playing();
        }
        onPlaying();
        break;
    case GST_STATE_NULL:
    case GST_STATE_READY:
        if (cur_state != RMF_STATE_NULL && cur_state != RMF_STATE_READY)
        {
            if(m_events)
                m_events->stopped();
            for( std::vector<IRMFMediaEvents*>::iterator it= m_event_handler.begin(); it != m_event_handler.end(); ++it )
            {
                if(*it)
                    (*it)->stopped();
            }
        }
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "GST Pipeline set to READY state\n");
        break;
    default:
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "GST Pipeline unknown state\n");
        break;
    }
    g_static_rec_mutex_unlock( &m_mutex);
}


void RMFMediaSourcePrivate::notifyBusMessagingEnded()
{
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "g_source_remove signaled\n");
	rmf_osal_condSet(m_bus_messaging_disabled);
}

//-----------------------------------------------------------------------------
//-- RMFMediaFilterBase ---------------------------------------------------------
//-----------------------------------------------------------------------------

RMFMediaFilterBase::RMFMediaFilterBase()
  : m_impl( 0 )
{
}

RMFMediaFilterBase::~RMFMediaFilterBase()
{
   if ( m_impl )
   {
      delete m_impl;
      m_impl= 0;
      RMFMediaSourceBase::m_pimpl= 0;
      RMFMediaSinkBase::m_pimpl= 0;
   }
}

RMFResult RMFMediaFilterBase::init()
{
    RMFResult result;
    
    result= RMFMediaSourceBase::init();
    if ( result == RMF_RESULT_SUCCESS )
    {
       result= RMFMediaSinkBase::init();
    }

    return result;
}

RMFResult RMFMediaFilterBase::term()
{
    RMFResult result;

    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "RMFMediaFilterBase::term::ENTRY\n");
    result= RMFMediaSourceBase::term();
    if ( result == RMF_RESULT_SUCCESS )
    {
       result= RMFMediaSinkBase::term();
    }

    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "RMFMediaFilterBase::term::EXIT\n");
    return result;
}

RMFResult RMFMediaFilterBase::addSink(RMFMediaSinkBase* sink)
{
    return m_impl->addSink(sink);
}

RMFResult RMFMediaFilterBase::removeSink(RMFMediaSinkBase* sink)
{
    return m_impl->removeSink(sink);
}

RMFMediaSourcePrivate* RMFMediaFilterBase::createPrivateSourceImpl()
{
    // Common private impl for both source and sink
    if ( !m_impl )
    {
       m_impl= createPrivateFilterImpl();
    }
    return (RMFMediaSourcePrivate*)m_impl;
}

RMFMediaSinkPrivate* RMFMediaFilterBase::createPrivateSinkImpl()
{
    // Common private impl for both source and sink
    if ( !m_impl )
    {
       m_impl= createPrivateFilterImpl();
    }
    return (RMFMediaSinkPrivate*)m_impl;
}

RMFResult RMFMediaFilterBase::getSpeed(float& speed)
{
    return m_impl->getSpeed(speed);
}

RMFResult RMFMediaFilterBase::setSpeed(float speed)
{
    return m_impl->setSpeed(speed);
}

RMFResult RMFMediaFilterBase::getMediaTime(double& time)
{
    return m_impl->getMediaTime(time);
}

RMFResult RMFMediaFilterBase::setMediaTime(double time)
{
    return m_impl->setMediaTime(time);
}

RMFResult RMFMediaFilterBase::getMediaInfo(RMFMediaInfo& info)
{
    return m_impl->getMediaInfo(info);
}

//-----------------------------------------------------------------------------
//-- RMFMediaFilterPrivate ----------------------------------------------------
//-----------------------------------------------------------------------------

RMFMediaFilterPrivate::RMFMediaFilterPrivate(RMFMediaFilterBase* parent)
:   RMFMediaSourcePrivate(parent), RMFMediaSinkPrivate(parent)
,   m_parent(parent)
,   m_filter(NULL)
,   m_tee(NULL)
{
}

RMFResult RMFMediaFilterPrivate::init(bool enableSource)
{
   RMFResult result;

   g_static_rec_mutex_init (&m_mutex);
   g_static_rec_mutex_init (&m_teeMutex);

   // Initialize source bases.
   if (enableSource == true) {
      result = RMFMediaSourcePrivate::init();
      if ( result != RMF_RESULT_SUCCESS )
      {
         return result;
      }
   }

   // Initialize sink bases.

   result = RMFMediaSinkPrivate::init();
   if ( result == RMF_RESULT_SUCCESS )
   {
      m_filter = RMFMediaSinkPrivate::getSink();
      if ( !m_filter )
      {
         result= RMF_RESULT_INTERNAL_ERROR;
      }
   }
   return result;
}

RMFResult RMFMediaFilterPrivate::addSink(RMFMediaSinkBase* sink)
{
    // Create a new queue for the sink and connect it to the tee.
   RMF_TRACE("RMFMediaFilterPrivate::addSink enter\n");
   if (!m_filter)
       return RMF_RESULT_NOT_INITIALIZED;

   GstElement* pipeline= RMFMediaSinkPrivate::getSource()->getPrivateSourceImpl()->getPipeline();
   RETURN_UNLESS( pipeline != 0 );
   
   GstElement* sink_el = sink->getPrivateSinkImpl()->getSink();
   RETURN_UNLESS( sink_el != 0 );

   // Create a tee if we don't have one yet
   g_static_rec_mutex_lock(&m_teeMutex);
   if ( !m_tee )
   {
#ifdef USE_GST1
      m_tee = gst_element_factory_make("tee", "filter_tee");
#else
      char *pTeeSrc = NULL;
      pTeeSrc = getenv("USE_GENERIC_GSTREAMER_TEE");
      RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "pTeeSrc = %p\n", pTeeSrc);
      if ((pTeeSrc != NULL) && (strcasecmp(pTeeSrc, "TRUE") == 0))
      {
          m_tee = gst_element_factory_make("tee", "filter_tee");
      }
      else
      {
          m_tee = gst_element_factory_make("rmftee", "filter_tee");
      }
#endif
      RETURN_UNLESS(m_tee != 0);
      gst_object_ref_sink(m_tee);
   }
   g_static_rec_mutex_unlock(&m_teeMutex);

   RMF_TRACE("creating queue element\n");
   GstElement* queue = gst_element_factory_make("queue", NULL); // a unique name will be generated
   RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaFilterPrivate::addSink creating queue element %p\n", queue);
   if (!queue)
   {
       RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.RMFBASE", "Error instantiating queue.\n");
       RMF_TRACE("RMFMediaSourcePrivate::addSink exit\n");
       return RMF_RESULT_INTERNAL_ERROR;
   }
   int queueSize= RMFMediaSinkPrivate::getSource()->getPrivateSourceImpl()->getQueueSize();
   if ( queueSize > 0 )
   {
      g_object_set(queue, "max-size-buffers", queueSize, NULL);
   }

   bool leakyQueue= RMFMediaSinkPrivate::getSource()->getPrivateSourceImpl()->getQueueLeaky();
   if ( leakyQueue )
   {
      g_object_set(queue, "leaky", 2, NULL);
   }
   RDK_LOG(RDK_LOG_INFO, "LOG.RDK.RMFBASE", "QueueSize=%d QueueType=%s\n", queueSize,
        leakyQueue ? "leaky" : "non-leaky");

   RMF_TRACE("done creating queue element\n");

   RMF_TRACE("adding queue element\n");
   RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaFilterPrivate::addSink adding queue element %p\n", queue);
   RETURN_UNLESS(gst_bin_add(GST_BIN(pipeline), queue) == true);
   RMF_TRACE("done adding queue element\n");
   RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaFilterPrivate::addSink done adding queue element %p\n", queue);

   RETURN_UNLESS(gst_element_sync_state_with_parent(queue) == true);


   // Append the sink element to the pipeline and link it to the newly created queue.
   RMF_TRACE("adding sink to pipeline\n");
   RETURN_UNLESS(gst_bin_add(GST_BIN(pipeline), sink_el) == true);
   RMF_TRACE("done adding sink to pipeline\n");

   RETURN_UNLESS(gst_element_link(queue, sink_el) == true);
   RETURN_UNLESS(gst_element_sync_state_with_parent(sink_el) == true);

   // Remember the queue name so that we can later look the queue element up.
   char* const queue_name = gst_element_get_name(queue);
   RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaFilterPrivate::addSink saving queue element %p name %s\n", queue, queue_name);

   g_static_rec_mutex_lock( &m_mutex);
   // Add tee to pipeline and link to filter if not yet done
   RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaFilterPrivate::addSink sinks size %d\n", m_sinks.size());
   if ( m_sinks.size() == 0 )
   {
      RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaFilterPrivate::addSink sinks size 0: add tee to pipeline\n");
      RETURN_UNLESS(gst_bin_add(GST_BIN(pipeline), m_tee) == true);
      RETURN_UNLESS(gst_element_sync_state_with_parent(m_tee) == true);
      RETURN_UNLESS(gst_element_link(m_filter, m_tee) == true);
   }
   RETURN_UNLESS(gst_element_link(m_tee, queue));

   m_queues[sink] = queue_name;
   RMF_TRACE("%s: added sink: %s\n", GST_ELEMENT_NAME(m_source), GST_ELEMENT_NAME(sink_el));
   g_free(queue_name);

   // Add sink to map for event source identification
   m_sinks[sink_el]= sink;
   g_static_rec_mutex_unlock( &m_mutex);

   RMF_TRACE("RMFMediaFilterPrivate::addSink exit\n");

   return RMF_RESULT_SUCCESS;
}

RMFResult RMFMediaFilterPrivate::removeSink(RMFMediaSinkBase* sink)
{
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaFilterPrivate::removeSink :: Entry\n");
	// Look up the queue element corresponding to the given sink.
	GstElement* pipeline= RMFMediaSinkPrivate::getSource()->getPrivateSourceImpl()->getPipeline();
	if (!pipeline)
		return RMF_RESULT_NOT_INITIALIZED;

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaFilterPrivate::removeSink :: Request Mutex Lock\n");
	g_static_rec_mutex_lock( &m_mutex);
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaFilterPrivate::removeSink :: Obtained Mutex Lock\n");
	queue_map_t::iterator queue_it = m_queues.find(sink);
	if (queue_it == m_queues.end())
	{
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaFilterPrivate::removeSink :: Mutex Unlock\n");
		g_static_rec_mutex_unlock( &m_mutex);
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaFilterPrivate::removeSink :: Mutex Unlocked\n");
		return RMF_RESULT_NO_SINK;
	}

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaFilterPrivate::removeSink :: Sinks Erase\n");
	// Remove sink from map
	GstElement* old_sink_el = static_cast<RMFMediaSinkPrivate*>(sink->getPrivateSinkImpl())->getSink();
	sink_map_t::iterator sink_it = m_sinks.find(old_sink_el);
	if (sink_it != m_sinks.end())
		m_sinks.erase(sink_it);

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaFilterPrivate::removeSink :: Queues Erase\n");
	const std::string queue_name = queue_it->second;
	m_queues.erase(queue_it);

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaFilterPrivate::removeSink :: Mutex Unlock\n");
	g_static_rec_mutex_unlock( &m_mutex);
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaFilterPrivate::removeSink :: Mutex Unlocked\n");
	GstElement* queue = gst_bin_get_by_name(GST_BIN(pipeline), queue_name.c_str());
	assert(queue);

	// Remove the queue and the sink element from the pipeline, destroy the queue.
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaFilterPrivate::removeSink :: Calling getSink\n");
	GstElement* sink_el = sink->getPrivateSinkImpl()->getSink();
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaFilterPrivate::removeSink :: Out of getSink\n");

	g_static_rec_mutex_lock(&m_teeMutex);
	GstPad* queue_sink_pad = gst_element_get_static_pad(queue, "sink");
	GstPad* tee_src_pad  = gst_pad_get_peer(queue_sink_pad);
	GstPad* tee_sink_pad  = gst_element_get_static_pad(m_tee, "sink");
	assert(queue_sink_pad);
	assert(tee_src_pad);
	assert(tee_sink_pad);

	if ( GST_STATE(queue) == GST_STATE_PLAYING ) {
		//Inject GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM event in tee_sink_pad
		probe_context_t *c = (probe_context_t *) g_malloc(sizeof(probe_context_t));
		c->signature = 0xb0adcafe;
		c->queue = queue;
		c->sink_el = sink_el;
		c->tee = m_tee;

#ifdef USE_GST1
		gst_pad_add_probe(tee_src_pad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, event_probe_cb, c, NULL );
#else
		gst_pad_add_event_probe(tee_src_pad, G_CALLBACK(event_probe_cb), c );
#endif

		GstStructure*  s = gst_structure_new("removesink", "msg", G_TYPE_STRING, "down", NULL);

		gst_pad_send_event(tee_sink_pad, 
			gst_event_new_custom(GstEventType(GST_EVENT_CUSTOM_DOWNSTREAM | GST_EVENT_TYPE_SERIALIZED), s));

		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "%s: after send_event\n", __FUNCTION__);
		g_free(c);
	} else {
		// Not in PLAYING state, unlink it here directly.
		gst_element_unlink(m_tee, queue);
		gst_element_release_request_pad(m_tee, tee_src_pad); 
	}

	gst_element_unlink(queue, sink_el);
	gst_element_set_state(sink_el, GST_STATE_NULL);
	gst_element_set_state(queue, GST_STATE_NULL);

	gst_object_unref(tee_src_pad);
	gst_object_unref(queue_sink_pad);
	gst_object_unref(tee_sink_pad);
	g_static_rec_mutex_unlock(&m_teeMutex);

	RETURN_UNLESS(gst_bin_remove(GST_BIN(pipeline), queue) == true);
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaFilterPrivate::removeSink :: Removed Queue\n");
	RETURN_UNLESS(gst_bin_remove(GST_BIN(pipeline), sink_el) == true);
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaFilterPrivate::removeSink :: Removed Sink\n");

	// Unlink tee when last sink is removed
	g_static_rec_mutex_lock( &m_mutex);
	if ( m_sinks.size() == 0 )
	{
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "RMFMediaFilterPrivate::removeSink sinks size 0: remove tee from pipeline\n");
		gst_element_unlink(m_filter, m_tee);
		RETURN_UNLESS(gst_bin_remove(GST_BIN(pipeline), m_tee) == true);
	}
	g_static_rec_mutex_unlock( &m_mutex);

	// Remove extra reference added by gst_bin_get_by_name
	gst_object_unref(queue);

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "RMFMediaFilterPrivate::removeSink :: Exit\n");

	return RMF_RESULT_SUCCESS;
}

RMFResult RMFMediaFilterPrivate::setSource(IRMFMediaSource* source)
{
   RMFResult result;
   
   result= RMFMediaSinkPrivate::setSource(source);
   
   return result;
}

RMFResult RMFMediaFilterPrivate::term()
{
    RMFResult result = RMF_RESULT_SUCCESS;

    if ( m_tee && m_filter )
    {
       gst_element_unlink(m_filter, m_tee);
    }

    // Remove all the sinks from the pipeline.
    g_static_rec_mutex_lock( &m_mutex);
    for (queue_map_t::iterator it = m_queues.begin(); it != m_queues.end(); /*nop*/)
    {
        queue_map_t::iterator cur_it = it++;
        RMFMediaSinkBase* sink = cur_it->first;
        result = sink->setSource(NULL); // safer than removeSink() b/c it resets the source pointer in the sink

        if (result != RMF_RESULT_SUCCESS)
            RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.RMFBASE", "RMFMediaSourcePrivate: failed to remove sink\n");
    }
    g_static_rec_mutex_unlock( &m_mutex);
    assert(m_queues.size() == 0);
    
    // Terminate sink base.  We didn't initialize source base
    // so don't call source base term.
    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "RMFMediaFilterPrivate::term: ENTRY, m_filter = %p \n", m_filter);
    result = RMFMediaSinkPrivate::term();

    if ( m_tee )
    {
        gst_element_set_state(m_tee, GST_STATE_NULL);
        gst_object_unref(m_tee);
        m_tee = NULL;
    }
    m_filter = NULL;

    g_static_rec_mutex_free (&m_mutex);
    g_static_rec_mutex_free (&m_teeMutex);

    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.RMFBASE", "RMFMediaFilterPrivate::term: EXIT \n");
    return result;
}

RMFResult RMFMediaFilterPrivate::play()
{
   return RMFMediaSinkPrivate::getSource()->play();
}

RMFResult RMFMediaFilterPrivate::play(float& speed, double& time)
{
   return RMFMediaSinkPrivate::getSource()->play(speed,time);
}

RMFResult RMFMediaFilterPrivate::getSpeed(float& speed)
{
   return RMFMediaSinkPrivate::getSource()->getSpeed(speed);
}

RMFResult RMFMediaFilterPrivate::setSpeed(float speed)
{
   return RMFMediaSinkPrivate::getSource()->setSpeed(speed);
}

RMFResult RMFMediaFilterPrivate::getMediaTime(double& time)
{
   return RMFMediaSinkPrivate::getSource()->getMediaTime(time);
}

RMFResult RMFMediaFilterPrivate::setMediaTime(double time)
{
   return RMFMediaSinkPrivate::getSource()->setMediaTime(time);
}

RMFResult RMFMediaFilterPrivate::getMediaInfo(RMFMediaInfo& info)
{
   return RMFMediaSinkPrivate::getSource()->getMediaInfo(info);
}

bool RMFMediaFilterPrivate::handleSinkMessage(GstMessage* msg)
{
    bool msgHandled = false;

    GstObject *source= GST_MESSAGE_SRC(msg);
    if ( source )
    {
        if ( GST_IS_ELEMENT(source) )
        {
            g_static_rec_mutex_lock( &m_mutex );
            for( sink_map_t::iterator it = m_sinks.begin(); it != m_sinks.end(); it++ )
            {
                RMFMediaSinkPrivate *impl= (it->second)->getPrivateSinkImpl();
                if (!impl) continue;
                RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.RMFBASE", "%s : SinkBase=%p Sink impl=%p Sink GstElement=%p Originated GstElement:%p\n",
                         __FUNCTION__, it->second, impl, impl->getSink(), GST_ELEMENT(source) );
                if ( impl->getSink() == GST_ELEMENT(source) )
                {
                    impl->handleBusMessage(msg);
                    msgHandled = true;
                }
           }
           g_static_rec_mutex_unlock( &m_mutex );
       }
   }

   return msgHandled;
}
