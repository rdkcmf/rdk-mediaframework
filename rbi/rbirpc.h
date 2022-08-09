/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2014 RDK Management
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

#ifndef _RBIRPC_H
#define _RBIRPC_H

/*
 * The well known name for the RBI RPC API
 */
#define RBI_RPC_NAME "rmfStreamer"

/*
 * The maximum length for a service uri in this API
 */
#define RBI_MAX_SERVICE_URI_LEN (256)

/*
 * The maximum length for a content uri in this API
 */
#define RBI_MAX_CONTENT_URI_LEN (2048)

/*
 * The maximum length for a detailCode Note in RBI_InsertionStatus_Data
 */
#define RBI_MAX_STATUS_DETAILNOTE_LEN (8192)

/*
 * The maximum length for arbitrary contextual data for imminent trigger messaging (see RBI_EVENT_INSERT_POINT_IMMINENT)
 */
#define RBI_MAX_DATA (257)

/*
 * The maximum length for storing stream id
 */
#define RBI_MAX_STREAM_ID (50)

/*
 * The maximum length for multiple POIDs that are contatenated using '|'
 */
#define POID_MAX_DATA (2049)

/*
 * The maximum number of Ad break spots that can be defined with a DefineSpots call.
 */
#define RBI_MAX_SPOTS (10)

/*
 * The maximum total size available for replacement content uri's.  
 */
#define RBI_MAX_URI_DATA (RBI_MAX_CONTENT_URI_LEN)

/*
 * The maximum length of a provider ID
 */
#define RBI_MAX_PROVIDERID_LEN (32)

/*
 * The maximum length of an asset ID
 */
#define RBI_MAX_ASSETID_LEN (32)

/*
 * The maximum number of stream specs that can be supplied for private data monitoring
 */
#define RBI_MAX_STREAMSPECS (5)

/*
 * The maximum length of data to be used for matching PMT descriptors for private data monitoring
 */
#define RBI_MAX_DESCR_DATA_LEN (32)

/*
 * The maximum length of character array for timezone name. ex.AKST09AKDT,M3.2.0,M11.1.0.
 */
#define RBI_MAX_TIMEZONE_NAME_LEN (40)

#define PROVIDER_STR       "urn:comcast:altcon:addressable"
#define MERLIN_STR         "urn:merlin:linear:stream:"
#define ADZONE_STR         "urn:comcast:location:vde-adzone:"
#define SERVICESOURCE_STR  "private:comcast_qam_gateway:"
#define AIR_DATE           "urn:comcast:altcon:airdate:"

#define DISTRIBUTOR         "Distributor"
#define PROVIDER            "Provider"

/*
 * The RBISessionID is an opaque value that can be used to associate events together.  All the events associated with a given
 * tuning session will use the same sessionId.
 */
typedef unsigned char RBISessionID[16];

/*
 * The maximum length of the Tracking ID - note that the trackingID string contains additional info beyond the GUID and must maintain
   the character case between the PlacementResponse and associated PlacementStatusNotification
 */
#define RBI_MAX_TRACKINGID_LEN (2083)

/*
 * // Max 6 tuners per XG
 */
#define RBI_MAX_RECEIVERS 6

//#define IS_LIVEDVR(X, Y) ( (X && Y) ? "LIVE-DVR" : ((X) ? "LIVE" : ((Y) ? "DVR" : "") ))

/*
 * Events published by RBIManager. These events can be received by 
 * registering for them with IARM_Bus_RegisterEventHandler
 */
typedef enum _RBI_Events
{
   // A tuning session has started
   RBI_EVENT_TUNEIN= 10,

   // A tuning session has ended
   RBI_EVENT_TUNEOUT,

   // An insertion opportunity will start in a few seconds
   RBI_EVENT_INSERTION_OPPORTUNITY,

   // Status update for an insertion opportunity
   RBI_EVENT_INSERTION_STATUS,
   
   // Private data
   RBI_EVENT_PRIVATE_DATA,

   // Update for timezone change
   RBI_EVENT_TIMEZONE_CHANGE,

   // Update for curl retry count
   RBI_EVENT_RETRYCOUNT_CHANGE,

   RBI_EVENT_MAX= RBI_EVENT_TUNEIN+10
} RBI_Events;

/*
 * RBI_EVENT_TUNEIN event data
 *
 * sessionId: the rbi session id 
 * deviceId: a value identifying the device (eSTB MAC address)
 * utcTime: the time in UTC milliseconds of the event
 * serviceURI: the uri of the tuned service
 */
