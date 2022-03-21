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

#include "testsink.h"
#include "rmfprivate.h"
#include "rdk_debug.h"
#include "rmf_osal_thread.h"
#include "rmf_osal_sync.h"
#include <semaphore.h>

#include <cassert>
#include <gst/gst.h>

#include <gst/app/gstappsink.h>
#include <glib.h>

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

#define FILE_WRITE

#ifndef FILE_WRITE
#include "qamsrc_test_server.h"
#endif

static GHashTable * gst_buff_table = NULL;
static rmf_osal_Mutex mutex = NULL;

static void free_buffer_ptr(void *data);

static void new_buffer_available (GstElement* object,
                           gpointer arg);

int getDataEntry (void ** pdata, int * plen, void** freeData, void* userdata);
int freedataEntry (void * freeData, void* userdata);

//-- TestSinkPrivate ---------------------------------------------------

class TestSinkPrivate: public RMFMediaSinkPrivate
{
public:
	TestSinkPrivate(TestSink* parent);
	~TestSinkPrivate();
	void* createElement();

	// from RMFMediaSinkBase
	/*virtual*/ RMFResult setSource(IRMFMediaSource* source);
	void runEvtThread(  );
	static void eventThread( void * arg );
	int getData (void ** pdata, int * plen, void** freeData);
	int freedata (void * freeData);
#ifndef FILE_WRITE
	void registerEventQ(rmf_osal_eventqueue_handle_t eventqueue);
	void unRegisterEventQ(rmf_osal_eventqueue_handle_t eventqueue);
	rmf_osal_eventmanager_handle_t getEventManager();
	uint32_t getListenerCount();
#endif

private:
	GstElement* sink;
#ifndef FILE_WRITE
	gulong handler_id;
#endif
#ifdef FILE_WRITE
	FILE * appsink_file;
#else
	rmf_osal_eventmanager_handle_t eventmanager;
	rmf_osal_eventqueue_handle_t  eventqueue;
	uint32_t listeners;
#endif
};

TestSinkPrivate::TestSinkPrivate(TestSink* parent) :	 RMFMediaSinkPrivate(parent)
{
	sink = NULL;
#ifdef FILE_WRITE
	appsink_file = NULL;
#else
	rmf_Error ret;
	if (NULL == mutex)
	{
		ret = rmf_osal_mutexNew(&mutex);
		if (RMF_SUCCESS != ret)
		{
			RDK_LOG(RDK_LOG_FATAL, "LOG.RDK.TEST", "%s:Mutex create failed - %d \n", __FUNCTION__, ret);
		}
	}
	rmf_osal_mutexAcquire(mutex);
	if (NULL == gst_buff_table)
	{
		gst_buff_table = g_hash_table_new(NULL, NULL);
		if (NULL  == gst_buff_table)
		{
			RDK_LOG(RDK_LOG_FATAL, "LOG.RDK.TEST", 
			"%s - g_hash_table_new failed.\n", __FUNCTION__);
		}
	}
	rmf_osal_mutexRelease(mutex);
	ret = rmf_osal_eventmanager_create ( &eventmanager );
	if (RMF_SUCCESS != ret)
	{
		RDK_LOG(RDK_LOG_FATAL, "LOG.RDK.TEST", "%s:Event manager create failed - %d \n", __FUNCTION__, ret);
	}
	ret = rmf_osal_eventqueue_create((const uint8_t*)"EVTQ", &eventqueue);
	if(ret  != RMF_SUCCESS)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.TEST","%s rmf_osal_eventqueue_create failed(%d)\n",__FUNCTION__, ret);
	}
	listeners = 0;
	registerEventQ( eventqueue );
#endif
}

#ifndef FILE_WRITE
int TestSinkPrivate::getData (void ** pdata, int * plen, void** freeData)
{
	rmf_Error ret = RMF_SUCCESS;
	rmf_osal_event_handle_t event_handle;
	rmf_osal_event_params_t event_params;
	rmf_osal_event_category_t event_category;;
	uint32_t event_type;
	ret = rmf_osal_eventqueue_get_next_event_timed(  eventqueue,
				&event_handle, &event_category, &event_type,  &event_params, 5*1000*1000);
	if ( RMF_SUCCESS != ret)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.TEST", "%s: No events recieved\n",
				__FUNCTION__);
		return -1;
	}
	else if ( TEST_SESSION_TERMINATED == event_type)
	{
		RDK_LOG( RDK_LOG_WARN, "LOG.RDK.TEST", "%s:TEST_SESSION_TERMINATED events recieved\n",
				__FUNCTION__);
		rmf_osal_event_delete( event_handle);
		rmf_osal_eventqueue_delete( eventqueue );
		return -1;
	}
	else
	{
		*pdata = event_params.data;
		*plen = event_params.data_extension;
		*freeData= (void* )event_handle;
	}
	return 0;
}

int TestSinkPrivate::freedata (void * userdata)
{
	rmf_osal_event_handle_t event_handle  = (rmf_osal_event_handle_t)userdata;
	rmf_osal_event_delete( event_handle);
}

#endif

