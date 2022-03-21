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

#ifndef RMFCORE_H
#define RMFCORE_H

// rmfcore.h

// Added because the IRMFMediaEvents class
// has been extended with a new caStatus() method
#define RMF_EVENT_CA_STATUS_METHOD

#define RMF_EVENT_SECTION_METHOD

// RMFResult
// 0 - Success
// 1 - General Error
// ... Others
typedef unsigned long RMFResult;

struct RMFTrickPlayCaps {};
struct RMFMediaInfo
{
	float m_duration;       // duration in seconds
	double m_startTime;     // start time in seconds in UTC
};

struct RMFStreamingStatus 
{
	unsigned long m_status;   //Status defined by rmf elements.
};

static const RMFResult RMF_RESULT_SUCCESS = 0;
static const RMFResult RMF_RESULT_FAILURE = 1;
static const RMFResult RMF_RESULT_NOT_IMPLEMENTED = 2;
static const RMFResult RMF_RESULT_NOT_INITIALIZED = 3;
static const RMFResult RMF_RESULT_INTERNAL_ERROR = 4;
static const RMFResult RMF_RESULT_INVALID_ARGUMENT = 5;
static const RMFResult RMF_RESULT_NO_SINK = 6;
static const RMFResult RMF_RESULT_NO_PERMISSION = 7;

typedef enum {
  RMF_STATE_VOID_PENDING        = 0, // no pending state
  RMF_STATE_NULL                = 1, // the NULL state or initial state of an element
  RMF_STATE_READY               = 2, // the element is ready to go to PAUSED
  RMF_STATE_PAUSED              = 3, // the element is PAUSED
  RMF_STATE_PLAYING             = 4  // the element is PLAYING
} RMFState;

typedef enum {
  RMF_STATE_CHANGE_FAILURE             = 0, // the state change failed
  RMF_STATE_CHANGE_SUCCESS             = 1, // the state change succeeded
  RMF_STATE_CHANGE_ASYNC               = 2, // the state change will happen asynchronously
  RMF_STATE_CHANGE_NO_PREROLL          = 3  // the state change succeeded but the element
                                            // cannot produce data in PAUSED. This typically
                                            // happens with live sources.
} RMFStateChangeReturn;

typedef void (*cci_event_cb_t)(void* arg);

class IRMFMediaSink;
class RMFMediaSourcePrivate;
class RMFMediaSinkPrivate;
class RMFMediaFilterPrivate;

//------------------------------
class IRMFMediaEvents
{
public:
    virtual ~IRMFMediaEvents() {}

    virtual void playing(){}
    virtual void paused(){}
    virtual void stopped(){}
    virtual void complete(){}//this is end of stream
    virtual void progress(int position, int duration){(void)position; (void)duration;}
    virtual void status(const RMFStreamingStatus& status){(void)status;}
    virtual void error(RMFResult err, const char* msg){(void)err; (void) msg;}
    virtual void caStatus(const void* data_ptr, unsigned int data_size){}
    virtual void section(const void* data_ptr, unsigned int data_size){}
};

class IRMFMediaSource
{
public:
    virtual ~IRMFMediaSource() {}

    // Lifetime Management
    virtual RMFResult init() = 0;
    virtual RMFResult term() = 0;

    // Content Acquisition
    virtual RMFResult open(const char* uri, char* mimetype) = 0;
    virtual RMFResult close() = 0;//might rename to stop

    // Streaming Control
    // Assumes open has been called

    virtual RMFResult play() = 0;
    virtual RMFResult play(float& speed, double& time) = 0;
    virtual RMFResult pause() = 0;
    virtual RMFResult stop() = 0;
    virtual RMFResult frameSkip(int n) = 0;
    //virtual RMFResult startStreaming() = 0;
    //virtual RMFResult stopStreaming() = 0;

    // TrickPlay
    virtual RMFResult getTrickPlayCaps(RMFTrickPlayCaps& caps) = 0;
    virtual RMFResult getSpeed(float& speed) = 0;
    virtual RMFResult setSpeed(float speed) = 0;

    // MediaTime
    virtual RMFResult getMediaTime(double& t) = 0;//might remove 'Media' as its implied
    virtual RMFResult setMediaTime(double t) = 0;//might remove 'Media' as its implied or might rename to seek

    // MediaInfo
    // duration, starttime etc...
    virtual RMFResult getMediaInfo(RMFMediaInfo& info) = 0;//might remove 'Media' as its implied

    // Events
    // Can unset by passing NULL
    virtual RMFResult waitForEOS() = 0;
    virtual RMFResult setEvents(IRMFMediaEvents* events) = 0;
    virtual RMFResult addEventHandler(IRMFMediaEvents* events) = 0;
    virtual RMFResult removeEventHandler(IRMFMediaEvents* events) = 0;
    // Misc
    virtual RMFStateChangeReturn getState(RMFState* current, RMFState* pending) = 0;

    virtual RMFMediaSourcePrivate* getPrivateSourceImpl() const = 0;

};

//------------------------------

class IRMFMediaSink
{
public:
    virtual ~IRMFMediaSink() {}

    // Lifetime Management
    virtual RMFResult init() = 0;
    virtual RMFResult term() = 0;

    // Source
    virtual RMFResult setSource(IRMFMediaSource* source) = 0;

    virtual RMFResult setEvents(IRMFMediaEvents* events) = 0;
    virtual RMFResult addEventHandler(IRMFMediaEvents* events) = 0;
    virtual RMFResult removeEventHandler(IRMFMediaEvents* events) = 0;
    virtual RMFMediaSinkPrivate* getPrivateSinkImpl() const = 0;
};

class IRMFMediaFilter : public IRMFMediaSource, public IRMFMediaSink
{
};


#endif // RMFCORE_H
