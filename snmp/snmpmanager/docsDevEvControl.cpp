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
#include "docsDevEvControl.h"
#include "docsDevEventTable.h"
#include "docsDevEvControlTable.h"
#include "docsDevEventTable_enums.h"
//#include "vlMemCpy.h"
#include "utilityMacros.h"
#include "SnmpIORM.h"
#include "rdk_debug.h"
/** Initializes the docsDevEvControl module */


void
init_docsDevEvControl(void)
{
    static oid      docsDevEvControl_oid[] =
        { 1, 3, 6, 1, 2, 1, 69, 1, 5, 1 };

    DEBUGMSGTL(("docsDevEvControl", "Initializing\n"));

    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("docsDevEvControl", handle_docsDevEvControl,
                             docsDevEvControl_oid,
                             OID_LENGTH(docsDevEvControl_oid),
                             HANDLER_CAN_RWRITE));
}

static int gOld_docEvContorl = resetLog; // initial value on bootup is resetLog
static int gdocEvContorl = useDefaultReporting; // initial value on bootup useDefaultReporting
/* this value keep on update the EventCortoal flag*/
int getdocEvContorltag()
{
    return gdocEvContorl;
}

int
handle_docsDevEvControl(netsnmp_mib_handler *handler,
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
        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&gdocEvContorl
                                 /* XXX: a pointer to the scalar's data */
                                 ,sizeof(gdocEvContorl)
                                 /*
                                  * XXX: the length of the data in bytes 
                                  */ );
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
        gOld_docEvContorl= gdocEvContorl;
        SNMP_DEBUGPRINT(" gOld_docEvContorl :::::::::  %d\n", gOld_docEvContorl);
        ret = netsnmp_check_vb_type(requests->requestvb, ASN_INTEGER);
        if (ret != SNMP_ERR_NOERROR) {
            netsnmp_set_request_error(reqinfo, requests, ret);
        }
        break;

    case MODE_SET_RESERVE2:
        /*
         * XXX malloc "undo" storage buffer 
         */
            
        //if ( /* XXX if malloc, or whatever, failed: */ ) {
          //  netsnmp_set_request_error(reqinfo, requests,
            //                          SNMP_ERR_RESOURCEUNAVAILABLE);
        //}
        break;

    case MODE_SET_FREE:
        /*
         * XXX: free resources allocated in RESERVE1 and/or
         * RESERVE2.  Something failed somewhere, and the states
         * below won't be called. 
         */

        break;

    case MODE_SET_ACTION:
        /*
         * XXX: perform the value change here 
         */
        gOld_docEvContorl = gdocEvContorl;
        SNMP_DEBUGPRINT("\nMODE_SET_ACTION :: *requests->requestvb->val.integer :::::::::  %d\n", *requests->requestvb->val.integer);
         switch(*requests->requestvb->val.integer){
              case 1:
                  gdocEvContorl = resetLog ;//== TV_TRUE user option beyond the range
                   break;
              case 2: 
                 gdocEvContorl =  useDefaultReporting;//== TV_TRUE user option beyond the range 
                  break;
         }
         SNMP_DEBUGPRINT("\nMODE_SET_ACTION :: gdocEvContorl :::::::::  %d\n", gdocEvContorl);
        if ( gdocEvContorl == gOld_docEvContorl ) {
            //netsnmp_set_request_error(reqinfo, requests,SNMP_ERR_COMMITFAILED);
        }
        break;

    case MODE_SET_COMMIT:
        /*
         * XXX: delete temporary storage 
         */
       SNMP_DEBUGPRINT("\n MODE_SET_COMMIT :: gOld_docEvContorl:::::::::  %d\n", gOld_docEvContorl);
       SNMP_DEBUGPRINT("\n *requests->requestvb->val.integer :::::::::  %d\n", *requests->requestvb->val.integer);
        switch(*requests->requestvb->val.integer){
              case 1:
                  gdocEvContorl = resetLog ;//== TV_TRUE user option beyond the range
                  break;
              case 2: 
                 gdocEvContorl =  useDefaultReporting;//== TV_TRUE user option beyond the range 
                  break;
         }
       SNMP_DEBUGPRINT("\n MODE_SET_COMMIT :: gdocEvContorl :::::::::  %d\n", gdocEvContorl);
        if (gdocEvContorl == gOld_docEvContorl ){
            /*
             * try _really_really_ hard to never get to this point 
             */
            //netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_COMMITFAILED);
        }
        break;

    case MODE_SET_UNDO:
        /*
         * XXX: UNDO and return to previous value for the object 
         */
        if ( gdocEvContorl == gOld_docEvContorl ) {
            /*
             * try _really_really_ hard to never get to this point 
             */
            //netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_UNDOFAILED);
        }
        gdocEvContorl = gOld_docEvContorl;
        SNMP_DEBUGPRINT("\n gOld_docEvContorl:::::::::  %d\n", gOld_docEvContorl);         
        break;

    default:
        /*
         * we should never get here, so this is a really bad error 
         */
        snmp_log(LOG_ERR, "unknown mode (%d) in handle_docsDevEvControl\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }
    
    switch (reqinfo->mode)
    {
        case MODE_SET_ACTION:
        case MODE_SET_COMMIT:
        {
            switch(gdocEvContorl)
            {
                case resetLog:
                {
                    vlSnmpDocsEventTable_ResetLog();
                }
                break;
                
                case useDefaultReporting:
                {
                    // reset to defaults
                    vl_docsDevEvControlTable_SET_reporting_method(DOCSDEVEVLEVEL_EMERGENCY  , -1);
                    vl_docsDevEvControlTable_SET_reporting_method(DOCSDEVEVLEVEL_ALERT      , -1);
                    vl_docsDevEvControlTable_SET_reporting_method(DOCSDEVEVLEVEL_CRITICAL   , -1);
                    vl_docsDevEvControlTable_SET_reporting_method(DOCSDEVEVLEVEL_ERROR      , -1);
                    
                    vl_docsDevEvControlTable_SET_reporting_method(DOCSDEVEVLEVEL_WARNING    , -1);
                    vl_docsDevEvControlTable_SET_reporting_method(DOCSDEVEVLEVEL_NOTICE     , -1);
                    vl_docsDevEvControlTable_SET_reporting_method(DOCSDEVEVLEVEL_INFORMATION, -1);
                    vl_docsDevEvControlTable_SET_reporting_method(DOCSDEVEVLEVEL_DEBUG      , -1);
                }
                break;
            }
        }
        break;
    }

    return SNMP_ERR_NOERROR;
}
