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
#ifndef RMFSMOOTHSOURCE_H
#define RMFSMOOTHSOURCE_H

#include "rmfbase.h"

class SmoothVideoPlayer;
class SmoothVideoEvents;

class SmoothSource : public RMFMediaSourceBase
{
public:
    SmoothSource();
    ~SmoothSource();
    RMFResult init();
    RMFResult term();
    RMFResult open(const char* uri, char* mimetype);
    RMFResult close();
    RMFResult play();
    RMFResult pause();
    RMFResult getTrickPlayCaps(RMFTrickPlayCaps& caps);
    RMFResult setSpeed(float speed);
    RMFResult getMediaTime(double& t);
    RMFResult setMediaTime(double t);
    RMFResult getMediaInfo(RMFMediaInfo& info);
    RMFResult waitForEOS();
protected:
private:
    void* createElement();
    void createPlayer();
    void destroyPlayer();

    SmoothVideoPlayer* m_player;
    SmoothVideoEvents* m_smoothEvents;
};

#endif

/* Notes
    Replace C style callbacks on IRMFMediaSource with C++ events class
    Replace start/stop Streaming with play/pause/resume which appears on most interfaces already
    why do setEOSCallback and setErroCallback have void return type
    add  gst_version to rmfbase
    we need to ENFORCE the rmfbase init function is called.  I don't think is should be virtual.
*/
