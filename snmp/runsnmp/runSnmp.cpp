/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2011 RDK Management
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

#include <rmf_osal_init.h>
#include <stdio.h>
#include <unistd.h>
#include "sys_init.h"
#include "snmpmanager.h"
#include "bufParseUtilities.h"
#include "rmf_error.h"

#ifdef USE_POD_IPC
#include <rmf_osal_ipc.h>
#include <podServer.h>
#include <ipcclient_impl.h>
#endif
#if USE_SYSRES_MLT
#include "rpl_malloc.h"
#include "rpl_new.h"
#endif
#ifdef GLI
#include "libIBus.h"
#include "sysMgr.h"
#endif

#ifdef INCLUDE_BREAKPAD
#include "breakpad_wrapper.h"
#endif

static SnmpManager *vlg_pSnmpManager = NULL;

static void vl_snmp_manager_init_task(void* arg)
{
    static int bInitialized = 0;
    if(0 == bInitialized)
    {
        bInitialized = 1;
        if(NULL == vlg_pSnmpManager)
        {
            vlg_pSnmpManager = new SnmpManager;

	        if(NULL != vlg_pSnmpManager)
            {
                vlg_pSnmpManager->initialize();
                rmf_osal_threadSleep(10000,0);
                vlg_pSnmpManager->init_complete();
            }
        }
    }
}

rmf_Error vlMpeosSnmpManagerInit()
{
	rmf_osal_ThreadId threadid = 0;
	rmf_osal_threadCreate(vl_snmp_manager_init_task, NULL , RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE,& threadid, "vl_snmp_manager_init_task");

	return RMF_SUCCESS;
}

int main(int argc, char * argv[])
{
	rmf_Error ret;
	const char* envConfigFile = NULL;
	const char* debugConfigFile = NULL;
	int itr=0;
	uint32_t Handle;
#ifdef INCLUDE_BREAKPAD
	breakpad_ExceptionHandler();
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

//	printf("%s:%d : envConfigFile= %s, debugConfigFile= %s\n", __FUNCTION__, __LINE__, envConfigFile, debugConfigFile);
	ret = rmf_osal_init( envConfigFile, debugConfigFile);
	if( RMF_SUCCESS != ret)
	{
		printf("runSnmp : rmf_osal_init() failed, ret = %x\n", ret);
	}

#ifdef GLI
	IARM_Result_t iResult = IARM_RESULT_SUCCESS;
	iResult = IARM_Bus_Init( "runSnmpClient" );
	if(IARM_RESULT_SUCCESS != iResult)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP","%s: Error initialing IARM bus()... error code : %d\n", __FUNCTION__, iResult);
    }

	iResult = IARM_Bus_Connect();
	if(IARM_RESULT_SUCCESS != iResult)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP","%s: Error initialing IARM bus()... error code : %d\n", __FUNCTION__, iResult);
    }
#endif

#ifdef USE_POD_IPC
	printf("%s : %s : %d : Starting to Listen for POD events in SNMP module\n", __FILE__, __FUNCTION__, __LINE__);
	/* Start listening for the pod events*/
	rmf_snmpmgrIPCInit();
	
	while(0 == getPodClientHandle(&Handle))
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP", "%s : %s : %d : POD_IS_DOWN\n", __FILE__, __FUNCTION__, __LINE__);	
		sleep(1);
	}
#endif

	rmf_Error result = RMF_SUCCESS;
	{
	
		if ( RMF_SUCCESS != ( result = vlMpeosSnmpManagerInit(  ) ) )
		{
		        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SYS","vlMpeosSnmpManagerInit() returned %d\n", result );
		}
	}

    while(1)
    {
	sleep(5000);
    }
	
#ifdef USE_POD_IPC
	printf("%s : %s : %d : Stopped to Listen for POD events in SNMP module\n", __FILE__, __FUNCTION__, __LINE__);
  	/* Stop listening for the pod events*/
  	rmf_snmpmgrIPCUnInit();
#endif
#ifdef GLI
    IARM_Bus_Disconnect();
    IARM_Bus_Term();
#endif

return 0;
}

