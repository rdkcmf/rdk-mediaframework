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
#ifndef MEDIA_PLAYER_SINK_H
#define MEDIA_PLAYER_SINK_H

#include "rmfbase.h"
#include <string>

/**
 * X1 video player sink.
 */
class MediaPlayerSink: virtual public RMFMediaSinkBase
{
public:
    RMFResult setVideoRectangle(unsigned x, unsigned y, unsigned w, unsigned h, bool apply_now = false);

    // from RMFMediaSinkBase
    /*virtual*/ void onSpeedChange(float new_speed);
    /*virtual*/ void* createElement();

    typedef void (*callback_t)(void* data);

    void setMuted(bool muted);
    bool getMuted();
    void setVolume(float volume);
    float getVolume();
    RMFResult getMediaTime(double& t);

protected:
    void setHaveVideoCallback(callback_t cb, void* data);
    void setHaveAudioCallback(callback_t cb, void* data);
    void setVolumeChangedCallback(callback_t cb, void* data);
    void setMuteChangedCallback(callback_t cb, void* data);

    long getVideoDecoderHandle() const;
    // from RMFMediaSinkBase
    /*virtual*/ RMFMediaSinkPrivate* createPrivateSinkImpl();
};

#endif // MEDIA_PLAYER_SINK_H
