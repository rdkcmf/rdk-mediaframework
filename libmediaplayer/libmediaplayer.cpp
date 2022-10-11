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
#include <unistd.h>
#include <stdio.h>
#include <sys/prctl.h>

#include <glib.h>
#include <gst/gst.h>

#include "rmf_osal_event.h"
#include "rmfqamsrc.h" 
#include "mediaplayersink.h"
#include "rmf_platform.h"

#include "libmediaplayer.h"
#include "libmediaplayer_utils.h"

namespace event_type 
{
    enum 
    {
        STATUS=0,
        ERROR,
        TERMINATE
    } _event_type;
}
namespace libmediaplayer
{
    class rmf_source_event_handler : public IRMFMediaEvents
    {
        public:
        typedef const char * (*translator_fn)(const RMFStreamingStatus& status);

        private:
        mediaplayer * m_player;
        std::string m_source;
        translator_fn m_status_translator;

        public:
        rmf_source_event_handler(mediaplayer * player, const std::string &source, translator_fn status_translator):
            m_player(player), m_source(source), m_status_translator(status_translator){}

        void status(const RMFStreamingStatus& status) override
        {
            switch(status.m_status)
            {
                case RMF_QAMSRC_EVENT_TUNE_SYNC: //fall through
                case RMF_QAMSRC_EVENT_STREAMING_STARTED: //fall through
                case RMF_QAMSRC_EVENT_ERRORS_ALL_CLEAR: //fall through
                case RMF_QAMSRC_EVENT_SPTS_OK: //fall through
                {
                    notification_payload * payload  = new notification_payload(m_source, static_cast <long long>(status.m_status),
                        m_status_translator(status), "");
                    m_player->enqueueEvent(event_type::STATUS, payload);
                }
                break;
                default:
                    break; //ignore all other status events.
            }
        }

        void error(RMFResult err, const char* msg) override
        {
            notification_payload *payload = new notification_payload(m_source, static_cast<long long>(err), msg, "");
            m_player->enqueueEvent(event_type::ERROR, payload);
        }
    };

    static const char * qam_status_translator(const RMFStreamingStatus &status)
    {
        const char *text = nullptr;
        switch (status.m_status)
        {
            case RMF_QAMSRC_EVENT_TUNE_SYNC:
                text = "tune sync";
                break;
            case RMF_QAMSRC_EVENT_STREAMING_STARTED:
                text = "streaming started";
                break;
            case RMF_QAMSRC_EVENT_ERRORS_ALL_CLEAR:
                text = "all errors cleared";
                break;
            case RMF_QAMSRC_EVENT_SPTS_OK:
                text = "spts ok";
                break;
            default:
                text = "";
                break;
        }
        return text;
    }

    static void mediaSinkHaveVideoCallback(void * data)
    {
        mediaplayer * player = static_cast <mediaplayer *> (data);
        notification_payload * payload  = new notification_payload("mpsink", 0, "video PES detected", "");
        player->enqueueEvent(event_type::STATUS, payload);
    }

    static void mediaSinkHaveAudioCallback(void * data)
    {
        mediaplayer * player = static_cast <mediaplayer *> (data);
        notification_payload * payload  = new notification_payload("mpsink", 0, "audio PES detected", "");
        player->enqueueEvent(event_type::STATUS, payload);
    }

    static void mediaSinkFirstVideoFrameCallback(void * data)
    {
        mediaplayer * player = static_cast <mediaplayer *> (data);
        notification_payload * payload  = new notification_payload("mpsink", 0, "report first video frame", "");
        player->enqueueEvent(event_type::STATUS, payload);
    }

    static void mediaSinkFirstAudioFrameCallback(void * data)
    {
        mediaplayer * player = static_cast <mediaplayer *> (data);
        notification_payload * payload  = new notification_payload("mpsink", 0, "report first audio frame", "");
        player->enqueueEvent(event_type::STATUS, payload);
    }

    static void mediaSinkMediaWarningCallback(void * data)
    {
        mediaplayer * player = static_cast <mediaplayer *> (data);
        MediaPlayerSink * sink = dynamic_cast <MediaPlayerSink *> (player->getSink());
        notification_payload * payload  = new notification_payload("mpsink", 0, "warning", sink->getMediaWarningString());
        player->enqueueEvent(event_type::STATUS, payload);
    }



