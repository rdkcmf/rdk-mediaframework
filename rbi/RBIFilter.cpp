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
#include "RBIFilter.h"
#include "rmfprivate.h"
#include "rbi.h"
#include <cassert>

//#define ENABLE_TRACE
#ifdef ENABLE_TRACE
#define TPRINTF(...) printf(__VA_ARGS__)
#else
#define TPRINTF(...)
#endif

#define FILTER_BIN getSource()
#define FILTER_IMPL static_cast<RBIFilterPrivate*>(getPrivateSourceImpl())

#define RETURN_UNLESS(x) RMF_RETURN_UNLESS(x, RMF_RESULT_INTERNAL_ERROR)

using namespace std;

pthread_mutex_t RBIFilter::m_filterMapMutex= PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
std::map<std::string,RBIFilter*> RBIFilter::m_filterMap;

static gboolean processPacketsIn( void* ctx, unsigned char* packets, int* len )
{
   gboolean queueBuffer;
   
   queueBuffer= ((RBIContext*)ctx)->processPackets( packets, len );
   
   return queueBuffer;
}

static int processPacketsOutSize( void* ctx )
{
   int size;
   
   size= ((RBIContext*)ctx)->getInsertionDataSize();
   
   return size;
}

static int processPacketsOutData( void* ctx, unsigned char* packets, int len )
{
   int insertedSize;
   
   insertedSize= ((RBIContext*)ctx)->getInsertionData( packets, len );
   
   return insertedSize;
}

class RBIFilterPrivate : public RMFMediaFilterPrivate
{
public:
    RBIFilterPrivate(RBIFilter* parent);
    void* createElement();

    RMFResult init();
    RMFResult term();
    RMFResult setSourceURI( const char *uri );
    RMFResult addReceiverId( const char *uri, const char *receiverId, int isLiveSrc);
    RMFResult removeReceiverId( const char *uri, const char *receiverId, int isLiveSrc);

private:
    bool m_initialized;
    RBIManager *m_rbm;
    RBIContext *m_ctx;    
};

RBIFilterPrivate::RBIFilterPrivate(RBIFilter* parent)
:   RMFMediaFilterPrivate(parent)
   , m_initialized(false), m_rbm(0), m_ctx(0)
{
}

RMFResult RBIFilterPrivate::init()
{
   RMFResult result;

   if ( !m_initialized )
   {
      result= RMFMediaFilterPrivate::init(false);
      if ( result == RMF_RESULT_SUCCESS )
      {
         m_initialized= true;
      }
   }
   else
   {
      result= RMF_RESULT_SUCCESS;
   }

   return result;
}

RMFResult RBIFilterPrivate::setSourceURI( const char *uri )
{
   RMFResult result;

   if ( m_ctx )
   {
      m_ctx->setSourceURI( uri );
      result= RMF_RESULT_SUCCESS;
   }
   else
   {
      result= RMF_RESULT_INTERNAL_ERROR;
   }
   
   return result;
}

RMFResult RBIFilterPrivate::addReceiverId( const char *uri, const char *receiverId, int isLiveSrc)
{
   RMFResult result;

   m_rbm= RBIManager::getInstance();
   if ( m_rbm )
   {
      m_rbm->addReceiverId( uri, receiverId, isLiveSrc);
      result= RMF_RESULT_SUCCESS;
   }
   else
   {
      result= RMF_RESULT_INTERNAL_ERROR;
   }

   return result;
}

RMFResult RBIFilterPrivate::removeReceiverId( const char *uri, const char *receiverId, int isLiveSrc)
{
   RMFResult result;

   m_rbm= RBIManager::getInstance();
   if ( m_rbm )
   {
      m_rbm->removeReceiverId( uri, receiverId, isLiveSrc);
      result= RMF_RESULT_SUCCESS;
   }
   else
   {
      result= RMF_RESULT_INTERNAL_ERROR;
   }

   return result;
}

void* RBIFilterPrivate::createElement()
{
    // Create an empty bin
    GstElement *bin= gst_bin_new("rbi_filter_bin");
    assert(bin);

    GstElement* el = gst_element_factory_make("rbifilter", "filter-rbi");
    assert( el );
    gst_bin_add(GST_BIN(bin), el);
    
    m_rbm= RBIManager::getInstance();
    if ( m_rbm )
    {
       m_ctx= m_rbm->getContext();
    }

    g_object_set(el, "rbi-packet-in-callback", processPacketsIn, NULL);
    g_object_set(el, "rbi-packet-out-size-callback", processPacketsOutSize, NULL);
    g_object_set(el, "rbi-packet-out-data-callback", processPacketsOutData, NULL);
    g_object_set(el, "rbi-context", m_ctx, NULL);
    
    // Add a src ghostpad to the bin.
    GstPad* pad = gst_element_get_static_pad(el, "src");
    gst_element_add_pad(bin, gst_ghost_pad_new("src", pad));
    gst_object_unref(GST_OBJECT(pad));
    
    // Add a sink ghostpad to the bin.
    pad = gst_element_get_static_pad(el, "sink");
    gst_element_add_pad(bin, gst_ghost_pad_new("sink", pad));
    gst_object_unref(GST_OBJECT(pad));

    INFO("RBIFilterPrivate::returning bin= %p filter element = %p\n", bin, el);
    return bin;
}

RMFResult RBIFilterPrivate::term()
{
    RMFResult result= RMF_RESULT_SUCCESS;
    
    if ( m_initialized )
    {
       m_initialized= false;

       if ( m_rbm )
       {
          if ( m_ctx )
          {
             m_rbm->releaseContext( m_ctx );
             m_ctx= 0;
          }
          m_rbm= 0;
       }
       
       result = RMFMediaFilterPrivate::term();
    }

    return result;
}

