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


#include "rmf_osal_mem.h"
#include "rmf_osal_event.h"
#include "rmf_osal_util.h"
#include "rmf_osal_storage.h"
#include "rdk_debug.h"
#include "rmf_osal_file.h"
#include "rmf_osal_socket.h"
#include "rmf_osal_init.h"
#include "rdk_debug.h"

rmf_Error rmf_osal_init( const char* envConfigFile, const char* debugConfigFile)
{
	rmf_Error ret;

	rmf_osal_memInit();
	rmf_osal_envInit();

	if (NULL == envConfigFile)
	{
		envConfigFile = ENV_CONF_FILE;
	}

	ret = rmf_osal_env_add_conf_file(envConfigFile);
	if ( RMF_SUCCESS != ret)
	{
		printf("%s:%d Adding env config file %s failed\n", __FUNCTION__, __LINE__, ENV_CONF_FILE);
		return ret;
	}
#if 0
	if (NULL == debugConfigFile)
	{
		debugConfigFile = DEBUG_CONF_FILE;
	}

	ret = rmf_osal_env_add_conf_file(debugConfigFile);
	if ( RMF_SUCCESS != ret)
	{
		printf("%s:%d Adding debug config file %s failed\n", __FUNCTION__, __LINE__, DEBUG_CONF_FILE);
		return ret;
	}

	rmf_dbgInit();
#endif
	rdk_logger_init(debugConfigFile);

	ret = rmf_osal_event_init();
	if ( RMF_SUCCESS != ret)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.OS", "%s:%d rmf_osal_event_init file failed\n", __FUNCTION__, __LINE__);
		return ret;
	}

	rmf_osal_storageInit();
	rmf_osal_fileInit();
	rmf_osal_socketInit();

	return RMF_SUCCESS;
}

