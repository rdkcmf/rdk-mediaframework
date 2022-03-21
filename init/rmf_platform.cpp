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

#include <pthread.h>
#include <gst/gst.h>
#include <glib.h>
#include <string.h>

#include "rmf_platform.h"
#include "rmf_osal_types.h"
#include "rmf_osal_thread.h"
#include "rdk_debug.h"
#include "rmf_osal_init.h"

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

#define PRINT_ON_ERROR( fun, ret ) ret = fun; \
                                                    if(RMF_SUCCESS != ret) \
                                                    { \
                                                          RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.TARGET", "%s: %s returned 0x%x", __FUNCTION__, #fun, ret ); \
                                                    }

#define RETURN_ON_ERROR( fun, ret ) ret = fun; \
                                                    if(RMF_SUCCESS != ret) \
                                                    { \
                                                          RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.TARGET", "%s: Exiting as %s returned 0x%x", __FUNCTION__, #fun, ret ); \
                                                          return ret; \
                                                    }

static pthread_mutex_t singleton_mutex = PTHREAD_MUTEX_INITIALIZER;
bool rmfPlatform::rmf_framework_initialized = false;
rmfPlatform* rmfPlatform::m_rmfPlatform = NULL;

#ifndef USE_DIRECT_QAM
#ifdef USE_SOC_INIT
void soc_uninit();
void soc_init(int id, char *appName, int bInitSurface);
#endif
#endif

class rmfPlatformPrivate						
{
public:
	rmfPlatformPrivate();
	~rmfPlatformPrivate();
	rmf_Error run_gmainloop();
private:
	rmf_osal_ThreadId g_default_gmainloop_thId;
	void gmainloop_thread();
	static void run_thread( void * arg );
	static GMainLoop* g_default_gmainloop;
	//GMainContext rmf_element_context;
};

rmfPlatformPrivate::rmfPlatformPrivate()
{
}

rmfPlatformPrivate::~rmfPlatformPrivate()
{
}

GMainLoop* rmfPlatformPrivate::g_default_gmainloop = NULL;


void rmfPlatformPrivate::gmainloop_thread(  )
{
    g_main_loop_run( g_default_gmainloop );
    g_main_loop_unref( g_default_gmainloop );
    g_default_gmainloop = NULL;
}

void rmfPlatformPrivate::run_thread( void * arg  )
{
	rmfPlatformPrivate *ptr = ( rmfPlatformPrivate* )arg;
	ptr->gmainloop_thread();
}

rmf_Error rmfPlatformPrivate::run_gmainloop()
{
	rmf_Error ret = RMF_SUCCESS;

	/* Start a new thread for running GMainLoop */
	if( NULL == g_default_gmainloop )
	{
		g_default_gmainloop = g_main_loop_new( NULL/*&rmf_element_context*/, FALSE );

		ret = rmf_osal_threadCreate( 
			run_thread, this,
			RMF_OSAL_THREAD_PRIOR_DFLT, 
			RMF_OSAL_THREAD_STACK_SIZE, 
			&g_default_gmainloop_thId, 
			"run_default_gmainloop" );
		if ( RMF_SUCCESS != ret )
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.TARGET",
			"%s: rmf_osal_threadCreate failed 0x%x\n", __FUNCTION__, ret);
			return ret;	
		}
	}

	return ret;
}

rmfPlatform::rmfPlatform()
{
	m_rmfPlatformPrivate = new rmfPlatformPrivate();
	rmf_framework_initialized = false;
}

/* No way to be called, just put it for reference */
rmfPlatform::~rmfPlatform()
{
	delete m_rmfPlatformPrivate;
	m_rmfPlatformPrivate = NULL;
}


rmf_Error rmfPlatform::init(int argc, char * argv[])
{
	rmf_Error ret = RMF_SUCCESS;
	const char* envConfigFile = NULL;
	const char* debugConfigFile = NULL;
	int itr=0;
	
	if( isInitialized() )
	{
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TARGET", "rmfPlatform: RMF_PLATFORM_ALREADY_INITIALIZED\n");
		return RMF_PLATFORM_ALREADY_INITIALIZED;
	}

// The owner of the pipeline already does soc_init. So avoid soc_init in receiver with qamsrc
#ifndef USE_DIRECT_QAM
#ifdef USE_SOC_INIT
    soc_init(-1, "rmf_platform", 0);
#endif
#endif

	while (itr < argc)
	{
		if(strcmp(argv[itr],"--config")==0)
		{
			itr++;
			if (itr < argc)
			{
				envConfigFile = argv[itr];
			}
			else
			{
				break;
			}
		}
		if(strcmp(argv[itr],"--debugconfig")==0)
		{
			itr++;
			if (itr < argc)
			{
				debugConfigFile = argv[itr];
			}
			else
			{
				break;
			}
		}
		itr++;
	}
	//printf("%s:%d : envConfigFile= %s, debugConfigFile= %s\n", __FUNCTION__, __LINE__, envConfigFile, debugConfigFile);
	RETURN_ON_ERROR( rmf_osal_init( envConfigFile, debugConfigFile), ret );
#ifndef USE_DIRECT_QAM
	// Don't call gmain loop run for QAMSrc with Receiver context
	RETURN_ON_ERROR( m_rmfPlatformPrivate->run_gmainloop(), ret );
#endif

       rmf_framework_initialized = true;

	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TARGET", "rmfPlatform: Initialized successfully\n");

	return ret;

}

bool rmfPlatform::isInitialized()
{
	return rmf_framework_initialized;

}

rmf_Error rmfPlatform::uninit()
{
	rmf_Error ret = RMF_SUCCESS;

       if( !isInitialized() )
      	{
	   	return RMF_PLATFORM_NOT_INITIALIZED;
       }
// The owner of the pipeline already does soc_uninit. So avoid soc_init in receiver with qamsrc context
#ifndef USE_DIRECT_QAM
#ifdef USE_SOC_INIT
        soc_uninit();
#endif
#endif

	
	rmf_framework_initialized = false;

	return ret;
}

rmfPlatform* rmfPlatform::getInstance()
{
	pthread_mutex_lock (&singleton_mutex);
	if( NULL == m_rmfPlatform )
	{
		m_rmfPlatform = new rmfPlatform();
	}
	pthread_mutex_unlock (&singleton_mutex);
	return m_rmfPlatform;
}

