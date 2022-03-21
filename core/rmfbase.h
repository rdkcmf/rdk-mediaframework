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

#ifndef RMFBASE_H
#define RMFBASE_H

#include "rmfcore.h"
//  Remove application-specific code from RMF core!
#ifdef RMF_STREAMER
#include "rdk_debug.h"
#include "rmf_error.h"
void RMF_registerGstElementDbgModule(char *gst_module,  char *rmf_module);
#endif

class RMFMediaSinkBase;
class RMFMediaFilterBase;

//------------------------------------------------------------------------------
// Base class for RMF sources.
// Takes care of attached sinks management.
// Usage:
// Implement createElement() to return a gstreamer source element or a bin.

class RMFMediaSourceBase : public IRMFMediaSource
{
public:
    RMFMediaSourceBase();
    virtual ~RMFMediaSourceBase();
    RMFResult init();
    RMFResult term();
    RMFResult open(const char* uri, char* mimetype);
    RMFResult close();
    RMFResult play();
    RMFResult play(float& speed, double& time);
    RMFResult pause();
    RMFResult stop();
    RMFResult frameSkip(int n);
    RMFResult getTrickPlayCaps(RMFTrickPlayCaps& caps);
    RMFResult getSpeed(float& speed);
    RMFResult setSpeed(float speed);
    RMFResult getMediaTime(double& t);
    RMFResult setMediaTime(double t);
    RMFResult getMediaInfo(RMFMediaInfo& info);
    RMFResult waitForEOS();
    RMFResult setEvents(IRMFMediaEvents* events);
    RMFResult addEventHandler(IRMFMediaEvents* events);
    RMFResult removeEventHandler(IRMFMediaEvents* events);
    RMFStateChangeReturn getState(RMFState* current, RMFState* pending);
    RMFMediaSourcePrivate* getPrivateSourceImpl() const;

protected:
    virtual RMFResult addSink(RMFMediaSinkBase* sink);
    virtual RMFResult removeSink(RMFMediaSinkBase* sink);
    virtual void* createElement() = 0;

    // Override to create a derived implementation.
    virtual RMFMediaSourcePrivate* createPrivateSourceImpl();

private:
    RMFMediaSourcePrivate* m_pimpl;
    friend class RMFMediaSinkPrivate;
    friend class RMFMediaSourcePrivate;
    friend class RMFMediaFilterBase;
};

//------------------------------------------------------------------------------
// Base class for RMF sinks.
// Usage:
// Implement createElement() to return a gstreamer sink element or a bin.
class RMFMediaSinkBase: public IRMFMediaSink
{
public:
    RMFMediaSinkBase();
    virtual ~RMFMediaSinkBase();

    // IRMFMediaSink
    RMFResult init();
    RMFResult term();
    RMFResult setSource(IRMFMediaSource* source);
    RMFResult setEvents(IRMFMediaEvents* events);
    RMFResult addEventHandler(IRMFMediaEvents* events);
    RMFResult removeEventHandler(IRMFMediaEvents* events);
    RMFMediaSinkPrivate* getPrivateSinkImpl() const;
    RMFMediaSinkPrivate* getPrivateSinkImplUnchecked() const;

protected:
    virtual void* createElement() = 0;

    virtual RMFMediaSinkPrivate* createPrivateSinkImpl(); // override to create a derived implementation
    virtual void onSpeedChange(float /*new_speed*/) {} // called by the source

private:
    RMFMediaSinkPrivate* m_pimpl;
    friend class RMFMediaSinkPrivate;
    friend class RMFMediaSourcePrivate;
    friend class RMFMediaFilterBase;
};

//------------------------------------------------------------------------------
// Base class for RMF filters.
// Usage:
// Implement createElement() to return a gstreamer sink element or a bin.
class RMFMediaFilterBase : public RMFMediaSourceBase, public RMFMediaSinkBase
{
public:
    RMFMediaFilterBase();
    virtual ~RMFMediaFilterBase();
   
    RMFResult init();
    RMFResult term();

    RMFResult getSpeed(float& speed);
    RMFResult setSpeed(float speed);
    RMFResult getMediaTime(double& time);
    RMFResult setMediaTime(double time);
    RMFResult getMediaInfo(RMFMediaInfo& info);

protected:
    virtual RMFResult addSink(RMFMediaSinkBase* sink);
    virtual RMFResult removeSink(RMFMediaSinkBase* sink);
    
    RMFMediaSourcePrivate* createPrivateSourceImpl();
    RMFMediaSinkPrivate* createPrivateSinkImpl();
    
    virtual void* createElement() = 0;
    virtual RMFMediaFilterPrivate* createPrivateFilterImpl() = 0;

private:
    RMFMediaFilterPrivate* m_impl;
    friend class RMFMediaFilterPrivate;
};


#endif // RMFFILESINK_H
