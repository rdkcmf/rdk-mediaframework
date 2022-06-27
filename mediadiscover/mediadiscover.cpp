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
/*Based on sample code which is  Copyright (C) GStreamer developers
Licensed under the MIT License*/

#include "mediadiscover.h"

void MediaDiscoverCallbacks::on_discovered_cb (GstDiscoverer *discoverer, GstDiscovererInfo *info, GError *err, MediaDiscover *md)
{
    GstDiscovererResult result;
    const gchar *uri;

    uri = gst_discoverer_info_get_uri (info);
    result = gst_discoverer_info_get_result (info);
    switch (result)
    {
      case GST_DISCOVERER_URI_INVALID:
        g_print ("Invalid URI '%s'\n", uri);
        break;
      case GST_DISCOVERER_ERROR:
        g_print ("Discoverer error: %s\n", err->message);
        break;
      case GST_DISCOVERER_TIMEOUT:
        g_print ("Timeout\n");
        break;
      case GST_DISCOVERER_BUSY:
        g_print ("Busy\n");
        break;
      case GST_DISCOVERER_MISSING_PLUGINS:{
        const GstStructure *s;
        gchar *str;

        s = gst_discoverer_info_get_misc (info);
        str = gst_structure_to_string (s);

        g_print ("Missing plugins: %s\n", str);
        g_free (str);
        break;
      }
      case GST_DISCOVERER_OK:
        g_print ("Discovered '%s'\n", uri);
        break;
    }

    if (result != GST_DISCOVERER_OK)
    {
        g_printerr ("This URI cannot be played\n");
        return;
    }

    md->m_streaminfolist = gst_discoverer_info_get_stream_list (info);
}

void MediaDiscoverCallbacks::on_finished_cb (GstDiscoverer *discoverer, MediaDiscover *md)
{
    g_print ("Finished discovering\n");
    gst_discoverer_stop(discoverer);
}

void MediaDiscover::release_stream_info_list()
{
    if(m_streaminfolist)
    {
        gst_discoverer_stream_info_list_free (m_streaminfolist);
        m_streaminfolist = 0;
    }
}


MediaDiscover::MediaDiscover()
: m_discoverer(0)
, m_streaminfolist(0)
{
    GError *err = NULL;
    m_discoverer = gst_discoverer_new (3 * GST_SECOND, &err);
    if (!m_discoverer)
    {
        g_print ("Error creating discoverer instance: %s\n", err->message);
        g_clear_error (&err);
    }
    else
    {
        g_signal_connect (m_discoverer, "discovered", G_CALLBACK (MediaDiscoverCallbacks::on_discovered_cb), this);
        g_signal_connect (m_discoverer, "finished", G_CALLBACK (MediaDiscoverCallbacks::on_finished_cb), this);
    }
}

MediaDiscover::~MediaDiscover()
{
    release_stream_info_list();
    g_object_unref (m_discoverer);
}

void MediaDiscover::start(const char* uri)
{
    if(m_uri == std::string(uri))
    {
        return;
    }

    release_stream_info_list();

    gst_discoverer_start (m_discoverer);

    if (!gst_discoverer_discover_uri_async (m_discoverer, uri))
    {
        g_print ("Failed to start discovering URI '%s'\n", uri);
    }
}

MediaDiscover::TStringList MediaDiscover::getAvailableAudioLanguages()
{
    TStringList langs;
    if(!m_streaminfolist)
    {
        return langs;
    }

    GList* current = m_streaminfolist;
    while(current)
    {
        GstDiscovererStreamInfo *si = (GstDiscovererStreamInfo *)(current->data);
        const GstTagList *tags = gst_discoverer_stream_info_get_tags (si);

        GValue val = { 0, };
        gchar *str;
        if(gst_tag_list_copy_value (&val, tags, GST_TAG_LANGUAGE_CODE))
        {
            if (G_VALUE_HOLDS_STRING (&val))
            {
                str = g_value_dup_string (&val);
            }
            else
            {
                str = gst_value_serialize (&val);
            }

            langs.push_back(str);
            g_free (str);
            g_value_unset (&val);
        }
        current = current->next;
    }
}
