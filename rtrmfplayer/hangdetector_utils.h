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

#ifndef HANGDETECTOR_UTILS_H
#define HANGDETECTOR_UTILS_H

#include <glib.h>
#include "logger.h"

#include <thread>
#include <atomic>
#include <sys/types.h>
#include <unistd.h>

namespace Utils
{

class HangDetector
{
    std::unique_ptr<std::thread> m_thread;
    std::atomic_bool m_running {false};
    std::atomic_int  m_resetCount {0};
    std::atomic_int m_threshold {30};
    guint m_timerSource {0};

    void runWatchDog()
    {
        while(m_running)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (m_resetCount > m_threshold)
            {
                m_running = false;
                printf("rtrmfplayer: hang detected, sending SIGFPE\n");
                fflush(stdout);
                kill(getpid(), SIGFPE);

                std::this_thread::sleep_for(std::chrono::seconds(m_threshold));

                printf("rtrmfplayer: hang detected, sending SIGKILL\n");
                fflush(stdout);
                kill(getpid(), SIGKILL);
            }
            else if (m_resetCount > (m_threshold / 2))
            {
                printf("rtrmfplayer: hang detector will force crash in %d seconds...\n", m_threshold - m_resetCount);
                fflush(stdout);
            }
            ++m_resetCount;
        }
    }

    void resetWatchDog()
    {
        /* The RT_TIMEOUT is 15s; when Client is hung and we couldnt send, we will timeout after 15s;
        * During that time it will be considered as hang but the main thread is free from this and playing fine.
        * For such incidents, lets Keep the warning after 16s
        */
        if (m_resetCount > 16)
            LOG_WARNING("rtrmfplayer: HangDetector detected Recovery from Hang. The m_resetCount is reset to 0 from %d\n", (m_resetCount - 0));

        m_resetCount = 0;
    }

    void initThreshold()
    {
        const char* var = getenv("RTRMFPLAYER_HANG_DETECTOR_THRESHOLD_SEC");
        if (var)
        {
            try
            {
                int tmp = std::atoi(var);
                if (tmp > 5)
                {
                    m_threshold = tmp;
                }
            } catch (...) {}
        }
    }

public:
    ~HangDetector()
    {
        stop();
    }

    void start()
    {
        if (m_thread)
            return;
        initThreshold();

        m_running = true;

        gint priority = getenv("RTRMFPLAYER_HANG_DETECTOR_PRIORITY_DEFAULT") ? G_PRIORITY_DEFAULT : G_PRIORITY_HIGH;
        m_timerSource = g_timeout_add_seconds_full
        (
            priority,
            1, // Timeout in seconds
            [](gpointer data) -> gboolean
            {
                static_cast<HangDetector*>(data)->resetWatchDog();
                return G_SOURCE_CONTINUE;
            },
            this,
            nullptr
        );

        m_thread.reset(new std::thread(&HangDetector::runWatchDog, this));
    }

    void stop()
    {
        m_running = false;
        g_source_remove(m_timerSource);
        m_timerSource = 0;
        m_thread->join();
        m_thread.reset(nullptr);
    }
};

} // namespace utils

#endif // HANGDETECTOR_H
