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
#define HN_SOURCE           "hnsource"
#define MEDIAPLAYER_SINK    "mediaplayersink"
#define DVR_SOURCE          "dvrsource"
#define DVR_SINK            "dvrsink"
#define HN_SINK             "hnsink"
#define QAM_SOURCE          "qamsource"
#define VOD_SOURCE          "vodsource"
#define DTV_SOURCE          "dtvsource"

#define MAX_PIPELINES 3
#define SKIPBACK_AMOUNT (5.0)
#define TSB_DURATION    60
#define DEFAULT_RECORD_DURATION 30

#define __DEBUG__


typedef enum {
            KEY_AVAILABLE = 0,
            KEY_TERMINATED
} test_event_type;

typedef struct launch_param
{
    std::string source;
    std::string sink;
    int _tsb;
    int duration;
    std::string recordingId;
    char *recordingTitle;
    std::string urls[50];
    int numOfURls;
} launch_param_t;