    mp_sink_config_helper::mp_sink_config_helper()
    {
        INFO("Creating new mp_sink_config_helper object 0x%p.\n", this);

    }
    mp_sink_config_helper::~mp_sink_config_helper()
    {
        INFO("Destroying mp_sink_config_helper object 0x%p.\n", this);
    }

    int mp_sink_config_helper::platform_init()
    {
        int ret = 0;
        return ret;
    }

    int mp_sink_config_helper::platform_deinit()
    {
        int ret = 0;
        return ret;
    }

    int mp_sink_config_helper::setup(mediaplayer & player)
    {
        int ret = 0;
        MediaPlayerSink * sink_element = dynamic_cast <MediaPlayerSink *> (player.getSink());
        sink_element->init(); //TODO: check return and propagate errors.
        sink_element->setHaveVideoCallback(mediaSinkHaveVideoCallback, static_cast <void *> (&player));
        sink_element->setHaveAudioCallback(mediaSinkHaveAudioCallback, static_cast <void *> (&player));
        sink_element->setVideoPlayingCallback(mediaSinkFirstVideoFrameCallback, static_cast <void *> (&player));
        sink_element->setAudioPlayingCallback(mediaSinkFirstAudioFrameCallback, static_cast <void *> (&player));
        sink_element->setMediaWarningCallback(mediaSinkMediaWarningCallback, static_cast <void *> (&player));
        return ret;
    }

    int mp_sink_config_helper::undo_setup(mediaplayer &player)
    {
        int ret = 0;
        MediaPlayerSink * sink_element = dynamic_cast <MediaPlayerSink *> (player.getSink());
        sink_element->setHaveVideoCallback(nullptr, nullptr);
        sink_element->setHaveAudioCallback(nullptr, nullptr);
        sink_element->setVideoPlayingCallback(nullptr, nullptr);
        sink_element->setAudioPlayingCallback(nullptr, nullptr);
        sink_element->setMediaWarningCallback(nullptr, nullptr);
        sink_element->setSource(nullptr); //TODO: is this the right sequence for this call? Is this even necessary?
        sink_element->term(); //TODO: check return and propagate errors
        return ret;
    }

    qam_source_config_helper::qam_source_config_helper()
    {
        INFO("Creating new qam_source_config_helper object 0x%p.\n", this);

    }
    
    qam_source_config_helper::~qam_source_config_helper()
    {
        INFO("Destroying qam_source_config_helper object 0x%p.\n", this);
    }

    int qam_source_config_helper::platform_init()
    {
        int ret = 0;
        RMFQAMSrc::init_platform();
        return ret;
    }

    int qam_source_config_helper::platform_deinit()
    {
        int ret = 0;
        RMFQAMSrc::uninit_platform();
        return ret;
    }

    int qam_source_config_helper::setup(mediaplayer & player)
    {
        int ret = 0;
        player.setSourceEventHandler(std::make_unique <rmf_source_event_handler> (&player, "qam", qam_status_translator));
        return ret;
    }

    int qam_source_config_helper::undo_setup(mediaplayer &player)
    {
        int ret = 0;
        player.clearSourceEventHandler();
        return ret;
    }



    bool mediaplayer::m_create_mainloop = false;

    int mediaplayer::initialize(source_type_t type, bool initLogger, bool createMainLoop)
    {
        int ret = 0;

        int simulated_argc = 4;
        char * simulated_argv[] = {"--config", "/etc/rmfconfig.ini", "--debugconfig", "/etc/debug.ini"};
        rmfPlatform *mPlatform = rmfPlatform::getInstance();
        mPlatform->init(simulated_argc, simulated_argv); //Since this is just a library, simulate safe defaults for argc and argv.
        //The above call will not play nice if this library is operating in the context of rmfstreamer.

        //FIXME:the above platform init is not suitable for multiple initialize-deinitialize cycles. There is no way to exit
        // the default g_main_loop, so we'll be leaking a thread and a main loop any time we de-init.

        m_create_mainloop = createMainLoop; //Do nothing for now. rmfPlatform::init() will create a main loop thread.

        if(QAM == type)
        {
            INFO("Calling initialize for QAM.\n");
            qam_source_config_helper cfg; //TODO: Consider having just one configurer object and reusing it everywhere.
            cfg.platform_init();
        }
        else
        {
            ERROR("Unsupported source type %d. Cannot initialize.\n", type);
            //invalid source.
            ret = -1;
        }
        return ret;
    }

