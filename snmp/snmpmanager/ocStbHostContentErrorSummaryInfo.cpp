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
#include "ocStbHostContentErrorSummaryInfo.h"
#include "SnmpIORM.h"
#include "vl_ocStbHost_GetData.h"
#include "rdk_debug.h"

/** Initializes the ocStbHostContentErrorSummaryInfo module */
void
init_ocStbHostContentErrorSummaryInfo(void)
{
    static oid      ocStbHostPatTimeoutCount_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 5, 7, 1 };
    static oid      ocStbHostPmtTimeoutCount_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 5, 7, 2 };
    static oid      ocStbHostOobCarouselTimeoutCount_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 5, 7, 3 };
    static oid      ocStbHostInbandCarouselTimeoutCount_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 5, 7, 4 };

    DEBUGMSGTL(("ocStbHostContentErrorSummaryInfo", "Initializing\n"));

    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostPatTimeoutCount",
                             handle_ocStbHostPatTimeoutCount,
                             ocStbHostPatTimeoutCount_oid,
                             OID_LENGTH(ocStbHostPatTimeoutCount_oid),
                             HANDLER_CAN_RWRITE));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostPmtTimeoutCount",
                             handle_ocStbHostPmtTimeoutCount,
                             ocStbHostPmtTimeoutCount_oid,
                             OID_LENGTH(ocStbHostPmtTimeoutCount_oid),
                             HANDLER_CAN_RWRITE));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostOobCarouselTimeoutCount",
                             handle_ocStbHostOobCarouselTimeoutCount,
                             ocStbHostOobCarouselTimeoutCount_oid,
                             OID_LENGTH
                             (ocStbHostOobCarouselTimeoutCount_oid),
                             HANDLER_CAN_RWRITE));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostInbandCarouselTimeoutCount",
                             handle_ocStbHostInbandCarouselTimeoutCount,
                             ocStbHostInbandCarouselTimeoutCount_oid,
                             OID_LENGTH
                             (ocStbHostInbandCarouselTimeoutCount_oid),
                             HANDLER_CAN_RWRITE));
}
//have to update these values
extern int PatTimeoutCount = 0;
extern int Old_PatTimeoutCount = 0;
extern int PmtTimeoutCount = 0;
extern int Old_PmtTimeoutCount = 0;
extern int OobCarouselTimeoutCount = 0;
extern int Old_OobCarouselTimeoutCount = 0;
extern int InbandCarouselTimeoutCount = 0;
extern int Old_InbandCarouselTimeoutCount = 0;
int
handle_ocStbHostPatTimeoutCount(netsnmp_mib_handler *handler,
                                netsnmp_handler_registration *reginfo,
                                netsnmp_agent_request_info *reqinfo,
                                netsnmp_request_info *requests)
{
    int ret = 0;
    int pat_update = 0;
    unsigned long pat = 0;
    unsigned long pmt = 0;
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
        Sget_ocStbHostPatPmtTimeoutCount(&pat, &pmt);
        PatTimeoutCount = pat;
        snmp_set_var_typed_value(requests->requestvb, ASN_COUNTER,
                                 (u_char *)&PatTimeoutCount
                                         ,sizeof(PatTimeoutCount));
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
        ret = netsnmp_check_vb_type(requests->requestvb, ASN_COUNTER);
        if (ret != SNMP_ERR_NOERROR) {
            netsnmp_set_request_error(reqinfo, requests, ret);
        }
        break;

//     case MODE_SET_RESERVE2:
//         /*
//          * XXX malloc "undo" storage buffer
//          */
//         if ( /* XXX if malloc, or whatever, failed: */ ) {
//             netsnmp_set_request_error(reqinfo, requests,
//                                       SNMP_ERR_RESOURCEUNAVAILABLE);
//         }
//         break;
//
//     case MODE_SET_FREE:
//         /*
//          * XXX: free resources allocated in RESERVE1 and/or
//          * RESERVE2.  Something failed somewhere, and the states
//          * below won't be called.
//          */
//         break;

    case MODE_SET_ACTION:
        /*
         * XXX: perform the value change here
         */
        pat_update = *requests->requestvb->val.integer;
        if (pat_update ==  0) {
            netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_RESOURCEUNAVAILABLE);
        }
        break;

    case MODE_SET_COMMIT:
        /*
         * XXX: delete temporary storage
         */

        pat_update = *requests->requestvb->val.integer;

        Old_PatTimeoutCount = PatTimeoutCount;
        PatTimeoutCount  = pat_update;
        if ( PatTimeoutCount < 0) {
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
        PatTimeoutCount = Old_PatTimeoutCount;

        if ( PatTimeoutCount == pat_update ) {
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
        snmp_log(LOG_ERR,
                 "unknown mode (%d) in handle_ocStbHostPatTimeoutCount\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbHostPmtTimeoutCount(netsnmp_mib_handler *handler,
                                netsnmp_handler_registration *reginfo,
                                netsnmp_agent_request_info *reqinfo,
                                netsnmp_request_info *requests)
{
    int             ret;
    int pmt_update;
    unsigned long pmt = 0;
    unsigned long pat = 0;
    /*
     * We are never called for a GETNEXT if it's registered as a
     * "instance", as it's "magically" handled for us.
     */

    /*
     * a instance handler also only hands us one request at a time, so
     * we don't need to loop over a list of requests; we'll only get one.
     */

    switch (reqinfo->mode) {
        Sget_ocStbHostPatPmtTimeoutCount(&pat, &pmt);
        PmtTimeoutCount = pmt;
    case MODE_GET:
        snmp_set_var_typed_value(requests->requestvb, ASN_COUNTER,
                                 (u_char *)&PmtTimeoutCount
                                         ,sizeof(PmtTimeoutCount) );
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
        ret = netsnmp_check_vb_type(requests->requestvb, ASN_COUNTER);
        if (ret != SNMP_ERR_NOERROR) {
            netsnmp_set_request_error(reqinfo, requests, ret);
        }
        break;

//     case MODE_SET_RESERVE2:
//         /*
//          * XXX malloc "undo" storage buffer
//          */
//         if ( /* XXX if malloc, or whatever, failed: */ ) {
//             netsnmp_set_request_error(reqinfo, requests,
//                                       SNMP_ERR_RESOURCEUNAVAILABLE);
//         }
//         break;
//
//     case MODE_SET_FREE:
//         /*
//          * XXX: free resources allocated in RESERVE1 and/or
//          * RESERVE2.  Something failed somewhere, and the states
//          * below won't be called.
//          */
//         break;

    case MODE_SET_ACTION:
        /*
         * XXX: perform the value change here
         */
        pmt_update = *requests->requestvb->val.integer;

        if ( pmt_update < 0 ) {
            netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_RESOURCEUNAVAILABLE
                                      );
        }
        break;

    case MODE_SET_COMMIT:
        /*
         * XXX: delete temporary storage
         */
        pmt_update = *requests->requestvb->val.integer;
        Old_PmtTimeoutCount = PmtTimeoutCount;
        PmtTimeoutCount = pmt_update;
        if ( PmtTimeoutCount < 0) {
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
        PmtTimeoutCount = Old_PmtTimeoutCount;
        if ( PmtTimeoutCount < 0 ) {
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
        snmp_log(LOG_ERR,
                 "unknown mode (%d) in handle_ocStbHostPmtTimeoutCount\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbHostOobCarouselTimeoutCount(netsnmp_mib_handler *handler,
                                        netsnmp_handler_registration
                                        *reginfo,
                                        netsnmp_agent_request_info
                                        *reqinfo,
                                        netsnmp_request_info *requests)
{
    int             ret;
    int obbcar_update;
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
        if(0 == Sget_ocStbHostOobCarouselTimeoutCount(&OobCarouselTimeoutCount))
        {
              SNMP_DEBUGPRINT("ERROR:vlget_ocStbHostHostID");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_COUNTER,
                                 (u_char *)&OobCarouselTimeoutCount
                                         ,sizeof(OobCarouselTimeoutCount));
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
        ret = netsnmp_check_vb_type(requests->requestvb, ASN_COUNTER);
        if (ret != SNMP_ERR_NOERROR) {
            netsnmp_set_request_error(reqinfo, requests, ret);
        }
        break;

//     case MODE_SET_RESERVE2:
//         /*
//          * XXX malloc "undo" storage buffer
//          */
//         if ( /* XXX if malloc, or whatever, failed: */ ) {
//             netsnmp_set_request_error(reqinfo, requests,
//                                       SNMP_ERR_RESOURCEUNAVAILABLE);
//         }
//         break;
//
//     case MODE_SET_FREE:
//         /*
//          * XXX: free resources allocated in RESERVE1 and/or
//          * RESERVE2.  Something failed somewhere, and the states
//          * below won't be called.
//          */
//         break;

    case MODE_SET_ACTION:
        /*
         * XXX: perform the value change here
         */
        obbcar_update = *requests->requestvb->val.integer;
        if (obbcar_update < 0 ) {
            netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_RESOURCEUNAVAILABLE
                                      );
        }
        break;

    case MODE_SET_COMMIT:
        /*
         * XXX: delete temporary storage
         */
        obbcar_update = *requests->requestvb->val.integer;
        Old_OobCarouselTimeoutCount = OobCarouselTimeoutCount;
        OobCarouselTimeoutCount = obbcar_update;
        if ( OobCarouselTimeoutCount  < 0 ) {
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
        OobCarouselTimeoutCount = Old_OobCarouselTimeoutCount;
        if ( OobCarouselTimeoutCount < 0 ) {
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
        snmp_log(LOG_ERR,
                 "unknown mode (%d) in handle_ocStbHostOobCarouselTimeoutCount\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbHostInbandCarouselTimeoutCount(netsnmp_mib_handler *handler,
                                           netsnmp_handler_registration
                                           *reginfo,
                                           netsnmp_agent_request_info
                                           *reqinfo,
                                           netsnmp_request_info *requests)
{
    int             ret;
    int Inbandcar_update;
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
        if(0 == Sget_ocStbHostInbandCarouselTimeoutCount(&InbandCarouselTimeoutCount))
        {
              SNMP_DEBUGPRINT("ERROR:vlget_ocStbHostHostID");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_COUNTER,
                                 (u_char *)&InbandCarouselTimeoutCount
                                         ,sizeof(InbandCarouselTimeoutCount));
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
        ret = netsnmp_check_vb_type(requests->requestvb, ASN_COUNTER);
        if (ret != SNMP_ERR_NOERROR) {
            netsnmp_set_request_error(reqinfo, requests, ret);
        }
        break;

//     case MODE_SET_RESERVE2:
//         /*
//          * XXX malloc "undo" storage buffer
//          */
//         if ( /* XXX if malloc, or whatever, failed: */ ) {
//             netsnmp_set_request_error(reqinfo, requests,
//                                       SNMP_ERR_RESOURCEUNAVAILABLE);
//         }
//         break;
//
//     case MODE_SET_FREE:
//         /*
//          * XXX: free resources allocated in RESERVE1 and/or
//          * RESERVE2.  Something failed somewhere, and the states
//          * below won't be called.
//          */
//         break;

    case MODE_SET_ACTION:
        /*
         * XXX: perform the value change here
         */
        Inbandcar_update = *requests->requestvb->val.integer;
        if ( Inbandcar_update < 0 ) {
            netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_RESOURCEUNAVAILABLE
                                      );
        }
        break;

    case MODE_SET_COMMIT:
        /*
         * XXX: delete temporary storage
         */
        Inbandcar_update = *requests->requestvb->val.integer;
        Old_InbandCarouselTimeoutCount = InbandCarouselTimeoutCount;
        InbandCarouselTimeoutCount = Inbandcar_update;
        if ( InbandCarouselTimeoutCount < 0 ) {
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
        InbandCarouselTimeoutCount = Old_InbandCarouselTimeoutCount;

        if ( InbandCarouselTimeoutCount< 0 ) {
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
        snmp_log(LOG_ERR,
                 "unknown mode (%d) in handle_ocStbHostInbandCarouselTimeoutCount\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}