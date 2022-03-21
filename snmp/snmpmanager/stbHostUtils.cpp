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

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "stbHostUtils.h"
#include "stbHostUtils_GetData.h"
#include "SnmpIORM.h"
#include "vl_ocStbHost_GetData.h"

/** Initializes the stbHostUtils module */
void
init_stbHostUtils(void)
{
    static oid currentChannelNumber_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,3,1 };
    static oid currentServiceType_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,3,2 };
    static oid currentChEncryptionStatus_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,3,3 };
    static oid deviceFriendlyName_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,3,4 };
    static oid currentZoomOption_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,3,5 };
    static oid channelLockStatus_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,3,6 };
    static oid parentalControlStatus_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,3,7 };
    static oid closedCaptionState_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,3,8 };
    static oid closedCaptionColor_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,3,9 };
    static oid closedCaptionFont_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,3,10 };
    static oid closedCaptionOpacity_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,3,11 };
    static oid easState_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,3,12 };
    static oid fpRecordLedStatus_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,3,13 };
    static oid fpPowerLedStatus_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,3,14 };

  DEBUGMSGTL(("stbHostUtils", "Initializing\n"));

    netsnmp_register_scalar(
        netsnmp_create_handler_registration("currentChannelNumber", handle_currentChannelNumber,
                               currentChannelNumber_oid, OID_LENGTH(currentChannelNumber_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("currentServiceType", handle_currentServiceType,
                               currentServiceType_oid, OID_LENGTH(currentServiceType_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("currentChEncryptionStatus", handle_currentChEncryptionStatus,
                               currentChEncryptionStatus_oid, OID_LENGTH(currentChEncryptionStatus_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("deviceFriendlyName", handle_deviceFriendlyName,
                               deviceFriendlyName_oid, OID_LENGTH(deviceFriendlyName_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("currentZoomOption", handle_currentZoomOption,
                               currentZoomOption_oid, OID_LENGTH(currentZoomOption_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("channelLockStatus", handle_channelLockStatus,
                               channelLockStatus_oid, OID_LENGTH(channelLockStatus_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("parentalControlStatus", handle_parentalControlStatus,
                               parentalControlStatus_oid, OID_LENGTH(parentalControlStatus_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("closedCaptionState", handle_closedCaptionState,
                               closedCaptionState_oid, OID_LENGTH(closedCaptionState_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("closedCaptionColor", handle_closedCaptionColor,
                               closedCaptionColor_oid, OID_LENGTH(closedCaptionColor_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("closedCaptionFont", handle_closedCaptionFont,
                               closedCaptionFont_oid, OID_LENGTH(closedCaptionFont_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("closedCaptionOpacity", handle_closedCaptionOpacity,
                               closedCaptionOpacity_oid, OID_LENGTH(closedCaptionOpacity_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("easState", handle_easState,
                               easState_oid, OID_LENGTH(easState_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("fpRecordLedStatus", handle_fpRecordLedStatus,
                               fpRecordLedStatus_oid, OID_LENGTH(fpRecordLedStatus_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("fpPowerLedStatus", handle_fpPowerLedStatus,
                               fpPowerLedStatus_oid, OID_LENGTH(fpPowerLedStatus_oid),
                               HANDLER_CAN_RONLY
        ));
}

int
handle_currentChannelNumber(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */
    
    switch(reqinfo->mode) {

        case MODE_GET:

	/********************** VL CODE  start*********************/
		char currentChannelNumber[16];
		if(0 == Sget_stbHostUtilsChannelNumber(currentChannelNumber, sizeof(currentChannelNumber)))
		{
		      printf("ERROR:%s\n", __FUNCTION__);
		      return SNMP_ERR_GENERR;
		}
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *)currentChannelNumber,
                                     strlen(currentChannelNumber));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_currentChannelNumber\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_currentServiceType(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */
    
    switch(reqinfo->mode) {

        case MODE_GET:
	/********************** VL CODE  start*********************/
		char currentServiceType[16];
		if(0 == Sget_stbHostUtilsCurrentServiceType(currentServiceType, sizeof(currentServiceType)))
		{
		      printf("ERROR:%s\n", __FUNCTION__);
		      return SNMP_ERR_GENERR;
		}
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) currentServiceType,
                                     strlen(currentServiceType));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_currentServiceType\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_currentChEncryptionStatus(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */
    
    switch(reqinfo->mode) {

        case MODE_GET:
	/********************** VL CODE  start*********************/
		char currentChEncryptionStatus[16];
		if(0 == Sget_stbHostUtilsCurrentChEncryptionStatus(currentChEncryptionStatus, sizeof(currentChEncryptionStatus)))
		{
		      printf("ERROR:%s\n", __FUNCTION__);
		      return SNMP_ERR_GENERR;
		}
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) currentChEncryptionStatus,
                                     strlen(currentChEncryptionStatus));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_currentChEncryptionStatus\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_deviceFriendlyName(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */
    
    switch(reqinfo->mode) {

        case MODE_GET:
	/********************** VL CODE  start*********************/
		char deviceFriendlyName[16];
		if(0 == Sget_stbHostUtilsDeviceFriendlyName(deviceFriendlyName, sizeof(deviceFriendlyName)))
		{
		      printf("ERROR:%s\n", __FUNCTION__);
		      return SNMP_ERR_GENERR;
		}
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) deviceFriendlyName,
                                     strlen(deviceFriendlyName));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_deviceFriendlyName\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_currentZoomOption(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */
    
    switch(reqinfo->mode) {

        case MODE_GET:
	/********************** VL CODE  start*********************/
		char currentZoomOption[16];
		if(0 == Sget_stbHostUtilsCurrentZoomOption(currentZoomOption, sizeof(currentZoomOption)))
		{
		      printf("ERROR:%s\n", __FUNCTION__);
		      return SNMP_ERR_GENERR;
		}
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) currentZoomOption,
                                     strlen(currentZoomOption));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_currentZoomOption\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_channelLockStatus(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */
    
    switch(reqinfo->mode) {

        case MODE_GET:
	/********************** VL CODE  start*********************/
		char channelLockStatus[16];
		if(0 == Sget_stbHostUtilsChannelLockStatus(channelLockStatus, sizeof(channelLockStatus)))
		{
		      printf("ERROR:%s\n", __FUNCTION__);
		      return SNMP_ERR_GENERR;
		}
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *)channelLockStatus,
                                     strlen(channelLockStatus));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_channelLockStatus\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_parentalControlStatus(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */
    
    switch(reqinfo->mode) {

        case MODE_GET:
	/********************** VL CODE  start*********************/
		char parentalControlStatus[16];
		if(0 == Sget_stbHostUtilsParentalControlStatus(parentalControlStatus, sizeof(parentalControlStatus)))
		{
		      printf("ERROR:%s\n", __FUNCTION__);
		      return SNMP_ERR_GENERR;
		}
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *)parentalControlStatus,
                                     strlen(parentalControlStatus));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_parentalControlStatus\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_closedCaptionState(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */
    
    switch(reqinfo->mode) {

        case MODE_GET:
	/********************** VL CODE  start*********************/
		char closedCaptionState[16];
		if(0 == Sget_stbHostUtilsClosedCaptionState(closedCaptionState, sizeof(closedCaptionState)))
		{
		      printf("ERROR:%s\n", __FUNCTION__);
		      return SNMP_ERR_GENERR;
		}
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) closedCaptionState,
                                     strlen(closedCaptionState));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_closedCaptionState\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_closedCaptionColor(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */
    
    switch(reqinfo->mode) {

        case MODE_GET:
	/********************** VL CODE  start*********************/
		char closedCaptionColor[16];
		if(0 == Sget_stbHostUtilsClosedCaptionColor(closedCaptionColor, sizeof(closedCaptionColor)))
		{
		      printf("ERROR:%s\n", __FUNCTION__);
		      return SNMP_ERR_GENERR;
		}
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) closedCaptionColor,
                                     strlen(closedCaptionColor));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_closedCaptionColor\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_closedCaptionFont(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */
    
    switch(reqinfo->mode) {

        case MODE_GET:
	/********************** VL CODE  start*********************/
		char closedCaptionFont[16];
		if(0 == Sget_stbHostUtilsClosedCaptionFont(closedCaptionFont, sizeof(closedCaptionFont)))
		{
		      printf("ERROR:%s\n", __FUNCTION__);
		      return SNMP_ERR_GENERR;
		}
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) closedCaptionFont,
                                     strlen(closedCaptionFont));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_closedCaptionFont\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_closedCaptionOpacity(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */
    
    switch(reqinfo->mode) {

        case MODE_GET:
	/********************** VL CODE  start*********************/
		char closedCaptionOpacity[16];
		if(0 == Sget_stbHostUtilsClosedCaptionOpacity(closedCaptionOpacity, sizeof(closedCaptionOpacity)))
		{
		      printf("ERROR:%s\n", __FUNCTION__);
		      return SNMP_ERR_GENERR;
		}
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *)closedCaptionOpacity,
                                     strlen(closedCaptionOpacity));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_closedCaptionOpacity\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_easState(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */
    
    switch(reqinfo->mode) {

        case MODE_GET:
	/********************** VL CODE  start*********************/
		char easState[16];
		if(0 == Sget_stbHostUtilsEASState(easState, sizeof(easState)))
		{
		      printf("ERROR:%s\n", __FUNCTION__);
		      return SNMP_ERR_GENERR;
		}
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) easState,
                                     strlen(easState));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_easState\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_fpRecordLedStatus(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */
    
    switch(reqinfo->mode) {

        case MODE_GET:
	/********************** VL CODE  start*********************/
		char fpRecordLedStatus[16];
		if(0 == Sget_stbHostUtilsFpRecordLedStatus(fpRecordLedStatus, sizeof(fpRecordLedStatus)))
		{
		      printf("ERROR:%s\n", __FUNCTION__);
		      return SNMP_ERR_GENERR;
		}
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) fpRecordLedStatus,
                                     strlen(fpRecordLedStatus));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_fpRecordLedStatus\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_fpPowerLedStatus(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */
    
    switch(reqinfo->mode) {

        case MODE_GET:
	/********************** VL CODE  start*********************/
		char fpPowerLedStatus[16];
		if(0 == Sget_stbHostUtilsFpPowerLedStatus(fpPowerLedStatus, sizeof(fpPowerLedStatus)))
		{
		      printf("ERROR:%s\n", __FUNCTION__);
		      return SNMP_ERR_GENERR;
		}
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) fpPowerLedStatus,
                                     strlen(fpPowerLedStatus));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_fpPowerLedStatus\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