TestSinkPrivate::~TestSinkPrivate()
{
#ifdef FILE_WRITE
	if (appsink_file != NULL)
	{
		fclose (appsink_file);
	}
#else
	rmf_Error  ret;
	rmf_osal_event_handle_t event_handle;
	if (0 != listeners)
	{
		ret = rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_HN,
		                                                 TEST_SESSION_TERMINATED, NULL, 
		                                                 &event_handle );
		if (RMF_SUCCESS != ret)
		{
			RDK_LOG(RDK_LOG_FATAL, "LOG.RDK.TEST", "%s: event create failed - %d \n", __FUNCTION__, ret);
			return;
		}
		ret = rmf_osal_eventmanager_dispatch_event( eventmanager , event_handle );
		if (RMF_SUCCESS != ret)
		{
			RDK_LOG(RDK_LOG_FATAL, "LOG.RDK.TEST", "%s: event dispatch failed - %d \n", __FUNCTION__, ret);
			rmf_osal_event_delete( event_handle);
		}
	}
	unRegisterEventQ( eventqueue );
	rmf_osal_eventmanager_delete( eventmanager);
#endif
}

void* TestSinkPrivate::createElement()
{
	g_print("TestSinkPrivate::createElement enter\n");
	// Create a bin to contain the sink elements.
	GstElement* bin = gst_bin_new("hn_bin");

	// Add a appsink to the bin.
	sink = gst_element_factory_make("appsink", "sink-buffer");
	assert(sink != NULL);
#ifdef FILE_WRITE
	static int openCount = 0;
	char filename[20];

	snprintf( filename, 19, "/opt/received_spts%d.ts", openCount);
	filename[19] = '\0';
	appsink_file = fopen( filename, "w");
	if(!appsink_file){
		g_printerr ("Appsink file could not be created. Exiting.\n");
		return NULL;
	}

	openCount++;
	g_signal_connect(sink, "new-buffer", G_CALLBACK (new_buffer_available), 
		appsink_file);
#endif
	gst_app_sink_set_emit_signals ((GstAppSink*) sink, TRUE);
	gst_bin_add(GST_BIN(bin), sink);

	// Add a ghostpad to the bin so it can proxy to the demuxer.
	GstPad* pad = gst_element_get_static_pad(sink, "sink");
	gst_element_add_pad(bin, gst_ghost_pad_new("sink", pad));
	gst_object_unref(GST_OBJECT(pad));

	g_print("TestSinkPrivate::createElement exit\n");
	return bin;
}

// virtual
RMFResult TestSinkPrivate::setSource(IRMFMediaSource* source)
{
#ifndef FILE_WRITE
	if (NULL != source)
	{
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TEST", "%s: Connecting  new-buffer signal\n", __FUNCTION__);
		handler_id = g_signal_connect(sink, "new-buffer", G_CALLBACK (new_buffer_available), 
		(void*)this);
	}
	else
	{
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TEST", "%s: Disconnecting new-buffer signal\n", __FUNCTION__);
		g_signal_handler_disconnect (sink, handler_id);
	}
	if (NULL != source)
	{
		startServer(getDataEntry, freedataEntry,this) ;
	}
	else
	{
		rmf_Error  ret;
		rmf_osal_event_handle_t event_handle;
		ret = rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_HN,
		                                                 TEST_SESSION_TERMINATED, NULL, 
		                                                 &event_handle );
		if (RMF_SUCCESS != ret)
		{
			RDK_LOG(RDK_LOG_FATAL, "LOG.RDK.TEST", "%s: event create failed - %d \n", __FUNCTION__, ret);
		}
		ret = rmf_osal_eventmanager_dispatch_event( eventmanager , event_handle );
		if (RMF_SUCCESS != ret)
		{
			RDK_LOG(RDK_LOG_FATAL, "LOG.RDK.TEST", "%s: event dispatch failed - %d \n", __FUNCTION__, ret);
			rmf_osal_event_delete( event_handle);
		}
		stopServer();
	}
#endif
	return RMFMediaSinkPrivate::setSource(source);
}

#ifndef FILE_WRITE
void TestSinkPrivate::registerEventQ(rmf_osal_eventqueue_handle_t eventqueue)
{
	rmf_Error ret;
	ret = rmf_osal_eventmanager_register_handler(
			eventmanager,
			eventqueue,
			RMF_OSAL_EVENT_CATEGORY_HN);
	if (RMF_SUCCESS != ret)
	{
		RDK_LOG(RDK_LOG_FATAL, "LOG.RDK.TEST", "%s:Register events failed - %d \n", __FUNCTION__, ret);
	}
	else
	{
		listeners++;
	}
}

void TestSinkPrivate::unRegisterEventQ(rmf_osal_eventqueue_handle_t eventqueue)
{
	rmf_Error ret;
	ret = rmf_osal_eventmanager_unregister_handler(
			eventmanager,
			eventqueue
			);
	if (RMF_SUCCESS != ret)
	{
		RDK_LOG(RDK_LOG_FATAL, "LOG.RDK.TEST", "%s:Register events failed - %d \n", __FUNCTION__, ret);
	}
	else
	{
		listeners--;
	}
}
rmf_osal_eventmanager_handle_t TestSinkPrivate::getEventManager()
{
	return eventmanager;
}
uint32_t TestSinkPrivate::getListenerCount()
{
	return listeners;
}
#endif

