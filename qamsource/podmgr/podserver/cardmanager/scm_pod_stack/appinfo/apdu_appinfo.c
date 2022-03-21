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



#include <string.h>
#include "poddef.h"
#include <lcsm_log.h>                   /* To support log file */

#include "utils.h"
//#include "module.h"
//#include "link.h"
#include "transport.h"
#include "session.h"
//#include "ci.h"
//#include "podlowapi.h"
#include "podhighapi.h"
#include "appmgr.h"
#include "applitest.h"
#include "global_event_map.h"
#include "appinfo-main.h"
#include <rdk_debug.h>

void vlTestMcardDiag(int num)
{
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," Entered vlTestMcardDiag: num:%d \n",num);
}

/**************************************************
 *    APDU Functions to handle APPLICATION INFO
 *************************************************/

/*
 *    APDU in: 0x9F, 0x80, 0x20, "application_info_req"
 */
int  APDU_AppInfoReq (USHORT sesnum, UCHAR * apkt, USHORT len)                 
{
    MDEBUG ( DPM_APPINFO, ERROR_COLOR "ai:Called -- STUBBED \n" RESET_COLOR);

    return 1;
}

/*
 *    APDU in: 0x9F, 0x80, 0x21, "application_info_cnf"
 */
int  APDU_AppInfoCnf (USHORT sesnum, UCHAR * apkt, USHORT len)                 
{
    MDEBUG ( DPM_APPINFO, ERROR_COLOR "ai:Called -- STUBBED \n" RESET_COLOR);

    return 1;
}

/*
 *    APDU in: 0x9F, 0x80, 0x22, "server_query"
 */
int  APDU_ServerQuery (USHORT sesnum, UCHAR * apkt, USHORT len)                         
{
    MDEBUG ( DPM_APPINFO, ERROR_COLOR "ai:Called -- STUBBED \n" RESET_COLOR);
    aiRcvServerQuery (apkt, len);
    return 1;
}

/*
 *    APDU in: 0x9F, 0x80, 0x23, "server_reply"
 */
int  APDU_ServerReply (USHORT sesnum, UCHAR * apkt, USHORT len)                             
{
    MDEBUG ( DPM_APPINFO, ERROR_COLOR "ai:Called -- STUBBED \n" RESET_COLOR);

    return 1;
}

