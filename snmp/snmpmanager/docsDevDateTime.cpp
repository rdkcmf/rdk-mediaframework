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

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "docsDevDateTime.h"
#include "docsDevEventTable_enums.h"
#include "utilityMacros.h"
#include "SnmpIORM.h"
/** Initializes the docsDevDateTime module */
static u_char old_docdateTime[TIME_MAX_CHAR];
static u_char docdateTime[TIME_MAX_CHAR];
size_t len_time;
void
init_docsDevDateTime(void)
{
    static oid      docsDevDateTime_oid[] =
        { 1, 3, 6, 1, 2, 1, 69, 1, 1, 2 };

    DEBUGMSGTL(("docsDevDateTime", "Initializing\n"));

    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("docsDevDateTime", handle_docsDevDateTime,
                             docsDevDateTime_oid,
                             OID_LENGTH(docsDevDateTime_oid),
                             HANDLER_CAN_RWRITE));
      GetLocalTime_YMDZ(docdateTime,&len_time);
      
}

int
handle_docsDevDateTime(netsnmp_mib_handler *handler,
                       netsnmp_handler_registration *reginfo,
                       netsnmp_agent_request_info *reqinfo,
                       netsnmp_request_info *requests)
{
    int             ret;
    /*
     * We are never called for a GETNEXT if it's registered as a
     * "instance", as it's "magically" handled for us.  
     */

    /*
     * a instance handler also only hands us one request at a time, so
     * we don't need to loop over a list of requests; we'll only get one. 
     */

    switch (reqinfo->mode) {

    case MODE_GET:
        GetLocalTime_YMDZ(docdateTime,&len_time);
        snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                 (u_char *)
                                 docdateTime
                                 ,
                                 len_time);
        break;
        /*
         * SET REQUEST
         *
         * multiple states in the transaction.  See:
         * http://www.net-snmp.org/tutorial-5/toolkit/mib_module/set-actions.jpg
         */
    case MODE_SET_RESERVE1:
        /*
         * or you could use netsnmp_check_vb_type_and_size instead 
         */
        ret = netsnmp_check_vb_type(requests->requestvb, ASN_OCTET_STR);
                                           
        if (ret != SNMP_ERR_NOERROR) {
            netsnmp_set_request_error(reqinfo, requests, ret);
        }
        break;

    case MODE_SET_RESERVE2:
        /*
         * XXX malloc "undo" storage buffer
         */
                            
        if (len_time == 0) /* XXX if malloc, or whatever, failed:*/ 
        {
            netsnmp_set_request_error(reqinfo, requests,
                                      SNMP_ERR_RESOURCEUNAVAILABLE);
        }
        break;

    case MODE_SET_FREE:
        /*
         * XXX: free resources allocated in RESERVE1 and/or
         * RESERVE2.  Something failed somewhere, and the states
        * below won't be called.
         */
         // memcpy(docdateTime ,"\0",1);
      
        break;

    case MODE_SET_ACTION:
        /*
         * XXX: perform the value change here 
         */
          memcpy(old_docdateTime, docdateTime, len_time);

       memcpy(docdateTime,(char*) *requests->requestvb->val.string, strlen(( char*)(*requests->requestvb->val.string)));
      
        if (len_time == 0)  {
            netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_COMMITFAILED
                                      );
        }
        break;

    case MODE_SET_COMMIT:
        /*
         * XXX: delete temporary storage 
         */
          memcpy(docdateTime,(char*) *requests->requestvb->val.string, strlen(( char*)(*requests->requestvb->val.string)));
        if (len_time == 0)  /* XXX: error? */ {
            /*
             * try _really_really_ hard to never get to this point 
             */
            netsnmp_set_request_error(reqinfo, requests,
                                      SNMP_ERR_COMMITFAILED);
        }
        break;

    case MODE_SET_UNDO:
        /*
         * XXX: UNDO and return to previous value for the object 
         */
          memcpy(docdateTime, old_docdateTime, len_time);
        if (len_time == 0)  {
            /*
             * try _really_really_ hard to never get to this point 
             */
            netsnmp_set_request_error(reqinfo, requests,
                                      SNMP_ERR_UNDOFAILED);
        }
        break;

    default:
        /*
         * we should never get here, so this is a really bad error 
         */
        snmp_log(LOG_ERR, "unknown mode (%d) in handle_docsDevDateTime\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}