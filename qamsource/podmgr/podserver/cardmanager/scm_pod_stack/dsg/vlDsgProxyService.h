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



#ifndef _VL_DSG_PROXY_SERVICE_H_
#define _VL_DSG_PROXY_SERVICE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ip_types.h"
#include "dsg_types.h"

typedef enum _VL_DSG_PROXY_RESULT
{
    VL_DSG_PROXY_RESULT_SUCCESS = 0,
    VL_DSG_PROXY_RESULT_FAILED  = 1,
    
} VL_DSG_PROXY_RESULT;

VL_DSG_PROXY_RESULT vlDsgProxyServer_Start();
VL_DSG_PROXY_RESULT vlDsgProxyServer_Register();
VL_DSG_PROXY_RESULT vlDsgProxyServer_Deregister(const char * pszReason);
VL_DSG_PROXY_RESULT vlDsgProxyServer_UpdateCableSystemParams();
VL_DSG_PROXY_RESULT vlDsgProxyServer_UpdateDcd(int nBytes, const unsigned char * pData);
VL_DSG_PROXY_RESULT vlDsgProxyServer_UpdateAdvConfig(const unsigned char * pData, int nBytes);
VL_DSG_PROXY_RESULT vlDsgProxyServer_UpdateDsgDirectory(const unsigned char * pData, int nBytes);
VL_DSG_PROXY_RESULT vlDsgProxyServer_UpdateDocsisParams();
VL_DSG_PROXY_RESULT vlDsgProxyServer_UpdateTlv217Params(const unsigned char * pData, int nBytes);
void vlDsgProxyServer_updateConf(const char * file, const char * str);
int vlDsgProxyServer_CheckIfClientConnectedToServer();
void vlDsgProxyServer_sendMcastData(VL_DSG_CLIENT_ID_ENC_TYPE encType, unsigned long nPort, unsigned char * pData, int nBytes);

#ifdef __cplusplus
}
#endif

#endif // _VL_DSG_PROXY_SERVICE_H_