typedef struct _RBI_TuneIn_Data
{
   RBISessionID sessionId;
   unsigned char deviceId[6];  // eSTB MAC address
   long long utcTime;       // UTC milliseconds for event time
   char serviceURI[RBI_MAX_SERVICE_URI_LEN];    
} RBI_TuneIn_Data; 

/*
 * RBI_EVENT_TUNEOUT event data
 *
 * sessionId: the rbi session id 
 * deviceId: a value identifying the device (eSTB MAC address)
 * utcTime: the time in UTC milliseconds of the event
 * serivceUri: the uri of the de-tuned service
 */
typedef struct _RBI_TuneOut_Data
{
   RBISessionID sessionId;
   unsigned char deviceId[6];  // eSTB MAC address
   long long utcTime;       // UTC milliseconds for event time
   char serviceURI[RBI_MAX_SERVICE_URI_LEN+1];    
} RBI_TuneOut_Data; 

typedef enum
{
   RBI_DVR=0,
   RBI_LIVE,
   RBI_TSB,
   RBI_LIVE_TSB //rmfStreamer pass  RBI_LIVE_TSB value in removeReceiverId, rbi should remove if LIVE/TSB whichever is available in the list, that may need to remove it.
}RBI_LIVEDVR;

typedef struct _LIVE_DVR {
    int Live_TSB; // 0 - No Live/TSB, 1- Live, 2 - TSB
    bool DVR;
}LiveDVR;

typedef struct _ReceiverActvity
{
   char receiverId[256];
   char activityState[10]; // Active / Inactive
   char tuneTime[15];
   char lastActivityTime[15];
   LiveDVR liveDVRState;
}ReceiverActivity;

typedef struct _Segments
{
   int upidType;
   unsigned char segmentUpid[RBI_MAX_DATA];
}Segments;

typedef struct _SegmentUpidElement
{
   unsigned int totalSegments;
   Segments segments[10];
   unsigned int eventId;
   unsigned int typeId;
   unsigned int segmentNum;
   unsigned int segmentsExpected;
}SegmentUpidElement;


typedef struct _PO_Data
{
   char signalId[RBI_MAX_DATA];
   long long poDuration;
   int poType;
   char streamId[RBI_MAX_DATA];
   unsigned char contentId[RBI_MAX_DATA];
   char serviceSource[RBI_MAX_DATA];
   char zone[RBI_MAX_DATA];
   long long windowStart;
   char elementId[RBI_MAX_DATA];
   char elementBreakId[RBI_MAX_DATA];
   SegmentUpidElement segmentUpidElement;
} PlacementOpportunityData;

/*
 * RBI_EVENT_INSERTION_OPPORTUNITY event data
 *
 * sessionId: the rbi session id 
 * deviceId: a value identifying the device (eSTB MAC address)
 * triggerId: the opportunity trigger id
 * serviceURI: the uri of the tuned service
 * utcTimeCurrent: the current time in UTC milliseconds
 * utcTimeOpportunity: the time in UTC milliseconds of the start of the opportunity
 * len: length of any additional contextual data
 * data: any additional contextual data.
 */
typedef struct _RBI_InsertionOpportunity_Data
{
   RBISessionID sessionId;
   unsigned char deviceId[6];
   int triggerId;
   char serviceURI[RBI_MAX_SERVICE_URI_LEN+1];
   long long utcTimeCurrent;
   long long utcTimeOpportunity;
   ReceiverActivity recActivity[RBI_MAX_RECEIVERS];
   size_t totalReceivers;
   PlacementOpportunityData poData;
#ifdef HAS_AUTHSERVICE
   char xifaId[RBI_MAX_DATA];
   int advtOptOut;
#endif
} RBI_InsertionOpportunity_Data; 

/*
 * RBI_EVENT_INSERTION_STATUS event data
 *
 * sessionId: the rbi session id 
 * deviceId: a value identifying the device (eSTB MAC address)
 * triggerId: the trigger id
 * event: the event type
 * action: the action being performed
 * utcTimeSpot: the time in UTC millisecconds of the start of the spot
 * utcTimeEvent: the time in UTC milliseconds of the event
 * detailCode: further data for insertion failure
 * scale: insertion rate
 * uri: content uri (if action is replace)
 * trackingIdString: user spupplied trackingID String (PSN TrackingId is case sensitive)
 *
 */

