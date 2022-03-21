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




#ifdef __cplusplus
extern "C" {
#endif

void sendFlowRequest( long int handle, int servicetype, long int pid, long int returnQueue ); 
void sendIPFlowRequest( long int handle, int servicetype, unsigned char *apduBuffer, long int returnQueue ); 
#ifdef USE_IPDIRECT_UNICAST
void sendIPDUFlowRequest( long int handle, int servicetype, unsigned char *apduBuffer, long int returnQueue ); // IPDU register flow
#endif // USE_IPDIRECT_UNICAST
void sendFlowConfirm( unsigned char *ptr, int len);

void sendDeleteFlowRequest( long int handle, long int flowId, long int returnQueue );
void deleteFlowConfirm( unsigned char *ptr );

void sendLostFlow( unsigned char *ptr );
void sendLostFlowConfirm( long int flowId );

void initializeSession1( unsigned sessionNum );


#ifdef __cplusplus
}
#endif


