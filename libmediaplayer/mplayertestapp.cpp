#include "libmediaplayer.h"
#include "libmediaplayer_utils.h"
#include <unistd.h>

using namespace libmediaplayer;

void app_event_callback(notification_payload * payload, void * data)
{
    INFO("Event received: Source: %s, code: %lld, title: %s, message: %s\n",
        payload->m_source.c_str(), payload->m_code, payload->m_title.c_str(), payload->m_message.c_str());
}

void app_error_callback(notification_payload * payload, void * data)
{
    INFO("Error received: Source: %s, code: %lld, title: %s, message: %s\n",
        payload->m_source.c_str(), payload->m_code, payload->m_title.c_str(), payload->m_message.c_str());
}

int main(int argc, const char * argv[])
{
    INFO("mplayertestapp starts.\n");
    mediaplayer::initialize(QAM, true, true);
    
    for(int playlist_index = 1; playlist_index < argc; playlist_index++)
    {
        mediaplayer * playerptr = mediaplayer::createMediaPlayer(QAM, argv[playlist_index]);
        playerptr->registerEventCallbacks(app_event_callback, app_error_callback, nullptr);
        playerptr->play();

        sleep(10);
        playerptr->stop();
        delete playerptr;
    }
    mediaplayer::deinitialize(QAM);
    INFO("mplayertestapp exits.\n");
    return 0;
}
