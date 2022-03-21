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



#ifndef _VL_DSG_PARSER_H_
#define _VL_DSG_PARSER_H_

#ifdef __cplusplus
extern "C" {
#endif

void vlParseDsgInit(); // required to force link-in

void vlParseDsgMode      (VL_BYTE_STREAM * pParseBuf, VL_DSG_MODE       * pDsgMode   );
void vlParseDsgAdsgFilter(VL_BYTE_STREAM * pParseBuf, VL_ADSG_FILTER    * pAdsgFilter);
void vlParseDsgClientId  (VL_BYTE_STREAM * pParseBuf, VL_DSG_CLIENT_ID  * pClientId  );
void vlParseDsgHostEntry (VL_BYTE_STREAM * pParseBuf, VL_DSG_HOST_ENTRY * pHostEntry );
void vlParseDsgCardEntry (VL_BYTE_STREAM * pParseBuf, VL_DSG_CARD_ENTRY * pCardEntry );
void vlParseDsgDirectory (VL_BYTE_STREAM * pParseBuf, VL_DSG_DIRECTORY  * pDirectory );
void vlParseDsgTunnelFilter(VL_BYTE_STREAM * pParseBuf, VL_DSG_TUNNEL_FILTER * pTunnelFilter);
void vlParseDsgAdvConfig (VL_BYTE_STREAM * pParseBuf, VL_DSG_ADV_CONFIG * pAdvConfig );

void vlParseDsgDCD(int nOrgLength, unsigned char * pOrgBuf,
                   VL_BYTE_STREAM * pParseBuf, VL_DSG_DCD * pStruct);

#ifdef __cplusplus
}
#endif

#endif //_VL_DSG_PARSER_H_

