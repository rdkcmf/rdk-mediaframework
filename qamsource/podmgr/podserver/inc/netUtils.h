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



/*-----------------------------------------------------------------*/
#ifndef __VL_NET_UTILS_H__
#define __VL_NET_UTILS_H__

/*-----------------------------------------------------------------*/

#define VL_IPV4_ADDRESS_LONG(a,b,c,d)   ((((a)&0xFF)<<24)|(((b)&0xFF)<<16)|(((c)&0xFF)<<8)|(((d)&0xFF)<<0))

/*-----------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif
    
void vlNetAddFlags(int sock, const char * pszIfName, int nFlags);
void vlNetRemoveFlags(int sock, const char * pszIfName, int nFlags);
void vlNetSetMacAddress(int sock, const char * pszIfName, unsigned char * pMacAddress);
void vlNetSetIpAddress(int sock, const char * pszIfName, unsigned char * pIpAddress);
void vlNetSetSubnetMask(int sock, const char * pszIfName, unsigned char * pSubnetMask);
int  vlNetSetNetworkRoute(unsigned long ipNet, unsigned long ipMask, unsigned long ipGateway, char * pszIfName);
int  vlNetDelNetworkRoute(unsigned long ipNet, unsigned long ipMask, unsigned long ipGateway, char * pszIfName);

#ifdef __cplusplus
}
#endif

/*-----------------------------------------------------------------*/
#endif // __VL_NET_UTILS_H__
/*-----------------------------------------------------------------*/