    int mediaplayer::deinitialize(source_type_t type)
    {
        int ret = 0;

        if(QAM == type)
        {
            INFO("Calling deinitialize for QAM.\n");
            qam_source_config_helper cfg; //TODO: inefficient. Consider having just one configurer object and reusing it everywhere.
            cfg.platform_deinit();
        }
        else
        {
            ERROR("Unsupported source type %d. Cannot deinitialize.\n", type);
            ret = -1;
        }

        rmfPlatform *mPlatform = rmfPlatform::getInstance();
        mPlatform->uninit(); //FIXME: Does this work? Does any other application do this?
        return ret;
    }

    //static player_id_t createMediaPlayer(std::string &service_descriptor);
    mediaplayer * mediaplayer::createMediaPlayer(source_type_t source_type, const std::string &service_descriptor)
    {
        mediaplayer * ptr = new mediaplayer(source_type, service_descriptor);
        if(nullptr == ptr)
        {
            ERROR("Could not create new mediaplayer object.\n");
        }
        return ptr;
    }

    mediaplayer::mediaplayer(source_type_t source_type, const std::string &service_descriptor): m_source_type(source_type), m_service_descriptor(service_descriptor),
        m_source_element(nullptr), m_status_callback(nullptr), m_error_callback(nullptr)
    {
        INFO("Creating new mediaplayer object for descriptor %s\n", m_service_descriptor.c_str());
        
        rmf_Error rmf_error = rmf_osal_eventqueue_create((const uint8_t*) "libmediaplayer", &m_event_queue);
        if (RMF_SUCCESS != rmf_error )
        {
            ERROR("%s: rmf_osal_eventqueue_create failed.\n", __FUNCTION__);
            //Not much we can do here since we're in a constructor. Not throwing an exception. The trigger is probably a memory issue and not much
            // we can do there. So chug along until an eventual crash.
        }
        m_dispatch_thread = std::thread(&mediaplayer::eventDispatcher, this);

        if(QAM == m_source_type)
        {
            m_source_configurer = std::make_unique <qam_source_config_helper> ();
        }

        m_sink_configurer = std::make_unique <mp_sink_config_helper> ();
        
        createSource();
        configureSource();
        createSink();
        configureSink();

        getSink()->setSource(m_source_element);

        //TODO: Check return and handle errors for all five calls above.

    }

    mediaplayer::~mediaplayer()
    {
        INFO("Destroying mediaplayer object for descriptor %s\n", m_service_descriptor.c_str());
        destroySourceAndSink();
        enqueueEvent(event_type::TERMINATE, nullptr);
        m_dispatch_thread.join();
        rmf_osal_eventqueue_delete(m_event_queue); 
    }

    static void free_event_payload (void* ptr)
    {
        notification_payload * nptr = static_cast <notification_payload *> (ptr);
        delete nptr;
    }

    void mediaplayer::enqueueEvent(unsigned int type, notification_payload * payload)
    {
        rmf_osal_event_handle_t event_handle;
        rmf_osal_event_params_t event_params = {0};
        rmf_Error ret = RMF_SUCCESS;

        event_params.data = static_cast <void *> (payload);
        event_params.free_payload_fn = free_event_payload;

        ret = rmf_osal_event_create(RMF_OSAL_EVENT_CATEGORY_QAM, type, &event_params, &event_handle);
        if(RMF_SUCCESS != ret)
        {
            ERROR("Failed to create status event\n");
            delete payload;
        }

        ret = rmf_osal_eventqueue_add(m_event_queue, event_handle);
        if (RMF_SUCCESS != ret)
        {
            ERROR("Failed to add to event queue.\n");
            rmf_osal_event_delete(event_handle);
        }
    }

    void mediaplayer::registerEventCallbacks(event_callback status_cb, event_callback error_cb, void * data)
    {
        m_status_callback = status_cb;
        m_error_callback = error_cb;
        m_callback_data = data;
        INFO("Registered callbacks.\n");
        return;
    }

