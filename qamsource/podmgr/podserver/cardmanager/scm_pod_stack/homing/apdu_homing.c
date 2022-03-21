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
#include <lcsm_log.h>

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
#include <rdk_debug.h>
#ifdef __cplusplus
extern "C" {
#endif

static int homingSndApdu( unsigned short sesnum, unsigned long tag, unsigned short dataLen, unsigned char *data );


/**************************************************
 *    APDU Functions to handle HOMING
 *************************************************/

/*
 *    APDU in: 0x9F, 0x99, 0x90, "open_homing"
 */
int  APDU_OpenHoming (USHORT sesnum, UCHAR *apkt, USHORT len)                          
{
    unsigned char *pMyData = NULL; // apdu carries no data
    unsigned short dataLen = 0;
    unsigned long tag = 0x9F9990;

    return homingSndApdu( sesnum, tag, dataLen, pMyData );
}

/*
 *    APDU in: 0x9F, 0x99, 0x91, "homing_cancelled"
 */
int  APDU_HomingCancelled (USHORT sesnum, UCHAR *apkt, USHORT len)                     
{
    unsigned char *pMyData = NULL; // apdu carries no data
    unsigned short dataLen = 0;
    unsigned long tag = 0x9F9991;

    return homingSndApdu( sesnum, tag, dataLen, pMyData );
}

/*
 *    APDU in: 0x9F, 0x99, 0x92, "open_homing_reply"
 */
int  APDU_OpenHomingReply (USHORT sesnum, UCHAR *apkt, USHORT len)               
{
    unsigned char *ptr;
    unsigned char tag = 0;

    ptr = apkt;
    // Get / check the tag to make sure we got a good packet
//    memcpy(&tag, ptr+2, sizeof(unsigned char));
    tag = *(ptr+2);
    if (tag != 0x92) 
    {
        MDEBUG (DPM_ERROR, "ERROR:homing: Non-matching tag in OpenHomingReply.\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

/*
 *    APDU in: 0x9F, 0x99, 0x93, "homing_active"
 */
int  APDU_HomingActive (USHORT sesnum, UCHAR *apkt, USHORT len)                      
{
    unsigned char *pMyData = NULL; // apdu carries no data
    unsigned short dataLen = 0;
    unsigned long tag = 0x9F9993;
    
    return homingSndApdu( sesnum, tag, dataLen, pMyData );
}

/*
 *    APDU in: 0x9F, 0x99, 0x94, "homing_complete"
 */
int  APDU_HomingComplete (USHORT sesnum, UCHAR *apkt, USHORT len)                  
{
    unsigned char *ptr;
    unsigned char tag = 0;

    ptr = apkt;
    // Get / check the tag to make sure we got a good packet
//    memcpy(&tag, ptr+2, sizeof(unsigned char));
    tag = *(ptr+2);
    if (tag != 0x94) 
    {
        MDEBUG (DPM_ERROR, "ERROR:homing: Non-matching tag in HomingComplete.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/*    
 *    APDU in: 0x9F, 0x99, 0x95, "firmware_upgrade"
 */
int  APDU_FirmUpgrade (USHORT sesnum, UCHAR *apkt, USHORT len)                         
{
    unsigned char *ptr;
    unsigned char tag = 0;

    ptr = apkt;
    // Get / check the tag to make sure we got a good packet
//    memcpy(&tag, ptr+2, sizeof(unsigned char));
    tag = *(ptr+2);
    if (tag != 0x95) 
    {
        MDEBUG (DPM_ERROR, "ERROR:homing: Non-matching tag in FirmUpgrade.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/*
 *    APDU in: 0x9F, 0x99, 0x96, "firmware_upgrade_reply"
 */
int  APDU_FirmUpgradeReply( USHORT sesnum, UCHAR *apkt, USHORT len )
{
    unsigned char *pMyData = NULL;  // apdu carries no data
    unsigned short dataLen = 0;
    unsigned long tag = 0x9F9996;

    return homingSndApdu( sesnum, tag, dataLen, pMyData );
}
int  SendAPDUProfileInq(  )
{
    unsigned char *pMyData = NULL;  // apdu carries no data
    unsigned short dataLen = 0;
    unsigned long tag = 0x9F8010;

    return 0;
//    return homingSndApdu( 1, tag, dataLen, pMyData );
}
/*
 *    APDU in: 0x9F, 0x99, 0x97, "firmware_upgrade_complete"
 */
int  APDU_FirmUpgradeComplete (USHORT sesnum, UCHAR *apkt, USHORT len)     
{
    unsigned char *ptr;
    unsigned char tag = 0;

    ptr = apkt;
    // Get / check the tag to make sure we got a good packet
//    memcpy(&tag, ptr+2, sizeof(unsigned char));
    
    if (*(ptr+2) != 0x97) 
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "ERROR:homing: Non-matching tag in FirmUpgrade.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


static int homingSndApdu( unsigned short sesnum, unsigned long tag, unsigned short dataLen, unsigned char *data )
{
    unsigned short alen;
    unsigned char papdu[MAX_APDU_HEADER_BYTES + dataLen];

    if ( buildApdu(papdu, &alen, tag, dataLen, data) == NULL ) 
    {
        MDEBUG (DPM_ERROR, "ERROR:homing: Unable to build apdu.\n");
        return EXIT_FAILURE;
    }

    AM_APDU_FROM_HOST(sesnum, papdu, alen);

    return EXIT_SUCCESS;
}

#ifdef __cplusplus
}
#endif

