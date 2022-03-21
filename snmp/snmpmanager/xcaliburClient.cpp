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

/**
 * @file This file contains the initialization of xcalibur client, getting ID of
 * varios parameters like plant id etc., power condition, DVR related info.
 *
 */

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "xcaliburClient.h"
#include "proxymib_GetData.h"
#include "SnmpIORM.h"
#include "vl_ocStbHost_GetData.h"
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

/********************** VL CODE  start*********************/
	extern	CardInfo_t HostCardobj;
	DeviceSInfo_t gDevSwObj;
/********************** VL CODE  end*********************/

/**
 * @defgroup XCAL XCalibur Client
 * @ingroup SNMP_MGR
 * @ingroup XCAL
 * @{
 */

/**
 * @brief This function is used to define oid for xcaliburClient parameters like
 * map ID, controller id, system time, no. of tuners etc. It registers handler
 * for each of these components.
 *
 * @return None
 */
void
init_xcaliburClient(void)
{
    static oid chMapId_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,2 };
    static oid controllerID_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,3 };
    static oid vodServerId_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,4 };
    static oid plantId_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,5 };
    static oid noTuners_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,6 };
    static oid hostPowerOn_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,7 };
    static oid dvrCapable_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,8 };
    static oid dvrEnabled_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,9 };
    static oid vodEnabled_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,10 };
    static oid currentChannel_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,11 };
    static oid ccStatus_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,12 };
    static oid easStatus_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,13 };
    static oid ethernetEnabled_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,18 };
    static oid ethernetInUse_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,19 };
    static oid sysTime_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,20 };
    static oid sysTimezone_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,21 };
    static oid sysDSTOffset_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,22 };
    static oid siCacheSize_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,23 };
    static oid siLoadComplete_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,24 };
    static oid noDvrTuners_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,25 };
    static oid freeDiskSpace_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,26 };
    static oid cableCardMacAddr_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,27 };
    static oid hdCapable_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,28 };
    static oid hdAuthorized_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,29 };
    static oid mdvrAuthorized_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,30 };
    static oid dvrReady_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,31 };
    static oid dvrProvisioned_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,32 };
    static oid dvrNotBlocked_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,33 };
    static oid dvrRecording_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,37 };
    static oid dlnadvrAuthorized_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,38 };
    static oid dlnalinearAuthorized_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,39 };
    static oid imageVersion_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,40 };
    static oid imageTimestamp_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,41 };
    static oid eidDisconnect_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,42 };
    static oid dvrProtocolVersion_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,46 };

//Commenting out the following mibs as they are now unused
#if 0
    static oid t2wpVersion_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,1 };
    static oid networkType_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,34 };
    static oid networkVersion_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,35 };
    static oid specVersion_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,36 };
    static oid hnIp_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,43 };
    static oid hnPort_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,44 };
    static oid hnReady_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,45 };
    static oid avgChannelTuneTime_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,14 };
    static oid cmacVersion_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,15 };
    static oid mocaEnabled_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,16 };
    static oid mocaInUse_oid[] = { 1,3,6,1,4,1,17270,9225,1,4,1,17 };
