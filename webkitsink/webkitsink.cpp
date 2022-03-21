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
#ifndef NPAPI_VDIEO_PLAYER
#include "config.h"
#endif

#include "webkitsink.h"

#ifndef NPAPI_VDIEO_PLAYER
#include "FloatRect.h"
#include "GraphicsContext.h"
#include "IntRect.h"
#include "Logging.h"
#else
#include <QMatrix>
#endif

#include <gst/gst.h>

#include <cassert>

#if defined(Q_WS_QWS)
#include <QtGui/qscreen_qws.h> // for qt_screen
#endif

WebkitSink::WebkitSink()
:   m_haveEverStartedPlay(false)
{
}

RMFResult WebkitSink::init()
{
    RMFResult res = MediaPlayerSink::init();
    return res;
}

void WebkitSink::setRepaintCallback(repaint_callback_t, void*)
{
    // NOP
}

void WebkitSink::setHaveVideoCallback(generic_callback_t cb, void* data)
{
    return MediaPlayerSink::setHaveVideoCallback(cb, data);
}

void WebkitSink::setHaveAudioCallback(generic_callback_t cb, void* data)
{
    return MediaPlayerSink::setHaveAudioCallback(cb, data);
}

void WebkitSink::setVolumeChangedCallback(generic_callback_t, void*)
{
    //return IMPL->setVolumeChangedCallback(cb, data);
}

void WebkitSink::setMuteChangedCallback(generic_callback_t, void*)
{
    //return IMPL->setMuteChangedCallback(cb, data);
}

void WebkitSink::paint(WebCore::GraphicsContext* context, const WebCore::IntRect& rect)
{
#ifdef NPAPI_VDIEO_PLAYER
    (void) context;
    (void) rect;
#else
    int x, y, w, h;
    // Convert rect to absolute coordinates by adding the QGraphicsWebView origin.
    const QMatrix& matrix = static_cast<QPainter*>(context->platformContext())->matrix();
    x= rect.x() + matrix.dx();
    y= rect.y() + matrix.dy();
    w= rect.width();
    h= rect.height();
    if ( x < 0 ) x= 0;
    if ( y < 0 ) y= 0;
    MediaPlayerSink::setVideoRectangle(x, y, w, h, true);

    using namespace WebCore;
    if (m_haveEverStartedPlay) {
        context->clearRect( FloatRect(rect) );
    } else {
        context->fillRect( FloatRect(rect), Color( 0, 0, 0, 0xFF ), ColorSpaceDeviceRGB );
    }
#endif
}

void WebkitSink::paint(QPainter* painter, const QRectF& rect)
{
#ifdef NPAPI_VDIEO_PLAYER
    // Scale the video to fit into the plugin rect.
    {
        int x, y, w, h;
        // Convert rect to absolute coordinates by adding the QGraphicsWebView origin.
        x= rect.x();
        y= rect.y();
        w= rect.width();
        h= rect.height();
        if ( x < 0 ) x= 0;
        if ( y < 0 ) y= 0;
        //g_print("webkitsink: Scaling video to %dx%d+%d+%d\n", w, h, x, y);
        //MediaPlayerSink::setVideoRectangle(x, y, w, h, true);
    }

    // Punch hole in the GUI plane to make video visible.
    QRectF punch_rect(rect);
    const QMatrix& matrix = painter->matrix();
    punch_rect.translate(-matrix.dx(), -matrix.dy());
    g_print("webkitsink: Punching hole: %dx%d+%d+%d\n",
        (int)punch_rect.width(), (int)punch_rect.height(), (int)punch_rect.x(), (int)punch_rect.y());
    QPainter::CompositionMode currentCompositionMode = painter->compositionMode();
    painter->setCompositionMode(QPainter::CompositionMode_Source);
    painter->fillRect(punch_rect, Qt::transparent);
    painter->setCompositionMode(currentCompositionMode);
#else
    (void) painter;
    (void) rect;
#endif
}

void WebkitSink::setMuted(bool muted)
{
    return MediaPlayerSink::setMuted(muted);
}

bool WebkitSink::getMuted()
{
    return MediaPlayerSink::getMuted();
}

void WebkitSink::setVolume(float volume)
{
    return MediaPlayerSink::setVolume(volume);
}

float WebkitSink::getVolume()
{
    return MediaPlayerSink::getVolume();
}

void WebkitSink::onPlay()
{
    m_haveEverStartedPlay = true;
}

long WebkitSink::getVideoDecoderHandle() const
{
    return MediaPlayerSink::getVideoDecoderHandle();
}

void WebkitSink::setAudioLanguage (const char* pAudioLang)
{
    MediaPlayerSink::setAudioLanguage (pAudioLang);
    return;
}