//-- Filter ------------------------------------------------------------------
RBIFilter* RBIFilter::getInstance(IRMFMediaSource *src, std::string srcLocator)
{
    RBIFilter *rbiFilter = 0;

    pthread_mutex_lock( &m_filterMapMutex );
    INFO("RBIFilter::getInstance: locator=%s", srcLocator.c_str() );

    // Check if a RBIFilter exists for the specified src locator
    map<std::string, RBIFilter*>::iterator it= m_filterMap.find( srcLocator );
    if ( it == m_filterMap.end() )
    {
        rbiFilter= new RBIFilter();
        INFO("RBIFilter::getInstance: locator=%s: creating new RBIFilter %p", srcLocator.c_str(), rbiFilter );

        if ( RMF_RESULT_SUCCESS == rbiFilter->init() )
        {
            // Add new RBIFilter to map
            m_filterMap.insert( pair<std::string, RBIFilter*>( srcLocator, rbiFilter ) );
            rbiFilter->m_srcLocator=srcLocator;
            rbiFilter->setSourceURI(srcLocator.c_str());
            rbiFilter->setSource(src);
        }
        else
        {
            INFO("RBIFilter::getInstance: Error: unable to create RBIFilter" );
            delete rbiFilter;
            rbiFilter = 0;
        }
    }
    else
    {
        rbiFilter= m_filterMap[srcLocator];
        RMFQAMSrc::getQAMSourceInstance( srcLocator.c_str(), RMFQAMSrc::USAGE_LIVE );
    }
    if ( rbiFilter )
    {
        rbiFilter->acquire();
    } 
    INFO("RBIFilter::getInstance: locator=%s return rbiFilter %p m_refCount %d", srcLocator.c_str(), rbiFilter, rbiFilter->print_refCount() );
    pthread_mutex_unlock( &m_filterMapMutex );

    return rbiFilter;
}

int RBIFilter::releaseInstance(RBIFilter *rbiFilter )
{
    if(rbiFilter)
    {
        pthread_mutex_lock( &m_filterMapMutex );
        INFO("RBIFilter::releaseInstance: RBIFilter %s release called for %p with refCount %d", rbiFilter->srcLocator().c_str(), rbiFilter, rbiFilter->print_refCount());

        if ( rbiFilter->m_refCount <= 0 )
        {
            assert( 0 );
            pthread_mutex_unlock( &m_filterMapMutex );
            return -1;
        }

        if ( rbiFilter->m_refCount > 0 )
        {
            --rbiFilter->m_refCount;
        }
        INFO("RBIFilter::releaseInstance: filter %p : refCount goes to %d", rbiFilter, rbiFilter->print_refCount() );

        if ( rbiFilter->m_refCount == 0 )
        {
            // Remove RBIFilter from mmap
            m_filterMap.erase( rbiFilter->srcLocator() );
            rbiFilter->setSource( NULL );
            rbiFilter->term();
            delete rbiFilter;
        }
        else if( rbiFilter->m_refCount > 0)
        {  
            RMFQAMSrc::freeQAMSourceInstance((RMFQAMSrc*)rbiFilter->m_source);
        }
        pthread_mutex_unlock( &m_filterMapMutex );
    }
    return 0;
}


RBIFilter::RBIFilter()
  : m_impl( 0 ), m_refCount(0)
{
}

RBIFilter::~RBIFilter()
{
}

RMFResult RBIFilter::init()
{
    RMFResult result;
    
    result= RMFMediaFilterBase::init();

    return result;
}

RMFResult RBIFilter::term()
{
    RMFResult result;

    result= RMFMediaFilterBase::term();

    return result;
}

RMFResult RBIFilter::setSourceURI( const char *uri )
{
   RMFResult result;

   if ( FILTER_IMPL )
   {
      result= FILTER_IMPL->setSourceURI( uri );
   }
   else
   {
      result= RMF_RESULT_NOT_INITIALIZED;
   }
   
   return result;
}

RMFResult RBIFilter::addReceiverId( const char *receiverId, int isLiveSrc)
{
   RMFResult result;

   if ( FILTER_IMPL )
   {
      result= FILTER_IMPL->addReceiverId( this->m_srcLocator.c_str(), receiverId, isLiveSrc);
   }
   else
   {
      result= RMF_RESULT_NOT_INITIALIZED;
   }

   return result;
}

RMFResult RBIFilter::removeReceiverId( const char *receiverId, int isLiveSrc)
{
   RMFResult result;

   if ( FILTER_IMPL )
   {
      result= FILTER_IMPL->removeReceiverId( this->m_srcLocator.c_str(), receiverId, isLiveSrc);
   }
   else
   {
      result= RMF_RESULT_NOT_INITIALIZED;
   }

   return result;
}

RMFResult RBIFilter::setSource(IRMFMediaSource* source)
{
    RMFResult result;

    pthread_mutex_lock( &m_filterMapMutex );

    result= RMFMediaFilterBase::setSource(source);
    if ( result == RMF_RESULT_SUCCESS )
    {
       m_source= source;
    }

    pthread_mutex_unlock( &m_filterMapMutex );

   return result;
}

RBIFilter::operator RMFQAMSrc*() const
{
   if ( FILTER_IMPL )
   {
      RMFQAMSrc *qamSrc= dynamic_cast<RMFQAMSrc *>(m_source);
      if ( qamSrc )
      {
         return qamSrc;
      }
   }
   return (RMFQAMSrc*)0;
}

RMFMediaFilterPrivate* RBIFilter::createPrivateFilterImpl()
{
   return new RBIFilterPrivate(this);
}

void* RBIFilter::createElement()
{
    return FILTER_IMPL->createElement();
}


