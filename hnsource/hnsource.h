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


/**
 * @file hnsource.h
 * @brief Home networking source.
 * Basically an HTTP source
 * with DLNA trickplay support.
 */

/**
 * @defgroup HNSource HN Source
 * <b>HNSource is responsible for the following operations. </b>
 * <ul>
 * <li> HN Source connects the media devices in the home network using the http protocol layer with a Virtual address/URI.
 * <li> In the case of an encrypted media stream the appropriate decryption (DTCP or any DRM)
 * technique are be applied before play out.
 * <li> As any other source the output of HN Source will be a clear SPTS. It uses http source gstreamer plugin.
 * </ul>
 * @ingroup RMF
 * @defgroup hnsrcapi HN Source API list
 * Describe the details about HN Source API specifications.
 * @ingroup HNSource
 */

#ifndef HN_SOURCE_H
#define HN_SOURCE_H

#include "rmfbase.h"

#include <list>
#include <string>


static const RMFResult RMF_RESULT_HNSRC_GENERIC_ERROR       = 0x1001;
static const RMFResult RMF_RESULT_HNSRC_FORMAT_ERROR        = 0x1002;
static const RMFResult RMF_RESULT_HNSRC_LEGACY_CA_ERROR     = 0x1003;
static const RMFResult RMF_RESULT_HNSRC_NETWORK_ERROR       = 0x1004;
static const RMFResult RMF_RESULT_HNSRC_VOD_PAUSE_TIMEOUT   = 0x1005;
static const RMFResult RMF_RESULT_HNSRC_TRAILER_ERROR       = 0x1006;
static const RMFResult RMF_RESULT_HNSRC_DTCP_CONN_ERROR     = 0x1007;
static const RMFResult RMF_RESULT_HNSRC_VOD_TUNE_TIMEOUT    = 0x1008;

/**
 * @defgroup hnsrcclass HN Source Class
 * @ingroup HNSource
 * @class HNSource
 * @brief This class contains public member functions for HN Source.
 * @ingroup hnsrcclass
 */
class HNSource : public RMFMediaSourceBase
{
public:
    typedef std::list<std::pair<float, float> > range_list_t;

    HNSource();
    ~HNSource();

    unsigned getFileSize() const;
    RMFResult getBufferedRanges(range_list_t& ranges);

    // from IRMFMediaSource
    /*virtual*/ RMFResult open(const char* url, char* mimetype);
    /*virtual*/ RMFResult term();
    /*virtual*/ RMFResult getMediaTime(double& time);
    /*virtual*/ RMFResult setMediaTime(double time);
    /*virtual*/ RMFResult setSpeed(float speed);
    /*virtual*/ RMFResult close();
    /*virtual*/ RMFResult playAtLivePosition(double length);
    /*virtual*/ RMFResult setVideoLength (double length);
    /*virtual*/ RMFResult setLowBitRateMode (bool isYes);

protected:
    /*virtual*/ void* createElement();
    /*virtual*/ RMFMediaSourcePrivate* createPrivateSourceImpl();
};

#endif // HN_SOURCE_H
