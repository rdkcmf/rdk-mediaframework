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


 
#ifndef _VL_DSG_OBJECT_PRINTER_H_
#define _VL_DSG_OBJECT_PRINTER_H_

#ifdef __cplusplus
extern "C" {
#endif
    
void vlPrintDsgInit();
    
void vlPrintDsgMode         (int nLevel, VL_DSG_MODE            * pDsgMode      );
void vlPrintDsgAdsgFilter   (int nLevel, VL_ADSG_FILTER         * pAdsgFilter   );
void vlPrintDsgClientId     (int nLevel, VL_DSG_CLIENT_ID       * pClientId     );
void vlPrintDsgHostEntry    (int nLevel, VL_DSG_HOST_ENTRY      * pHostEntry    );
void vlPrintDsgCardEntry    (int nLevel, VL_DSG_CARD_ENTRY      * pCardEntry    );
void vlPrintDsgDirectory    (int nLevel, VL_DSG_DIRECTORY       * pDirectory    );
void vlPrintDsgTunnelFilter (int nLevel, VL_DSG_TUNNEL_FILTER   * pTunnelFilter );
void vlPrintDsgAdvConfig    (int nLevel, VL_DSG_ADV_CONFIG      * pAdvConfig    );

void vlPrintIpCpe           (int nLevel, VL_DSG_IP_CPE          * pStruct       );
void vlPrintDsgClassifier   (int nLevel, VL_DSG_CLASSIFIER      * pStruct       );
void vlPrintDsgRule         (int nLevel, VL_DSG_RULE            * pStruct       );
void vlPrintDsgConfig       (int nLevel, VL_DSG_CONFIG          * pStruct       );
void vlPrintDsgDCD          (int nLevel, VL_DSG_DCD             * pStruct       );

#ifdef __cplusplus
}
#endif

#endif //_VL_DSG_OBJECT_PRINTER_H_

