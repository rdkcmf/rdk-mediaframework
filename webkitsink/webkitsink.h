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
#ifndef MEDIA_SINK_WEBKIT_H
#define MEDIA_SINK_WEBKIT_H

#include "mediaplayersink.h"

namespace WebCore {
    class GraphicsContext;
    class IntRect;
}

class QPainter;
class QRectF;

class WebkitSink : public MediaPlayerSink 
{
public:
    typedef void (*generic_callback_t)(void* data);
    typedef void (*repaint_callback_t)(void* buffer, void* data);

    void setRepaintCallback(repaint_callback_t cb, void* data);
    void setHaveVideoCallback(generic_callback_t cb, void* data);
    void setHaveAudioCallback(generic_callback_t cb, void* data);
    void setVolumeChangedCallback(generic_callback_t cb, void* data);
    void setMuteChangedCallback(generic_callback_t cb, void* data);
    WebkitSink();

    // from RMFBase
    RMFResult init();
    void paint(WebCore::GraphicsContext* context, const WebCore::IntRect& rect);
    void paint(QPainter* painter, const QRectF& rect);
    void setMuted(bool muted);
    bool getMuted();
    void setVolume(float volume);
    float getVolume();
    void onPlay();
    long getVideoDecoderHandle() const;
    void setAudioLanguage (const char* pAudioLang);

private:
    bool m_haveEverStartedPlay;
};

#endif //MEDIA_SINK_WEBKIT_H 