#endif

  DEBUGMSGTL(("xcaliburClient", "Initializing\n"));

    netsnmp_register_scalar(
        netsnmp_create_handler_registration("chMapId", handle_chMapId,
                               chMapId_oid, OID_LENGTH(chMapId_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("controllerID", handle_controllerID,
                               controllerID_oid, OID_LENGTH(controllerID_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("vodServerId", handle_vodServerId,
                               vodServerId_oid, OID_LENGTH(vodServerId_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("plantId", handle_plantId,
                               plantId_oid, OID_LENGTH(plantId_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("noTuners", handle_noTuners,
                               noTuners_oid, OID_LENGTH(noTuners_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("hostPowerOn", handle_hostPowerOn,
                               hostPowerOn_oid, OID_LENGTH(hostPowerOn_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("dvrCapable", handle_dvrCapable,
                               dvrCapable_oid, OID_LENGTH(dvrCapable_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("dvrEnabled", handle_dvrEnabled,
                               dvrEnabled_oid, OID_LENGTH(dvrEnabled_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("vodEnabled", handle_vodEnabled,
                               vodEnabled_oid, OID_LENGTH(vodEnabled_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("currentChannel", handle_currentChannel,
                               currentChannel_oid, OID_LENGTH(currentChannel_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("ccStatus", handle_ccStatus,
                               ccStatus_oid, OID_LENGTH(ccStatus_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("easStatus", handle_easStatus,
                               easStatus_oid, OID_LENGTH(easStatus_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("ethernetEnabled", handle_ethernetEnabled,
                               ethernetEnabled_oid, OID_LENGTH(ethernetEnabled_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("ethernetInUse", handle_ethernetInUse,
                               ethernetInUse_oid, OID_LENGTH(ethernetInUse_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("sysTime", handle_sysTime,
                               sysTime_oid, OID_LENGTH(sysTime_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("sysTimezone", handle_sysTimezone,
                               sysTimezone_oid, OID_LENGTH(sysTimezone_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("sysDSTOffset", handle_sysDSTOffset,
                               sysDSTOffset_oid, OID_LENGTH(sysDSTOffset_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("siCacheSize", handle_siCacheSize,
                               siCacheSize_oid, OID_LENGTH(siCacheSize_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("siLoadComplete", handle_siLoadComplete,
                               siLoadComplete_oid, OID_LENGTH(siLoadComplete_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("noDvrTuners", handle_noDvrTuners,
                               noDvrTuners_oid, OID_LENGTH(noDvrTuners_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("freeDiskSpace", handle_freeDiskSpace,
                               freeDiskSpace_oid, OID_LENGTH(freeDiskSpace_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("cableCardMacAddr", handle_cableCardMacAddr,
                               cableCardMacAddr_oid, OID_LENGTH(cableCardMacAddr_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("hdCapable", handle_hdCapable,
                               hdCapable_oid, OID_LENGTH(hdCapable_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("hdAuthorized", handle_hdAuthorized,
                               hdAuthorized_oid, OID_LENGTH(hdAuthorized_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("mdvrAuthorized", handle_mdvrAuthorized,
                               mdvrAuthorized_oid, OID_LENGTH(mdvrAuthorized_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("dvrReady", handle_dvrReady,
                               dvrReady_oid, OID_LENGTH(dvrReady_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("dvrProvisioned", handle_dvrProvisioned,
                               dvrProvisioned_oid, OID_LENGTH(dvrProvisioned_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("dvrNotBlocked", handle_dvrNotBlocked,
                               dvrNotBlocked_oid, OID_LENGTH(dvrNotBlocked_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("dvrRecording", handle_dvrRecording,
                               dvrRecording_oid, OID_LENGTH(dvrRecording_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("dlnadvrAuthorized", handle_dlnadvrAuthorized,
                               dlnadvrAuthorized_oid, OID_LENGTH(dlnadvrAuthorized_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("dlnalinearAuthorized", handle_dlnalinearAuthorized,
                               dlnalinearAuthorized_oid, OID_LENGTH(dlnalinearAuthorized_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("imageVersion", handle_imageVersion,
                               imageVersion_oid, OID_LENGTH(imageVersion_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("imageTimestamp", handle_imageTimestamp,
                               imageTimestamp_oid, OID_LENGTH(imageTimestamp_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("eidDisconnect", handle_eidDisconnect,
                               eidDisconnect_oid, OID_LENGTH(eidDisconnect_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("dvrProtocolVersion", handle_dvrProtocolVersion,
                               dvrProtocolVersion_oid, OID_LENGTH(dvrProtocolVersion_oid),
                               HANDLER_CAN_RONLY
        ));
//Commenting out the following mibs as they are now unused
#if 0
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("t2wpVersion", handle_t2wpVersion,
                               t2wpVersion_oid, OID_LENGTH(t2wpVersion_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("avgChannelTuneTime", handle_avgChannelTuneTime,
                               avgChannelTuneTime_oid, OID_LENGTH(avgChannelTuneTime_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("cmacVersion", handle_cmacVersion,
                               cmacVersion_oid, OID_LENGTH(cmacVersion_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("mocaEnabled", handle_mocaEnabled,
                               mocaEnabled_oid, OID_LENGTH(mocaEnabled_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("mocaInUse", handle_mocaInUse,
                               mocaInUse_oid, OID_LENGTH(mocaInUse_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("networkType", handle_networkType,
                               networkType_oid, OID_LENGTH(networkType_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("networkVersion", handle_networkVersion,
                               networkVersion_oid, OID_LENGTH(networkVersion_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("specVersion", handle_specVersion,
                               specVersion_oid, OID_LENGTH(specVersion_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("hnIp", handle_hnIp,
                               hnIp_oid, OID_LENGTH(hnIp_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("hnPort", handle_hnPort,
                               hnPort_oid, OID_LENGTH(hnPort_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("hnReady", handle_hnReady,
                               hnReady_oid, OID_LENGTH(hnReady_oid),
                               HANDLER_CAN_RONLY
        ));

#endif
}

/**
 * @brief This function is used to get t2p version of xcalibur client and then
 * set received data into the netsnmp request info structure. 
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where t2p version
 * of xcalibur client is saved.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_t2wpVersion(netsnmp_mib_handler *handler,
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
		char t2pVersion[16];
        if(0 == Sget_xcaliburClientT2pVersion(t2pVersion, sizeof(t2pVersion)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *)t2pVersion,
                                     strlen(t2pVersion));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_t2wpVersion\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to get channel map id and then set received data
 * into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where map id of
 * channel is saved.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_chMapId(netsnmp_mib_handler *handler,
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
		char chMapId[16];
        if(0 == Sget_xcaliburClientChannelMapId(chMapId, sizeof(chMapId)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *)chMapId, 
                                     strlen(chMapId));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_chMapId\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to get controller id of client and then set received data
 * into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where controller id of
 * client is saved.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_controllerID(netsnmp_mib_handler *handler,
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
		char controllerId[16];
        if(0 == Sget_xcaliburClientControllerID(controllerId, sizeof(controllerId)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *)controllerId,
                                     strlen(controllerId));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_controllerID\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to get VOD server id of xcalibur client and then
 * set received data into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where vod server id of
 * client is saved.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_vodServerId(netsnmp_mib_handler *handler,
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
		char vodServerId[16];
        if(0 == Sget_xcaliburClientVodServerId(vodServerId, sizeof(vodServerId)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *)vodServerId,
                                     strlen(vodServerId));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_vodServerId\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to get Plant id of client and then set received data
 * into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where plant id of
 * client is saved.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_plantId(netsnmp_mib_handler *handler,
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
		char plantId[16];
        if(0 == Sget_xcaliburClientPlantId(plantId, sizeof(plantId)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) plantId,
                                     strlen(plantId));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_plantId\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to get number of available tuners for client and
 * then set received data into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where total num
 * of available tuners of client is stored.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_noTuners(netsnmp_mib_handler *handler,
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
		char numTuners[16];
        if(0 == Sget_xcaliburClientNumberOfTuners(numTuners, sizeof(numTuners)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) numTuners,
                                     strlen(numTuners));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_noTuners\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to get info about power source like Power Status
 * (ON or OFF) and Ac Outlet Status (Switched etc.) and then set these received data
 * into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where info of power
 * supply is saved.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_hostPowerOn(netsnmp_mib_handler *handler,
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
		HostPowerInfo_t HostPowerobj;
        if(0 == Sget_ocStbHostPowerInf(&HostPowerobj))
        {
              printf("ERROR:Sget_ocStbHostPowerInf");
              return SNMP_ERR_GENERR;
        }

#if 0
        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&g_HostPowerobj.ocStbHostPowerStatus,
                                 sizeof(g_HostPowerobj.ocStbHostPowerStatus)
                                );
        /********************** VL CODE  end*********************/
#endif
		char powerStatus[8];
		if(1==HostPowerobj.ocStbHostPowerStatus)
		{
			snprintf(powerStatus, sizeof(powerStatus),"powerOn", sizeof("powerOn"));
		}
		else if (2==HostPowerobj.ocStbHostPowerStatus)
		{
			snprintf(powerStatus,sizeof(powerStatus), "standBy", sizeof("standBy"));
		}
		//printf("handle_hostPowerOn : %d\n", HostPowerobj.ocStbHostPowerStatus);
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) powerStatus,
                                     sizeof(powerStatus));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_hostPowerOn\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to check whether xcalibur client is DVR (Digital
 * Video Recording) capable or not.It then sets received data into the netsnmp 
 * request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where capability 
 * of client is stored.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_dvrCapable(netsnmp_mib_handler *handler,
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
		char dvrCapable[16];
        if(0 == Sget_xcaliburClientDVRCapable(dvrCapable, sizeof(dvrCapable)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) dvrCapable,
                                     strlen(dvrCapable));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_dvrCapable\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to check whether xcalibur client is DVR (Digital
 * Video Recording) enabled or not.It then sets received data into the netsnmp
 * request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where status of
 * DVR of client whether it is enabled or not, is stored.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_dvrEnabled(netsnmp_mib_handler *handler,
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
		char dvrEnabled[16];
        if(0 == Sget_xcaliburClientDVREnabled(dvrEnabled, sizeof(dvrEnabled)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *)dvrEnabled,
                                     strlen(dvrEnabled));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_dvrEnabled\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to check whether VOD is enabled or not in the client.
 * It then sets received data into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where status of
 * of VOD, whether enabled or not, is stored.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_vodEnabled(netsnmp_mib_handler *handler,
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
		char vodEnabled[16];
        if(0 == Sget_xcaliburClientVodEabled(vodEnabled, sizeof(vodEnabled)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) vodEnabled,
                                      strlen(vodEnabled));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_vodEnabled\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to check which one is the current channel.
 * It then sets received data into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where current
 * channel number is stored.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_currentChannel(netsnmp_mib_handler *handler,
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
		char currChannel[16];
        if(0 == Sget_xcaliburClientCurrentChannel(currChannel, sizeof(currChannel)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) currChannel,
                                     strlen(currChannel));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_currentChannel\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to check the status of closed caption.
 * It then sets received data into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where status of
 * closed caption is stored.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_ccStatus(netsnmp_mib_handler *handler,
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
		char ccStatus[16];
        if(0 == Sget_xcaliburClientCCStatus(ccStatus, sizeof(ccStatus)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) ccStatus,
                                     strlen(ccStatus));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_ccStatus\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to check the status of EAS message whether it is
 * recieved, in progress etc.It sets received data into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where status of
 * EAS message is stored.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_easStatus(netsnmp_mib_handler *handler,
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
		char easStatus[16];
        if(0 == Sget_xcaliburClientEASStatus(easStatus, sizeof(easStatus)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) easStatus,
                                     strlen(easStatus));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_easStatus\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to get the average time in which channel is getting
 * tuned. This received info is stored into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where average time
 * for channel tuning is stored.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_avgChannelTuneTime(netsnmp_mib_handler *handler,
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
		char avgChannelTuneTime[16];
        if(0 == Sget_xcaliburClientAvgChannelTuneTime(avgChannelTuneTime, sizeof(avgChannelTuneTime)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *)avgChannelTuneTime,
                                     strlen(avgChannelTuneTime));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_avgChannelTuneTime\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to get the CMAC Version of client. This received
 * info is stored into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where cmac version
 * is stored.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_cmacVersion(netsnmp_mib_handler *handler,
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
		char cmacVersion[16];
        if(0 == Sget_xcaliburClientCMACVersion(cmacVersion, sizeof(cmacVersion)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *)cmacVersion,
                                     strlen(cmacVersion));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_cmacVersion\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to check whether the client if MoCA enabled.
 * This received status is stored into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where status of
 * MoCA, whether enabled or not, is stored.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_mocaEnabled(netsnmp_mib_handler *handler,
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
		char mocaEnabled[16];
        if(0 == Sget_xcaliburClientMoCAEnabled(mocaEnabled, sizeof(mocaEnabled)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) mocaEnabled,
                                     strlen(mocaEnabled));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_mocaEnabled\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to check whether MoCA is currently in use or not.
 * This received status is stored into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where current
 * status of MoCA is stored.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_mocaInUse(netsnmp_mib_handler *handler,
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
		char mocaInUse[16];
        if(0 == Sget_xcaliburClientMoCAInUse(mocaInUse, sizeof(mocaInUse)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) mocaInUse,
                                     strlen(mocaInUse));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_mocaInUse\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to check whether Ethernet connection is enabled
 * or not. This status is stored into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where status of
 * ethernet connection, whether enabled or not, is stored.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_ethernetEnabled(netsnmp_mib_handler *handler,
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
		char ethEnabled[16];
        if(0 == Sget_xcaliburClientEthernetEnabled(ethEnabled, sizeof(ethEnabled)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) ethEnabled,
                                     strlen(ethEnabled));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_ethernetEnabled\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to check whether Ethernet connection is in use
 * or not. This status is stored into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where status of
 * ethernet connection, whether it is in use or not, is stored.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_ethernetInUse(netsnmp_mib_handler *handler,
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
		char ethInUse[16];
        if(0 == Sget_xcaliburClientEthernetInUse(ethInUse, sizeof(ethInUse)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) ethInUse,
                                     strlen(ethInUse));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_ethernetInUse\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to get the system time of client.
 * This info is stored into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where info of
 * system time is stored.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_sysTime(netsnmp_mib_handler *handler,
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
		char *sysTime; 
		/********************** VL CODE  start*********************/
        if(0 == Sget_xcaliburClientSysTime(&sysTime, sizeof(sysTime)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) sysTime,
                                     strlen(sysTime));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_sysTime\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to get the system time-zone of client.
 * This info is stored into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where info of
 * system time-zone is stored.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_sysTimezone(netsnmp_mib_handler *handler,
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
		SocStbHostSnmpProxy_CardInfo_t lProxyCardInfo;
		char sysTimeZone[16];
		if(0 == Sget_ocStbHostSnmpProxyCardInfo(&lProxyCardInfo))
		{
		    printf("ERROR : Sget_ocStbHostSnmpProxyCardInfo");
		    return SNMP_ERR_GENERR;
		}
		///To get correct time zone , do 2's complement to the comming value
		///logic to implement x,y  x = value; find x -ve | +ve value and y = ~x+1;
		short time2scomplement;
		short timeZone ;
		time2scomplement = 0;
		timeZone  = 0;
		time2scomplement = lProxyCardInfo.ocStbHostCardTimeZoneOffset;
		if(time2scomplement & 0x8000)
		{
		    //add -ve;
		        timeZone = ~time2scomplement;
		        timeZone  = timeZone +1;
		        timeZone = -(timeZone/60);
		}
		else
		{
		        timeZone = (time2scomplement/60);
		}
		snprintf(sysTimeZone, sizeof(sysTimeZone),"%d", timeZone);
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) sysTimeZone,
                                     strlen(sysTimeZone));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_sysTimezone\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to get the offset system time of client.
 * This info is stored into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where info of
 * system time offset is stored.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_sysDSTOffset(netsnmp_mib_handler *handler,
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
		SocStbHostSnmpProxy_CardInfo_t lProxyCardInfo;
		char sysDSTOffset[64];
		if(0 == Sget_ocStbHostSnmpProxyCardInfo(&lProxyCardInfo))
		{
		    printf("ERROR : Sget_ocStbHostSnmpProxyCardInfo");
		    return SNMP_ERR_GENERR;
		}
		snprintf(sysDSTOffset, sizeof(sysDSTOffset), "%ld", (lProxyCardInfo.ocStbHostCardDaylightSavingsTimeExit - lProxyCardInfo.ocStbHostCardDaylightSavingsTimeEntry));
#if 0
        if(0 == Sget_xcaliburClientSysDSTOffset(sysDSTOffset, sizeof(sysDSTOffset)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
#endif
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) sysDSTOffset,
                                     strlen(sysDSTOffset));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_sysDSTOffset\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to get the size of SI cache.
 * This info is stored into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where info of
 * size of SI cache is stored.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_siCacheSize(netsnmp_mib_handler *handler,
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
		char siCacheSize[16];
        if(0 == Sget_xcaliburClientSICacheSize(siCacheSize, sizeof(siCacheSize)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *)siCacheSize,
                                     strlen(siCacheSize));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_siCacheSize\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to get the status of SI load completion.
 * This info is stored into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where status
 * of SI load completion is saved.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_siLoadComplete(netsnmp_mib_handler *handler,
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
		char siLoaded[16];
        if(0 == Sget_xcaliburClientSILoadCompleted(siLoaded, sizeof(siLoaded)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) siLoaded,
                                     strlen(siLoaded));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_siLoadComplete\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to get total number of available DVR tuners
 * This info is stored into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where number of
 * available DVR tuners is saved.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_noDvrTuners(netsnmp_mib_handler *handler,
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
		char numDVRTuners[16];
        if(0 == Sget_xcaliburClientNumberOfDVRTuners(numDVRTuners, sizeof(numDVRTuners)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *)numDVRTuners,
                                     strlen(numDVRTuners));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_noDvrTuners\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to get the free available space in the xcal
 * client disk. This info is stored into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where size of free
 * disk space is saved.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_freeDiskSpace(netsnmp_mib_handler *handler,
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
		char freeDiskSpace[16];
        if(0 == Sget_xcaliburClientFreeDiskSpace(freeDiskSpace, sizeof(freeDiskSpace)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *)freeDiskSpace,
                                     strlen(freeDiskSpace));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_freeDiskSpace\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to get info about cable card like its mac addr.
 * This info is stored into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where mac addr
 * of cable card is saved.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_cableCardMacAddr(netsnmp_mib_handler *handler,
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
        if(0 == Sget_ocStbHostCardInfo(&HostCardobj))
        {
              printf("ERROR:SNMPget_ocStbHostCardInfo");
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     /*(u_char *)*/(const u_char *)HostCardobj.ocStbHostCardMACAddress,
                                     6*sizeof(char));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_cableCardMacAddr\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to get info about HD video capability.
 * This info is stored into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where status of
 * HD capability is saved.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_hdCapable(netsnmp_mib_handler *handler,
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
		char hdCapable[16];
        if(0 == Sget_xcaliburClientHDCapable(hdCapable, sizeof(hdCapable)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *)hdCapable,
                                     strlen(hdCapable));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_hdCapable\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to get info about HD video authorization.
 * This info is stored into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where status of
 * HD authorization is saved.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_hdAuthorized(netsnmp_mib_handler *handler,
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
		char hdAuth[16];
        if(0 == Sget_xcaliburClientHDAuth(hdAuth, sizeof(hdAuth)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) hdAuth,
                                     strlen(hdAuth));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_hdAuthorized\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to check whether mDVR authorization is done.
 * The status in boolean form is stored into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where status of
 * mDVR authorization is saved.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_mdvrAuthorized(netsnmp_mib_handler *handler,
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
		char mDVRAuth[16];
        if(0 == Sget_xcaliburClientmDVRAuth(mDVRAuth, sizeof(mDVRAuth)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *)mDVRAuth,
                                     strlen(mDVRAuth));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_mdvrAuthorized\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to check ready condition of DVR.
 * This state in boolean form is stored into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where status of
 * DVR whether ready or not, is saved.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_dvrReady(netsnmp_mib_handler *handler,
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
		char DVRReady[16];
        if(0 == Sget_xcaliburClientDVRReady(DVRReady, sizeof(DVRReady)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *)DVRReady,
                                     strlen(DVRReady));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_dvrReady\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to check whether DVR has been provisioned or not.
 * This state in boolean form is stored into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where status of
 * provisioned DVR is saved.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_dvrProvisioned(netsnmp_mib_handler *handler,
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
		char DVRProvisioned[16];
        if(0 == Sget_xcaliburClientDVRProvisioned(DVRProvisioned, sizeof(DVRProvisioned)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) DVRProvisioned,
                                     strlen(DVRProvisioned));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_dvrProvisioned\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to check block status of DVR.
 * This state in boolean form is stored into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where block status
 * of DVR is saved.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_dvrNotBlocked(netsnmp_mib_handler *handler,
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
		char DVRNotBlocked[16];
        if(0 == Sget_xcaliburClientDVRNotBlocked(DVRNotBlocked, sizeof(DVRNotBlocked)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *)DVRNotBlocked,
                                     strlen(DVRNotBlocked));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_dvrNotBlocked\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to get the type of network like "barcelona" etc..
 * This data is stored into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where type of
 * network is saved.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_networkType(netsnmp_mib_handler *handler,
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
		char NWType[16];
        if(0 == Sget_xcaliburClientNetworkType(NWType, sizeof(NWType)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) NWType,
                                    strlen(NWType));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_networkType\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to get the version number of network.
 * This value is stored into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where version
 * number of network is saved.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_networkVersion(netsnmp_mib_handler *handler,
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
		char NWVersion[16];
        if(0 == Sget_xcaliburClientNetworkVersion(NWVersion, sizeof(NWVersion)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *)NWVersion,
                                     strlen(NWVersion));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_networkVersion\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to get the version number of specification.
 * This value is stored into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where version
 * number of specification is saved.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_specVersion(netsnmp_mib_handler *handler,
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
		char SpecVersion[16];
        if(0 == Sget_xcaliburClientSpecVersion(SpecVersion, sizeof(SpecVersion)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) SpecVersion,
                                    strlen(SpecVersion));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_specVersion\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to get the status of DVR recording.
 * This value is stored into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where status of
 * DVR recording is saved.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */ 
int
handle_dvrRecording(netsnmp_mib_handler *handler,
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
		char DVRRec[16];
        if(0 == Sget_xcaliburClientDVRRec(DVRRec, sizeof(DVRRec)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) DVRRec,
                                    strlen(DVRRec));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_dvrRecording\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to get the status of authorization for dlna
 * DVR. This value is stored into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where status of
 * authorization for dlna DVR recording is saved.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_dlnadvrAuthorized(netsnmp_mib_handler *handler,
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
		char dlnaDVRAuth[16];
        if(0 == Sget_xcaliburClientDLNADVRAuth(dlnaDVRAuth, sizeof(dlnaDVRAuth)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) dlnaDVRAuth,
                                     strlen(dlnaDVRAuth));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_dlnadvrAuthorized\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to get the status of dlna linear authorization.
 * This value is stored into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where status of
 * dlna linear authorization is saved.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_dlnalinearAuthorized(netsnmp_mib_handler *handler,
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
		char dlnaLinearAuth[16];
        if(0 == Sget_xcaliburClientDLNALinearAuth(dlnaLinearAuth, sizeof(dlnaLinearAuth)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *)dlnaLinearAuth,
                                    strlen(dlnaLinearAuth));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_dlnalinearAuthorized\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to get the version of device software image
 * like its firmware. This value is stored into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where version of
 * device firmware is saved.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_imageVersion(netsnmp_mib_handler *handler,
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
        if(0 == Sget_ocStbHostDeviceSoftware(&gDevSwObj) )
        {
              printf("ERROR:Sget_ocStbHostDeviceSoftware");
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *)gDevSwObj.SFirmwareVersion,
                                     strlen(gDevSwObj.SFirmwareVersion));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_imageVersion\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to get the timestamp for the version of firmware
 * image, when it was generated. The value is stored into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where time-stamp
 * version of device software image is saved.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_imageTimestamp(netsnmp_mib_handler *handler,
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
	{
        string strTimeStamp="";
        string line;
        string strToFind = "Generated on ";
        ifstream versionFile ("/version.txt");
        if (versionFile.is_open())
        {
                while ( getline (versionFile,line) )
                {
                        //cout << line << '\n';
			snmp_log(LOG_ERR, "version.txt line=%s\n", line.c_str());
                        std::size_t found = line.find(strToFind);
                        if (found!=std::string::npos)
                        {
                                strTimeStamp = line.substr(strToFind.length());
				snmp_log(LOG_ERR, "version.txt line=%s\n", strTimeStamp.c_str());
                        }
                }
                versionFile.close();
        }
        else
	{
		snmp_log(LOG_ERR, "Unable to open version.txt file");
	}
        snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) (strTimeStamp.c_str()),
                                     strTimeStamp.length());

#if 0
        if(0 == Sget_ocStbHostDeviceSoftware(&gDevSwObj) )
        {
            printf("ERROR:Sget_ocStbHostDeviceSoftware");
            return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) (gDevSwObj.ocStbHostSoftwareFirmwareReleaseDate),
                                     gDevSwObj.ocStbHostSoftwareFirmwareReleaseDate_len);
#endif
	}
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_imageTimestamp\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to get the status of EID disconnection.
 * This boolean value is stored into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where time-stamp
 * version of device software image is saved.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_eidDisconnect(netsnmp_mib_handler *handler,
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
		char eidDisconnect[16];
        if(0 == Sget_xcaliburClientEIDDisconnect(eidDisconnect, sizeof(eidDisconnect)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *)eidDisconnect,
                                    strlen(eidDisconnect));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_eidDisconnect\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to get the value of HN Ip.
 * This value is stored into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where value of
 * HN Ipis saved.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_hnIp(netsnmp_mib_handler *handler,
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
		char hnIp[16];
        if(0 == Sget_xcaliburClientHNIp(hnIp, sizeof(hnIp)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *)hnIp,
                                     strlen(hnIp));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_hnIp\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to get the value of HN port.
 * This value is stored into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where value of
 * HN port is saved.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_hnPort(netsnmp_mib_handler *handler,
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
		char hnPort[16];
        if(0 == Sget_xcaliburClientHNPort(hnPort, sizeof(hnPort)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *)hnPort,
                                     strlen(hnPort));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_hnPort\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to get the status od HN ready condition.
 * This boolean value is stored into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where HN ready
 * status is saved.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_hnReady(netsnmp_mib_handler *handler,
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
		char hnReady[16];
        if(0 == Sget_xcaliburClientHNReady(hnReady, sizeof(hnReady)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *)hnReady,
                                     strlen(hnReady));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_hnReady\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @brief This function is used to get the value of DVR protocol version.
 * This value is stored into the netsnmp request info structure.
 *
 * @param[in] handler Struct pointer for netsnmp MIB handler.
 * @param[in] reginfo Struct pointer for netsnmp registration handler.
 * @param[in] reqinfo Struct pointer for netsnmp agent request, like MODE_GET etc.
 * @param[out] requests Struct pointer for netsnmp request info where version of
 * DVR protocol is saved.
 *
 * @retval SNMP_ERR_GENERR Returns for error condition.
 * @retval SNMP_ERR_NOERROR Returns for success condition.
 */
int
handle_dvrProtocolVersion(netsnmp_mib_handler *handler,
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
		char dvrProtocolVersion[16];
        if(0 == Sget_xcaliburClientDvrProtocolVersion(dvrProtocolVersion, sizeof(dvrProtocolVersion)))
        {
              printf("ERROR:%s\n", __FUNCTION__);
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *)dvrProtocolVersion ,
                                     strlen(dvrProtocolVersion));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_dvrProtocolVersion\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/**
 * @}
 */