static void free_buffer_ptr(void *data)
{
	rmf_osal_mutexAcquire(mutex);
	GstBuffer *gstbuffer = (GstBuffer*)g_hash_table_lookup (gst_buff_table, data);
	if (NULL == gstbuffer)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.TEST", "%s:g_hash_table_lookup returned NULL\n", __FUNCTION__);
	}
	else
	{
		g_hash_table_remove(gst_buff_table, data);
		gst_buffer_unref (gstbuffer);
		RDK_LOG(RDK_LOG_TRACE2, "LOG.RDK.TEST", "%s: Unreffed gst buffer\n", __FUNCTION__);
	}
	rmf_osal_mutexRelease(mutex);
}

static void new_buffer_available (GstElement* object,
                           gpointer arg)
{
	GstAppSink* app_sink = (GstAppSink*) object;
	GstBuffer * gstbuffer = gst_app_sink_pull_buffer(app_sink);
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TEST", "%s:%d httpStreaming obj =0x%x\n", __FUNCTION__, __LINE__, (unsigned int)object);
	if (NULL == gstbuffer)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.TEST", "%s:gst_app_sink_pull_buffer returned NULL\n", __FUNCTION__);
		return;
	}
#ifdef FILE_WRITE
	FILE* file = (FILE*) arg;
	if(fwrite (GST_BUFFER_DATA(gstbuffer), 1 , GST_BUFFER_SIZE(gstbuffer) , file) != GST_BUFFER_SIZE(gstbuffer)){
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.TEST", "%s:Error writing to file\n", __FUNCTION__);
	}
	else
	{
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TEST", "%s:Wrote %d bytes GstElement= 0x%x\n", __FUNCTION__, GST_BUFFER_SIZE(gstbuffer), (unsigned int)object);
	}
	gst_buffer_unref (gstbuffer);
#else

	rmf_osal_event_handle_t event_handle;
	rmf_osal_event_params_t event_params;
	TestSinkPrivate* hnimpl = (TestSinkPrivate*)arg;
	rmf_Error ret;

	if (hnimpl->getListenerCount())
	{
		event_params.data = GST_BUFFER_DATA(gstbuffer);
		event_params.data_extension =  GST_BUFFER_SIZE(gstbuffer);
		event_params.free_payload_fn = free_buffer_ptr;
		event_params.priority = 0;


		ret = rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_HN,
		                                                 TEST_BUFFER_AVAILABLE, &event_params, 
		                                                 &event_handle );
		if (RMF_SUCCESS != ret)
		{
			gst_buffer_unref (gstbuffer);
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.TEST", "%s:Event create failed - %d \n", __FUNCTION__, ret);
			return;
		}

		rmf_osal_mutexAcquire(mutex);
		g_hash_table_insert(gst_buff_table, event_params.data, gstbuffer);
		rmf_osal_mutexRelease(mutex);

		ret = rmf_osal_eventmanager_dispatch_event( hnimpl->getEventManager(), event_handle );
		if (RMF_SUCCESS != ret)
		{
			g_hash_table_remove(gst_buff_table, event_params.data);
			gst_buffer_unref (gstbuffer);
			rmf_osal_event_delete( event_handle);
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.TEST", "%s:Dispatch event failed - %d \n", __FUNCTION__, ret);
			return;
		}
	}
	else
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.TEST", "%s:  obj =0x%x 0 listeners\n", __FUNCTION__, (unsigned int)object);
		gst_buffer_unref (gstbuffer);
	}
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TEST", "%s:  obj =0x%x Exiting\n", __FUNCTION__, (unsigned int)object);
#endif
}

// virtual
void* TestSink::createElement()
{
	return static_cast<TestSinkPrivate*>(getPrivateSinkImpl())->createElement();
}

// virtual
RMFMediaSinkPrivate* TestSink::createPrivateSinkImpl()
{
	return new TestSinkPrivate(this);
}

void TestSink::registerEventQ(rmf_osal_eventqueue_handle_t eventqueue)
{
#ifndef FILE_WRITE
	static_cast<TestSinkPrivate*>(getPrivateSinkImpl())->registerEventQ(eventqueue);
#endif
}

void TestSink::unRegisterEventQ(rmf_osal_eventqueue_handle_t eventqueue)
{
#ifndef FILE_WRITE
	static_cast<TestSinkPrivate*>(getPrivateSinkImpl())->unRegisterEventQ(eventqueue);
#endif
}

#ifndef FILE_WRITE
int getDataEntry (void ** pdata, int * plen, void** freeData, void* userdata)
{
	TestSinkPrivate* pSink = (TestSinkPrivate* )userdata;
	pSink->getData( pdata,  plen, freeData);
}

int freedataEntry (void * freeData, void* userdata)
{
	TestSinkPrivate* pSink = (TestSinkPrivate* )userdata;
	pSink->freedata( freeData);
}
#endif

