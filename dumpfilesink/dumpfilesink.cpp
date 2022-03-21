/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2013 RDK Management
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

#include "dumpfilesink.h"
#include "rmfprivate.h"

#include <cassert>
#include <gst/gst.h>
#include <stdlib.h>
#include <string.h>

//-- DFSinkPrivate ---------------------------------------------------

class DumpFileSinkPrivate: public RMFMediaSinkPrivate
{
public:
	DumpFileSinkPrivate(DumpFileSink* parent);
	~DumpFileSinkPrivate();
	void* createElement();

	// from RMFMediaSinkBase
	/*virtual*/ RMFResult setSource(IRMFMediaSource* source);
	RMFResult setLocation(char *location);

private:
	DumpFileSink* m_parent;
	GstElement* m_filesink;
};

DumpFileSinkPrivate::DumpFileSinkPrivate(DumpFileSink* parent) :	 RMFMediaSinkPrivate(parent)
{
	m_parent = parent;
	m_filesink = NULL;
}

DumpFileSinkPrivate::~DumpFileSinkPrivate()
{
}

void* DumpFileSinkPrivate::createElement()
{

	// Create a bin to contain the sink elements.
	GstElement* bin = gst_element_factory_make("bin", NULL); // a unique name will be generated

	//RDK_LOG(RDK_LOG_INFO, "LOG.RDK.HN", "creating elements\n");
	g_print( "creating gst elements\n");

	m_filesink = gst_element_factory_make("filesink", "file-sink");

	GstPad *pad1 = NULL;
	gst_bin_add_many(GST_BIN(bin), m_filesink, NULL);

	// Add a ghostpad to the bin so it can proxy to the demuxer.
	pad1 = gst_element_get_static_pad(m_filesink, "sink");

	gst_element_add_pad(bin, gst_ghost_pad_new("sink", pad1));
	gst_object_unref(GST_OBJECT(pad1));

	return bin;
}


RMFResult DumpFileSinkPrivate::setLocation(char *location)
{
	g_print("DumpFileSinkPrivate::setLocation ... '%s'\n", location);
	g_object_set (G_OBJECT (m_filesink), "location", location, NULL);

	return RMF_RESULT_SUCCESS;
}

// virtual
RMFResult DumpFileSinkPrivate::setSource(IRMFMediaSource* source)
{
	return RMFMediaSinkPrivate::setSource(source);
}


// virtual
void* DumpFileSink::createElement()
{
	return static_cast<DumpFileSinkPrivate*>(getPrivateSinkImpl())->createElement();
}

// virtual
RMFMediaSinkPrivate* DumpFileSink::createPrivateSinkImpl()
{
	return new DumpFileSinkPrivate(this);
}

void DumpFileSink::setLocation(char* location)
{
	static_cast<DumpFileSinkPrivate*>(getPrivateSinkImpl())->setLocation(location);
}
