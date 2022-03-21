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



#if 0       // obsolete now

#include <string.h>
#include <signal.h>
#include "utils.h"
#include "module.h"
#include "link.h"
#include "transport.h"
#include "session.h"
#include "ci.h"
#include "podlowapi.h"
#include "podhighapi.h"
#include "appmgr.h"
#include "applitest.h"
#include "systime-main.h"


/**************************************************
 *    APDU Functions to handle SYSTEM TIME
 *************************************************/

/*
 *    SIGALRM Handler
 */
static void wakeup(int signal)
{
    if (systime_openSessions(1,0)==0)
    {
        disable_alarm();
        MDEBUG (DPM_SYSTIME, "SIGALRM handler disabled due to system time session closing\n");
    }
    else
        MDEBUG (DPM_SYSTIME, "SIGALRM handler invoked\n");
}

static sighandler_t old_handler=0;    // original alarm timer handler
void disable_alarm (void)
{
    if (old_handler==0)    // we have not sent any time packet yet
        return;        // thus, the signal(SIGALRM, wakeup) has not been called. So, simply return
    
    signal (SIGALRM, old_handler);    // restore the old SIGALRM handler
    alarm(0);
}

/*
 *    APDU in: 0x9F, 0x84, 0x42, "system_time_inq"
 */
UCHAR sec=0;    // response_interval for sending time APDU to the POD
int  APDU_SysTimeInq (USHORT sesnum, UCHAR * apkt, USHORT len)                      
{
    sighandler_t    current_handler;

    current_handler = signal(SIGALRM, wakeup);    // install SIGALRM handler
    if (current_handler != (sighandler_t) wakeup)    // only save original handler - not the wakeup handler!
        old_handler = current_handler;

    sec = *(apkt+4); 
    MDEBUG (DPM_SYSTIME, "called with response_interval=%d second\n", (int)sec);

    for (;;)
    {
        APDU_SysTime (sesnum, apkt, len);    // actual sending of time packet
        if (*(apkt+4) == 0 || systime_openSessions(1,0)==0)    // either interval=0 or session is closed
        {
            disable_alarm();
            break;
        }
        
        alarm(sec);    // re-arm the alarm timer
        pause();    // suspend ourselves until triggered by SIGALRM
    }
    
    return 1;
}

/*
 *    This is only to simulate getting the realtime clock data
 *    Needs to be replaced by an actual GetSystime from the systemz
 */
void GetSystime (UCHAR *ptr)
{
    static ULONG timedate=0;
    UCHAR *tptr, temp[4];
    
    timedate += sec;
    tptr = (UCHAR *)&timedate;
    
    // reverse the order of the 4 byte timedate, then copy it to the input parameter
    temp[3] = *tptr++;
    temp[2] = *tptr++;
    temp[1] = *tptr++;
    temp[0] = *tptr++;
    memcpy (ptr, (UCHAR *)temp, 4);
}
/*
 *    APDU in: 0x9F, 0x84, 0x43, "system_time"
 *    HOST to POD
 */
int  APDU_SysTime (USHORT sesnum, UCHAR * apkt, USHORT len)                              
{
    UCHAR apdu_out[10] = {0x9F, 0x84, 0x43, 0x05, 0, 0, 0, 0, 0, 0};
    
    /* Get System Time in form of xx xx xx xx yy (4 byte count since 1/1/80 and offset)
     then form an outgoing APDU : 
     */
    GetSystime (&apdu_out[4]);    // assume GetSystime places 5 bytes of time info at apdu_out[4], and higher
//    AM_APDU_FROM_HOST (sesnum, apdu_out, 10);
    AM_APDU_TO_POD (sesnum, apdu_out, 10);        // sending apdu to pod with mutex protection for the podstack queue

    return 1;
}

#endif // 0

