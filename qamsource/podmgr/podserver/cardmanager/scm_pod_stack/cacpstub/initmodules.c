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

 

#include <rdk_debug.h>
#include <rmf_osal_util.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string>
#ifdef __cplusplus
extern "C" {
#endif
rmf_Error vlMpeosPopulateDhcpOptions();
rmf_Error vlMpeosSysInit(  );
int vlhal_oobtuner_Capable(  );
unsigned char vl_isFeatureEnabled( unsigned char *featureString );
#ifdef __cplusplus
}
#endif
using namespace std;


rmf_Error vlMpeosPopulateDhcpOptions()
{
}

rmf_Error vlMpeosHalInit(  )
{
}

rmf_Error vlMpeosSysInit(  )
{
}

int vlg_util_get_platform_name_val( void )
{
}

char *vlg_util_get_platform_name( void )
{
	return NULL;
}

int vl_util_is_parker_device( void )
{
}

int vl_util_does_not_have_oob_tuner( void )
{
	return 0;
}

int vlhal_oobtuner_Capable(  )
{
	return 0;
}


char *vlg_util_get_ecm_rpc_if_name( void )
{
	return NULL;
}

char *vlg_util_get_docsis_wan_if_name( void )
{
	return NULL;
}

char *vlg_util_get_docsis_dhcp_if_name( void )
{
	return NULL;
}
unsigned char vl_isFeatureEnabled( unsigned char *featureString )
{
	return 0;
}

int vl_GetEnvValue( unsigned char *featureString )
{
	const char *iniValue;
	int iniIntValue = 0;

	iniValue = rmf_osal_envGet( ( const char * ) featureString );
	if ( iniValue != NULL )
	{
		iniIntValue = atoi( iniValue );
	}

	return iniIntValue;
}

float vl_GetEnvValue_float( unsigned char *featureString )
{
	const char *iniValue;
	float iniFloatValue = 0;

	iniValue = rmf_osal_envGet( ( const char * ) featureString );
	if ( iniValue != NULL )
	{
		iniFloatValue = atof( iniValue );
	}

	return iniFloatValue;
}

char *VL_mpeosImpl_getCreateDate( char *value )
{
	return NULL;
}

char *VL_mpeosImpl_getVersionId( char *value )
{
	return NULL;
}

char *VL_mpeosImpl_getSerialNumber( char *value )
{
	return NULL;
}

