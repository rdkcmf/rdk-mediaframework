#ifndef __LIBMEDIAPLAYER_H__
#define __LIBMEDIAPLAYER_H__
#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <cstdint>
#include <thread>

#include "rmf_error.h"
#include "rmfcore.h"
#include "rmfbase.h"



namespace libmediaplayer
{
    struct notification_payload
    {
        const std::string m_source;
        long long m_code;
        const std::string m_title;
        const std::string m_message;

        notification_payload(const std::string &source, int code, const std::string &title, const std::string &message):
            m_source(source), m_code(code), m_title(title), m_message(message)
        {

        }

    };

    typedef void (*event_callback)(notification_payload * payload, void * data);

    typedef enum
    {
        QAM = 0,
        HNSRC,
        INVALID
    } source_type_t;

    typedef uint32_t rmf_osal_eventqueue_handle_t; //Avoid exposing rmf_osal_* to application and forward declare type locally.
    class mediaplayer;
    
    class element_config_helper // Helps set up RMF elements in accordance with their specialization.
    {
        public:
        element_config_helper(){}
        virtual ~element_config_helper(){}
        //platform_*() calls do not need to be repeated between tune requests.
        virtual int platform_init() = 0;
        virtual int platform_deinit() = 0;
        //Below APIs are expected to be invoked once for every single tune request.
        virtual int setup(mediaplayer &) = 0;
        virtual int undo_setup(mediaplayer &) = 0;
    };

    class qam_source_config_helper : public element_config_helper
    {
        public:
        qam_source_config_helper();
        virtual ~qam_source_config_helper();
        virtual int platform_init() override;
        virtual int platform_deinit() override;
        virtual int setup(mediaplayer &) override;
        virtual int undo_setup(mediaplayer &) override;
    };

    class mp_sink_config_helper : public element_config_helper
    {
        public:
        mp_sink_config_helper();
        virtual ~mp_sink_config_helper();
        virtual int platform_init() override;
        virtual int platform_deinit() override;
        virtual int setup(mediaplayer &) override;
        virtual int undo_setup(mediaplayer &) override;
    };

    class mediaplayer
    {
        public:
        static int initialize(source_type_t type, bool initLogger, bool createMainLoop);
        static int deinitialize(source_type_t type);
    
        //static player_id_t createMediaPlayer(std::string &service_descriptor);
        static mediaplayer * createMediaPlayer(source_type_t source_type, const std::string &service_descriptor); //TODO: memory management using smart pointers?
        mediaplayer(source_type_t source_type, const std::string &service_descriptor);
        ~mediaplayer();
        int play();
        int stop();
        int mute(bool mute);
        int setVideoRectangle(unsigned int x, unsigned int y, unsigned int width, unsigned int height);
        int setVolume(float volume);
        const std::string & getServiceDescriptor() const {return m_service_descriptor;}
        int changeService(std::string &service_descriptor);

        RMFMediaSinkBase *getSink() const {return m_sink_element.get();}
        void registerEventCallbacks(event_callback status_cb, event_callback error_cb, void * data);
        void enqueueEvent(unsigned int type, notification_payload * payload);
        void setSourceEventHandler(std::unique_ptr <IRMFMediaEvents> handler);
        void clearSourceEventHandler();

        
        private:
        int createSource();
        int createSink();
        int configureSource();
        int configureSink();
        int destroySourceAndSink();
        void eventDispatcher();
    
        //Static data.
        static bool m_create_mainloop;

        //Per-instance data;
        source_type_t m_source_type;
        std::string m_service_descriptor;
        std::mutex m_player_mutex;
        std::unique_ptr <element_config_helper> m_source_configurer;
        std::unique_ptr <element_config_helper> m_sink_configurer;
        RMFMediaSourceBase * m_source_element;
        std::unique_ptr <RMFMediaSinkBase> m_sink_element;
        rmf_osal_eventqueue_handle_t m_event_queue;
        std::thread m_dispatch_thread;
        event_callback m_status_callback;
        event_callback m_error_callback;
        void * m_callback_data;
        std::unique_ptr <IRMFMediaEvents> m_source_event_handler;
    };
    
}

#endif //__LIBMEDIAPLAYER_H__