    void mediaplayer::eventDispatcher()
    {
        INFO("%s thread starts.\n", __FUNCTION__);
        
        rmf_Error ret;
        rmf_osal_event_handle_t event_handle;
        uint32_t event_type;
        rmf_osal_event_params_t eventData;
        rmf_osal_event_category_t eventCategory;
        bool run_loop = true;

        while(run_loop)
	    {
            ret = rmf_osal_eventqueue_get_next_event(m_event_queue, &event_handle, &eventCategory, &event_type, &eventData);
            if ( RMF_SUCCESS != ret)
            {
                ERROR("Failed to get next event from queue.\n");
                break;
            }

            if(RMF_OSAL_EVENT_CATEGORY_QAM == eventCategory)
            {
                switch(event_type)
                {
                    case event_type::STATUS:
                    {
                        notification_payload * payload = static_cast <notification_payload *>(eventData.data);
                        if(nullptr != m_status_callback) //FIXME: protect against race?
                        {
                            m_status_callback(payload, m_callback_data);
                        }
                    }
                    break;
                    
                    case event_type::ERROR:
                    {
                        notification_payload * payload = static_cast <notification_payload *>(eventData.data);
                        if(nullptr != m_error_callback) //FIXME: protect against race?
                        {
                            m_error_callback(payload, m_callback_data);
                        }
                    }
                    break;
                    
                    case event_type::TERMINATE:
                        INFO("Received terminate event.\n");
                        run_loop = false;
                        break;
                    
                    default:
                        ERROR("Unhandled event type received.\n");
                }
            }
            rmf_osal_event_delete(event_handle);
	    } 
        INFO("%s thread exits.\n", __FUNCTION__);
        return;
    }

    void mediaplayer::setSourceEventHandler(std::unique_ptr <IRMFMediaEvents> handler)
    {
        m_source_event_handler = std::move(handler);
        m_source_element->addEventHandler(m_source_event_handler.get());
    }

    void mediaplayer::clearSourceEventHandler()
    {
        m_source_element->removeEventHandler(m_source_event_handler.get());
        m_source_event_handler.reset();
    }

    int mediaplayer::play()
    {
        int ret = 0;
        m_source_element->play();
        INFO("Playing.\n");
        return ret;
    }

    int mediaplayer::stop()
    {
        int ret = 0;
        m_source_element->stop();
        INFO("Stopping.\n");
        return ret;        
    }

    int mediaplayer::createSource()
    {
        int ret = 0;
        if(QAM == m_source_type)
        {
            //TODO: sanitize and validate incoming descriptor string.
            RMFQAMSrc* qamsrc = RMFQAMSrc::getQAMSourceInstance(m_service_descriptor.c_str());
            if(nullptr == qamsrc)
            {
                ERROR("Failed to create QAMSrc instance for %s.\n", m_service_descriptor.c_str());
                ret = -1;
            }
            else
            {
                m_source_element = static_cast <RMFMediaSourceBase *>(qamsrc);
            }
        }
        else
        {
            ERROR("Unsupported source type %d.\n", m_source_type);
            ret = -1;
        }
        return ret;
    }

    int mediaplayer::createSink()
    {
        m_sink_element = std::make_unique <MediaPlayerSink> ();
        return 0;
    }

    int mediaplayer::configureSource()
    {
        int ret = 0;
        m_source_configurer->setup(*this); //TODO: Check return and propagate error.
        return ret;
    }

    int mediaplayer::configureSink()
    {
        int ret = 0;
        m_sink_configurer->setup(*this); //TODO: Check return and propagate error.
        return ret;
    }

    int mediaplayer::destroySourceAndSink()
    {
        int ret = 0;
        m_source_configurer->undo_setup(*this);
        m_sink_configurer->undo_setup(*this);
        m_sink_element.reset(); //Destroy sink.
        if(QAM == m_source_type)
        {
            RMFQAMSrc::freeQAMSourceInstance(static_cast <RMFQAMSrc* > (m_source_element));
        }
        //TODO: Check return and propagate errors above
        return ret;        
    }

    int mediaplayer::mute(bool setMuted)
    {
        MediaPlayerSink * sink = dynamic_cast <MediaPlayerSink *> (m_sink_element.get());
        sink->setMuted(setMuted);
        return 0;
    }

    int mediaplayer::setVideoRectangle(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
    {
        int ret = 0;
        MediaPlayerSink * sink = dynamic_cast <MediaPlayerSink *> (m_sink_element.get());
        if(RMF_RESULT_SUCCESS != sink->setVideoRectangle(x, y, width, height))
        {
            ERROR("setVideoRectangle failed.\n");
            ret = -1;
        }
        return ret;
    }


    int mediaplayer::setVolume(float volume)
    {
        MediaPlayerSink * sink = dynamic_cast <MediaPlayerSink *> (m_sink_element.get());
        sink->setVolume(volume);
        return 0;
    }
}