typedef enum _RBI_DetailCode
{
   RBI_DetailCode_NoError=0,
   RBI_DetailCode_unableToSplice_missed_prefetchWindow = 1,
   RBI_DetailCode_unableToSplice_missed_spotWindow = 2,
   RBI_DetailCode_invalidVersion= 3,
   RBI_DetailCode_invalidXMLInformation= 4,
   RBI_DetailCode_missingContent=5,
   RBI_DetailCode_placementResponseError=6,
   RBI_DetailCode_assetDurationInadequate=7,
   RBI_DetailCode_assetDurationExceeded=8,
   RBI_DetailCode_httpResponseTimeout=9,
   RBI_DetailCode_missingContentLocation=10,
   RBI_DetailCode_assetUnreachable= 11,
   RBI_DetailCode_matchingAssetNotFound=12,
   RBI_DetailCode_matchingAudioAssetNotFound=13,
   RBI_DetailCode_missingMediaType=14,
   RBI_DetailCode_messageRefMismatch= 15,
   RBI_DetailCode_badMessageRefFormat=16,
   RBI_DetailCode_PRSPAudioAssetNotFound=17,
   RBI_DetailCode_responseTimeout= 18,
   RBI_DetailCode_insertionTimeDeltaExceeded= 19,
   RBI_DetailCode_assetCorrupted= 20,
   RBI_DetailCode_invalidSpot=21,
   RBI_DetailCode_abnormalTermination=22,
   RBI_DetailCode_messageMalformed=23,
   RBI_DetailCode_patPmtChanged=24
} RBI_DetailCode;

typedef enum _RBI_SpotAction
{
   RBI_SpotAction_unknown,
   RBI_SpotAction_fixed,
   RBI_SpotAction_replace
} RBI_SpotAction;

typedef enum _RBI_Spot
{
   RBI_Spot_invalid,
   RBI_Spot_valid,
} RBI_Spot;

typedef enum _RBI_SpotEvent
{
   RBI_SpotEvent_unknown,
   RBI_SpotEvent_endAll,
   RBI_SpotEvent_insertionStarted,
   RBI_SpotEvent_insertionStatusUpdate,
   RBI_SpotEvent_insertionStopped,
   RBI_SpotEvent_insertionFailed
} RBI_SpotEvent;

typedef struct _RBI_InsertionStatus_Data
{
   RBISessionID sessionId;
   unsigned char deviceId[6];        // eSTB MAC address
   int triggerId;
   RBI_SpotEvent event;
   RBI_SpotAction action;   // Action being performed
   long long utcTimeSpot;   // UTC milliseconds for start of spot
   long long utcTimeEvent;  // UTC milliseconds for event time
   int spotNPT;
   int detailCode;          // Further data for insertion failure
   char detailNoteString[RBI_MAX_STATUS_DETAILNOTE_LEN+1]; // StatusDetailCode Note for detailCode above
   char *detailNotePtr;
   float scale;             // Insertion rate: will always be 1.0
   char assetID[RBI_MAX_ASSETID_LEN+1];  // Asset ID (if action is replace)
   char providerID[RBI_MAX_PROVIDERID_LEN+1]; // Provider ID (if action is replace)
   char uri[RBI_MAX_CONTENT_URI_LEN+1];  // Spot content uri (if action is replace)
   char trackingIdString[RBI_MAX_TRACKINGID_LEN+1]; // trackingId String representation of trackingId above
   int  spotDuration;
} RBI_InsertionStatus_Data; 

/*
 * RBI_EVENT_PRIVATE_DATA event data
 *
 * sessionId: the rbi session id 
 * deviceId: a value identifying the device (eSTB MAC address)
 * serviceURI: the uri of the tuned service
 * streamType: the streamType from the PMT for this packet
 * packet: the packet data
 */
typedef struct _RBI_PrivateData_Data
{
   RBISessionID sessionId;
   unsigned char deviceId[6];        // eSTB MAC address
   char serviceURI[RBI_MAX_SERVICE_URI_LEN+1];    
   int streamType;
   unsigned char packet[188];
} RBI_PrivateData_Data; 

/*
 * Timezone_Update event data
 *
 * tzOffset: updates timezone offset value in minutes
   ex: for GMT-5 this value will be -(5*60) = -300.
 */
typedef struct _RBI_LSA_Timezone_Update
{
   int tzOffset;
   char tzName[RBI_MAX_TIMEZONE_NAME_LEN+1];
} RBI_LSA_Timezone_Update;

