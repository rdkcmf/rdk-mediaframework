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
#ifndef RMFRBIFILTER_H
#define RMFRBIFILTER_H

#include "rmfbase.h"
#include "rmfqamsrc.h"

#ifdef GCC4_XXX
#include <map>
#include <vector>
#include <string>
#else
#include <map.h>
#include <vector.h>
#include <string.h>
#endif

class RBIFilterPrivate;

class RBIFilter : public RMFMediaFilterBase
{
public:
    RBIFilter();
    virtual ~RBIFilter();
   
    RMFResult init();
    RMFResult term();
    
    RMFResult setSourceURI( const char *uri );

    RMFResult setSource(IRMFMediaSource* source);

    RMFResult addReceiverId( const char *receiverId, int isLiveSrc=1, const char *recordingId=NULL);

    RMFResult removeReceiverId( const char *receiverId, int isLiveSrc=1, const char *recordingId=NULL);

    virtual operator RMFQAMSrc*() const;
    static RBIFilter *getInstance( IRMFMediaSource *src, std::string locator );
    int acquire()
    {
        return ++m_refCount;
    }

    int print_refCount()
    {
        return m_refCount;
    }

    std::string srcLocator()
    {
        return m_srcLocator;
    }
    static int releaseInstance( RBIFilter *rbiFilter);

protected:
    RMFMediaFilterPrivate* createPrivateFilterImpl();
    void* createElement();

private:
    RBIFilterPrivate* m_impl;
    IRMFMediaSource* m_source;

    std::string m_srcLocator;
    static pthread_mutex_t m_filterMapMutex;
    static std::map<std::string,RBIFilter*> m_filterMap;
    int m_refCount;
};

#endif // RMFRBIFILTER_H
