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


/*
 * Note: this file originally auto-generated by mib2c using
 *        : mib2c.scalar.conf 11805 2005-01-07 09:37:18Z dts12 $
 */
#ifndef OCSTBHOSTCONTENTERRORSUMMARYINFO_H
#define OCSTBHOSTCONTENTERRORSUMMARYINFO_H

/*
 * function declarations 
 */
void            init_ocStbHostContentErrorSummaryInfo(void);
Netsnmp_Node_Handler handle_ocStbHostPatTimeoutCount;
Netsnmp_Node_Handler handle_ocStbHostPmtTimeoutCount;
Netsnmp_Node_Handler handle_ocStbHostOobCarouselTimeoutCount;
Netsnmp_Node_Handler handle_ocStbHostInbandCarouselTimeoutCount;

#endif                          /* OCSTBHOSTCONTENTERRORSUMMARYINFO_H */
