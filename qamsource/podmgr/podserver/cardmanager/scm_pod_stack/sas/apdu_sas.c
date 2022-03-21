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


/**************************************************
 *    APDU Functions to handle SAS
 *************************************************/

/*
 *    APDU in: 0x9F, 0x9A, 0x00, "sas_connect_rqst"
 */
int  APDU_SASConnectReq (USHORT sesnum, UCHAR * apkt, USHORT len)                   
{

    return 1;
}

/*
 *    APDU in: 0x9F, 0x9A, 0x01, "sas_connect_cnf"
 */
int  APDU_SASConnectCnf (USHORT sesnum, UCHAR * apkt, USHORT len)                  
{

    return 1;
}

/*
 *    APDU in: 0x9F, 0x9A, 0x02, "sas_data_rqst"
 */
int  APDU_SASDataReq (USHORT sesnum, UCHAR * apkt, USHORT len)                          
{

    return 1;
}

/*
 *    APDU in: 0x9F, 0x9A, 0x03, "sas_data_av"
 */
int  APDU_SASDataAv (USHORT sesnum, UCHAR * apkt, USHORT len)                          
{

    return 1;
}

/*
 *    APDU in: 0x9F, 0x9A, 0x04, "sas_data_cnf"
 */
int  APDU_SASDataCnf (USHORT sesnum, UCHAR * apkt, USHORT len)                          
{

    return 1;
}

/*
 *    APDU in: 0x9F, 0x9A, 0x05, "sas_server_query"
 */
int  APDU_SASServerQuery (USHORT sesnum, UCHAR * apkt, USHORT len)                  
{

    return 1;
}

/*
 *    APDU in: 0x9F, 0x9A, 0x06, "sas_server_reply"
 */
int  APDU_SASServerReply (USHORT sesnum, UCHAR * apkt, USHORT len)                  
{

    return 1;
}

