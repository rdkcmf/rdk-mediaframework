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
#ifndef MEDIADISCOVER_H
#define MEDIADISCOVER_H

#include <gst/pbutils/pbutils.h>
#include <vector>
#include <string>

class MediaDiscover;

namespace MediaDiscoverCallbacks
{
    static void on_discovered_cb (GstDiscoverer *discoverer, GstDiscovererInfo *info, GError *err, MediaDiscover *data);
    static void on_finished_cb (GstDiscoverer *discoverer, MediaDiscover *data);
}

class MediaDiscover
{
public:
    typedef std::vector<std::string> TStringList;

    MediaDiscover();
    ~MediaDiscover();

    void start(const char* uri);

    TStringList getAvailableAudioLanguages();

protected:

private:
    void release_stream_info_list();

    GstDiscoverer *m_discoverer;

public:

    std::string m_uri;
    GList* m_streaminfolist;
};

#endif // MEDIADISCOVER_H
