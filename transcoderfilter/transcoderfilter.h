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
#ifndef RMFTRANSCODERFILTER_H
#define RMFTRANSCODERFILTER_H

#include "rmfbase.h"

/*
Target device settings
Determines output resolution and aspect ratio
*/
typedef enum
{
	TRANSCODER_TARGET_IPHONE_16_9,
	TRANSCODER_TARGET_IPHONE_4_3,
	TRANSCODER_TARGET_IPAD_16_9,
	TRANSCODER_TARGET_IPAD_4_3,
	TRANSCODER_TARGET_1080p,
	TRANSCODER_TARGET_720p,
	TRANSCODER_TARGET_480p,
	TRANSCODER_TARGET_1080i,
	TRANSCODER_TARGET_480i,
}TRANSCODER_TARGET_t;

/*
Quality settings
Determines rate control parameters and overall audio quality
*/
typedef enum
{
	TRANSCODER_QUALITY_HIGH,
	TRANSCODER_QUALITY_MEDIUM,
	TRANSCODER_QUALITY_LOW,
} TRANSCODER_QUALITY_t;


typedef struct _TranscoderProperties 
{
    unsigned int max_bitrate;
    unsigned int target_bitrate;
//    unsigned int max_size_buffers;
//    char rectangle[32];
	TRANSCODER_TARGET_t target_device;
    TRANSCODER_QUALITY_t codec_profile;
    unsigned int codec_level;
} TranscoderProperties;


class TranscoderFilterPrivate;

class TranscoderFilter : public RMFMediaFilterBase
{
public:
    TranscoderFilter();
    virtual ~TranscoderFilter();
   
    RMFResult init();
    RMFResult term();
	void setProperties(TranscoderProperties *prop);

protected:
	RMFMediaFilterPrivate* createPrivateFilterImpl();
    void* createElement();

private:
    TranscoderFilterPrivate* m_impl;
};

#endif // RMFTRANSCODERFILTER_H