/*
 * LSA_IARM_EVENT_ENABLE event data
 *
 * lsaStart: Enable LSA if true else ignore 
 */
typedef struct _RBI_LSA_Enable_Data
{
   bool lsaStart;
} RBI_LSA_Enable_Data; 

/*
 * RPC API's published by RBIManager. These API's may be invoked using
 * IARM_Bus_Call.
 *
 * for example:
 *
 *   RBI_DefineSpots_Args args;
 *   . . .
 *   IARM_Bus_Call( RBI_RPC_NAME, RBI_DEFINE_SPOTS, &args, sizeof(args) );
 */
 
/*
 * DefineSpots
 *
 * This call is used to define a set of one or more spots in an Ad break.
 * Each spot has a duration and an action.  The action is either RBI_SpotAction_fixed
 * or RBI_SpotAction_replace.  For RBI_SpotAction_fixed the default Ad that is 
 * broadcast will be left unmodified.  For RBI_SpotAction_replace a replacement Ad
 * will be inserted with content from the supplied uri. 
 *
 * triggerId: the trigger id for which to associate an uri
 * numSpots: the number of Ad spots being defined
 * spots: an array of parameters for each spot
 * uriData: an array holding the uri's for replacement content
 *
 * For each spot (from 0 to numSpots-1) the spots array contains the following
 * parameters:
 *
 * action: the action to take (fixed or replace) for the spot
 * utcTimeSpot: the start time of the spot in utc milliseconds
 * duration: the spot duration in milliseconds
 * uriOffset: the offset into uriData of the uri for replacement content (ignored for action fixed)
 * trackingIdString: string representation of case sensitive trackingId
 * 
 */
typedef struct _RBI_Insert_Spot
{
   RBI_SpotAction action;
   RBI_Spot validSpot;
   int duration;
   char assetID[RBI_MAX_ASSETID_LEN+1];
   char providerID[RBI_MAX_PROVIDERID_LEN+1];
   char trackingIdString[RBI_MAX_TRACKINGID_LEN+1];
   char uriData[RBI_MAX_URI_DATA];
   RBI_DetailCode detailCode;
} RBI_Insert_Spot;

typedef struct _RBI_DefineSpots_Args
{
   RBISessionID sessionId;
   int triggerId;
   long long utcTimeStart;
   int numSpots;
   RBI_Insert_Spot spots[RBI_MAX_SPOTS];
   char uriData[RBI_MAX_URI_DATA];
   RBI_DetailCode detailCode;
} RBI_DefineSpots_Args;

#define RBI_DEFINE_SPOTS "DefineSpots"

/*
 * MonitorPrivateData
 *
 * This call is used to obtain RBI_EVENT_PRIVATE_DATA events with packet data from
 * specified streams.  Streams are specified with a streamType, a descriptor tag, and
 * a descriptor data sequence.  When the PMT for a selected service is parsed, matches
 * will be searched for based on the provided criteria.  For each stream in the PMT,
 * its streamType will be compared to the streamType in each provided stream RBI_StreamSpec.
 * If the streamType matches, and the value of matchTag is true, then descriptor 
 * tag values will be compared to the descrTag value in the RBI_StreamSpec.
 * If matchTag is false, a match is declared if the streamType matches. If the tag
 * value matches and the RBI_StreamSpec has a descrDataLen that is greater than zero,
 * then descrDataLen bytes are compared between the pmt descriptor and the values in
 * the descrData array for the  RBI_StreamSpec.  If this data matches, or if the value
 * of descrDataLen was negative, then a match is declared.  TS packets from the PID
 * associated with this PMT entries that match supplied RBIStreamSpec's will be sent out
 * in RBI_EVENT_PRIVATE_DATA events.  
 * 
 * Note: this mechanism is intended to be used to monitor a small number of very low 
 * bandwidth data streams.
 */
typedef struct _RBI_StreamSpec
{
   unsigned char streamType;
   bool matchTag;
   unsigned char descrTag;
   int descrDataLen;
   unsigned char descrData[RBI_MAX_DESCR_DATA_LEN];
} RBI_StreamSpec;

typedef struct _RBI_MonitorPrivateData_Args
{
   int numStreams;
   RBI_StreamSpec specs[RBI_MAX_STREAMSPECS];   
} RBI_MonitorPrivateData_Args;

#define RBI_MONITOR_PRIVATEDATA "MonitorPrivateData"



#endif

