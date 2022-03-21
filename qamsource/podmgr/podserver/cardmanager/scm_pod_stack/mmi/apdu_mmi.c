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

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************
 *    APDU Functions to handle MMI
 *************************************************/

/*
 *    APDU in: 0x9F, 0x88, 0x20, "open_mmi_req"
 */
int  APDU_OpenMMIReq (USHORT sesnum, UCHAR * apkt, USHORT len)                             
{

    return 1;
}

/*
 *    APDU in: 0x9F, 0x88, 0x21, "open_mmi_cnf"
 */
int  APDU_OpenMMICnf (USHORT sesnum, UCHAR * apkt, USHORT len)                             
{

    return 1;
}

/*
 *    APDU in: 0x9F, 0x88, 0x22, "close_mmi_req"
 */
int  APDU_CloseMMIReq (USHORT sesnum, UCHAR * apkt, USHORT len)                       
{

    return 1;
}

/*
 *    APDU in: 0x9F, 0x88, 0x23, "close_mmi_cnf"
 */
int  APDU_CloseMMICnf (USHORT sesnum, UCHAR * apkt, USHORT len)                       
{

    return 1;
}

#ifdef __cplusplus
}
#endif
