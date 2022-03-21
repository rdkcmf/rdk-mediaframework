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
 *    APDU Functions to handle RESOURCE MANAGER
 *************************************************/

/*
 *    APDU in: 0x9F, 0x80, 0x10, "profile inq"
 */
int APDU_ProfileInq (USHORT sesnum, UCHAR * apkt, USHORT len)                         
{
    UCHAR ApduResp[] = \
    {    0, 0, 0,                // Apdu Tag value for APDU_ProfileReply()
        0x82, 0x00, 0x18,        // Size (to be modified according to declared resources)
        0x00, 0x01, 0x00, 0x41,    // RSC_RESOURCEMANAGER
        0x00, 0x03, 0x00, 0x41,    // RSC_CASUPPORT
        0x00, 0x02, 0x00, 0x41,    // RSC_APPINFO
        0x00, 0x24, 0x00, 0x41,    // RSC_DATETIME
        0x00, 0x40, 0x00, 0x41, // RSC_MMI
        0x00, 0x20, 0x00, 0x41, // RSC_HOSTCONTROL
        0x00, 0xA0, 0x00, 0x42,    // RSC_EXTENDEDCHANNEL
        0x00, 0xB0, 0x00, 0xC1    // RSC_COPY PROTECTION
    };

    AM_APDU_FROM_HOST(sesnum, ApduResp, sizeof(ApduResp));

      return 1;
}

/*
 *    APDU in: 0x9F, 0x80, 0x11, "profile reply"
 */
int  APDU_ProfileReply (USHORT sesnum, UCHAR * apkt, USHORT len)                
{

    return 1;
}

/*
 *  APDU in: 0x9F, 0x80, 0x12, "profile changed"
 */
int  APDU_ProfileChanged (USHORT sesnum, UCHAR * apkt, USHORT len)         
{
    UCHAR prof_inq[] = {0x9f, 0x80, 0x10, 0};

     AM_APDU_FROM_HOST(sesnum, prof_inq, sizeof(prof_inq));

    return 1;
}

/*    
 *    This function will be called by the host to initiate a profile_inq
 *    from the POD. Profile_inq() from the POD will cause the host to reply with
 *    a Profile Reply to the POD, sending all the available host resources to the POD
 */
int    Host_ProfileChanged (USHORT sesnum)
{
    UCHAR tagvalue[4];

//    Get_APDUTag ((int *)APDU_ProfileChanged, tagvalue);    // get tag value for function APDU_ProfileChanged() 
    tagvalue[3] = 0;
    AM_APDU_FROM_HOST(sesnum, tagvalue, 4);

    return 1;
}

