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
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <map>
#include <string>
#include <time.h>
#include "pthread.h"
#include "libIBus.h"
#include "rbirpc.h"
#include "tinyxml.h"
#include <curl/curl.h>
#include <uuid/uuid.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <string>
#include <fstream>
#include <sstream>
#include "rfcapi.h"

using namespace std;

#define INRANGE(value, min, max) (value < max && value > min)
#define UTCMILLIS(tv) (((tv).tv_sec*1000LL)+((tv).tv_usec/1000LL))

#define IDENTITY_ATTRIBUTE_LENGTH 40

typedef struct _SessionData
{
   RBISessionID sessionId;
   char tunedUri[RBI_MAX_SERVICE_URI_LEN+1];
   int currentTriggerId;
   unsigned char deviceId[6];
   int privateDataPacketCount;
   unsigned char privateDataPid[8192];
   char prspIdentity[IDENTITY_ATTRIBUTE_LENGTH];
   uuid_t preqMessageId;
   uuid_t prspMessageId;
   uuid_t popId;
   uuid_t pdId;
   char poElementId[RBI_MAX_DATA];
   unsigned int duration;
   bool distributorSignal;
} SessionData;

typedef struct _SpotDefinition
{
   RBI_SpotAction action;
   RBI_Spot validSpot;
   int duration;
   char *assetID;
   char *providerID;
   char *uri;
   char *trackingIdString;
} SpotDefinition;

typedef struct _TriggerDefinition
{
   int id;
   int numSpots;
   char *service;
   SpotDefinition spots[RBI_MAX_SPOTS];
} TriggerDefinition;

static bool gInitialized= false;
static bool g_ContinueMainLoop= true;
static pthread_mutex_t g_sessionMutex= PTHREAD_MUTEX_INITIALIZER;
static std::map<std::string,SessionData*> g_sessionDataMap;
static bool g_showARMTuning= false;
static bool g_showARMPrivateData= false;
static bool g_showARMOpportunity= false;
static bool g_showARMStatus= false;
static int g_postTimeout= 5000;
static char* g_endPointRequest= 0;
static char* g_endPointStatus= 0;
static char* g_preqMessageId =0;
static char* g_popId =0;
static int   g_serviceId =-1;
static bool g_PlacementResponseReceived = false;
static bool g_PSNError = false;
static unsigned char g_deviceId[6];
static std::string g_failureReason;
static std::string g_PReqtime;
static std::string g_PResptime;
static std::string g_PlacementReqMsg;
static char g_sessionId[64];
static bool g_PRespError = false;
static bool g_PRespSpotError = false;
static int g_tzOffset =0;
static int g_retryCount =0;
static char g_tzName[RBI_MAX_TIMEZONE_NAME_LEN+1];
RFC_ParamData_t ALTCON_RECEIVER,
                SCHEMAS_NGOD,
                SCHEMA_LOCATION,
                XMLNS_SCHEMA,
                XMLSCHEMA_INSTANCE,
                SCHEMAS_ADMIN,
                SCHEMAS_CORE;

static void setupExit(void);
static void sighandler(int signum);
static void dumpPacket( unsigned char *packet, int packetSize );
static void startUp();
static void shutDown();
static SessionData *getSessionData( RBISessionID sessionId );
static void destroySession( RBISessionID sessionId );
static bool configurePrivateData();
static void tuneInHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
static void tuneOutHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
static void opportunityHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
static void insertionStatusHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
static void updateTimezone(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
static void setRetryCount(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
static void privateDataHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
static bool parseTimeString( const char *s, bool useUtc, long long *utcTime);
static bool parseXmlPREQPlayPositionStart( TiXmlElement *pElement, SessionData *session, RBI_InsertionOpportunity_Data *opportunity );
static bool parseXmlPREQPlayPositionsAvailBinding( TiXmlElement *pElement, SessionData *session, RBI_InsertionOpportunity_Data *opportunity );
static bool parseXmlPREQCoreExt( TiXmlElement *pElement, SessionData *session, RBI_InsertionOpportunity_Data *opportunity );
static bool parseXmlPREQContent( TiXmlElement *pElement, SessionData *session, RBI_InsertionOpportunity_Data *opportunity );
static bool parseXmlPREQCoreEntertainment( TiXmlElement *pElement, SessionData *session, RBI_InsertionOpportunity_Data *opportunity );
static bool parseXmlPREQSystemContext( TiXmlElement *pElement, SessionData *session, RBI_InsertionOpportunity_Data *opportunity );
static bool parseXmlPREQService( TiXmlElement *pElement, SessionData *session, RBI_InsertionOpportunity_Data *opportunity );
static bool parseXmlPREQClient( TiXmlElement *pElement, SessionData *session, RBI_InsertionOpportunity_Data *opportunity );
static bool parseXmlPREQPlacementOpportunity( TiXmlElement *pElement, SessionData *session, RBI_InsertionOpportunity_Data *opportunity );
static bool parseXmlPREQPlacementRequest( TiXmlElement *pElement, SessionData *session, RBI_InsertionOpportunity_Data *opportunity );
static bool parseXmlPRSPContent( TiXmlElement *pElement, SessionData *session, RBI_DefineSpots_Args *spotDfn, 
                                 int *duration, char *assetID, char *providerID, char *uri, char *trackingIdString, bool* assetFound, RBI_SpotAction action );
static bool parseXmlPRSPPlacement( TiXmlElement *pElement, SessionData *session, RBI_DefineSpots_Args *spotDfn, int index );
static bool parseXmlPRSPPlayPositionStart( TiXmlElement *pElement, SessionData *session, RBI_DefineSpots_Args *spotDfn );
static bool parseXmlPRSPPlayPositionsAvailBinding( TiXmlElement *pElement, SessionData *session, RBI_DefineSpots_Args *spotDfn );
static bool parseXmlPRSPCoreExt( TiXmlElement *pElement, SessionData *session, RBI_DefineSpots_Args *spotDfn );
static bool parseXmlPREQOpportunityConstraints( TiXmlElement *pElement, SessionData *session );
static bool parseDurationString( const char *s);
static bool parseXmlPRSPPlacementDecision( TiXmlElement *pElement, SessionData *session, RBI_DefineSpots_Args *spotDfn );
static bool parseXmlPRSPClient( TiXmlElement *pElement, SessionData *session, RBI_DefineSpots_Args *spotDfn );
static bool parseXmlPRSPSystemContext( TiXmlElement *pElement, SessionData *session, RBI_DefineSpots_Args *spotDfn );
static bool parseXmlPRSPPlacementResponse( TiXmlElement *pElement, SessionData *session, RBI_DefineSpots_Args *spotDfn );
static SessionData *getSessionFromPlacementResponse( std::string *msg );
static void handleSCTE130PlacementRequest( SessionData *session, std::string *msg );
static void handleSCTE130PlacementResponse( std::string *msg );
static void handleSCTE130PlacementStatus( SessionData *session, std::string *msg );
static char* formatTime( long long utcTime, bool useUtc, char *buffer );
static const int getDetailCode( int detailCode );
static std::string* createPlacementRequest( SessionData *session, RBI_InsertionOpportunity_Data *opportunity );
static std::string* createPlacementStatusNotification( SessionData *session, RBI_InsertionStatus_Data *status, bool placementResponseTimeout );
static int postWriteCallback(char *data, size_t size, size_t nmemb, std::string *responseData );
static int postMessage( const char *endPoint, std::string *msg, std::string *response, bool isPReq, bool bRetry );
static void showUsage();

size_t b64_encoded_size(size_t inlen)
{
    size_t ret;

    ret = inlen;
    if (inlen % 3 != 0)
        ret += 3 - (inlen % 3);
    ret /= 3;
    ret *= 4;

    return ret;
}

/*
** encode
**
** base64-encode a stream adding padding and line breaks as per spec.
 * Assume that output is large enough to fill out last 4-byte boundary.
*/
void base64_encode( const uint8_t *input, const size_t input_size, uint8_t *output )
{
    size_t remsize = input_size;    // (Note: could just use input_size were it not const)
    while (remsize > 0) {
        int i;
        uint32_t tmp;
        int len = (remsize >= 3) ? 3 : remsize;
        remsize -= len;
        tmp = 0;
        for (i=0; i < 3; ++i) {
            tmp = (tmp << 8) | *input++;
        }
        for (i=4; --i >= 0; ) {
            int index = tmp & 0x3F;
            // Note: per earlier code, we only use '=' in at most the last 2 output bytes
            //  The following comparison should still do this (len is 1-3)
            output[i] =   (i > len) ? '='   // use '=' for chars past end of input
                        : (index < 26) ? (index + 'A')      //  0-25 = 'A-Z'
                        : (index < 52) ? (index + ('a'-26)) // 26-51 = 'a-z'
                        : (index < 62) ? (index + ('0'-52)) // 52-61 = '0-9'
                        : (index == 62) ? '+'               //    62 = '+'
                        : '/';                              //    63 = '/'
            tmp >>= 6;
        }
        output += 4;
    }
}

static void setupExit(void)
{
    // cannot catch SIGKILL or SIGSTOP
    signal(SIGINT,  sighandler); // CTRL-C
    signal(SIGQUIT, sighandler);
    signal(SIGILL,  sighandler);
    signal(SIGABRT, sighandler);
    signal(SIGFPE,  sighandler);
    signal(SIGSEGV, sighandler);
    signal(SIGTERM, sighandler);
    signal(SIGPIPE, sighandler);
    atexit(shutDown);
    gInitialized= true;
}

static void sighandler(int signum)
{
    printf("-----------------------------------------------------------\n");
    sleep(1); // allow other threads to print
    switch (signum) 
    {
#define ONCASE(x) case x: printf("Caught signal %s in %s:%d\n", #x, __FILE__,__LINE__); break
        ONCASE(SIGINT);     /* CTRL-C   */
        ONCASE(SIGQUIT);    /* CTRL-\   */
        ONCASE(SIGILL);
        ONCASE(SIGABRT);    /* abort()  */
        ONCASE(SIGFPE);
        ONCASE(SIGSEGV);
        ONCASE(SIGTERM);
        ONCASE(SIGPIPE);
#undef ONCASE
        default: printf("Caught unknown signal %d in %s:%d\n", signum, __FILE__, __LINE__); break;
    }
    signal(signum, SIG_DFL);    // restore default handler

    // send the signal to the default signal handler, to allow a debugger
    // to trap it
    if (signum == SIGINT)
    {
       printf("SIGINT - don't call kill\n");
       g_ContinueMainLoop= false;
    }
    else
    {
       // perform a graceful cleanup
       if (gInitialized)
       {
           shutDown();
       }
       else
       {
           printf("Signal caught during shutDown()\n");
       }

       kill(getpid(), signum);     // invoke default handler
    }
}

static void dumpPacket( unsigned char *packet, int packetSize )
{
   int i;
   char buff[1024];   
   
   int col= 0;
   int buffPos= 0;
   buffPos += snprintf(&buff[buffPos], sizeof(buff), "\n" );
   for( i= 0; i < packetSize; ++i )
   {
       buffPos += snprintf(&buff[buffPos], sizeof(buff)-buffPos, "%02X ", packet[i] );
       ++col;
       if ( col == 8 )
       {
          strncat( buff, " ", 1);
          buffPos += 1;
       }
       if ( col == 16 )
       {
          buffPos += snprintf(&buff[buffPos], sizeof(buff)-buffPos,"\n" );
          col= 0;
       }
   }
   printf( "%s\n", buff );
}

static void startUp()
{
   IARM_Bus_Init("rbiDaemon");
   
   IARM_Bus_Connect();
   
   // Register for rbiManager events
   IARM_Bus_RegisterEventHandler(RBI_RPC_NAME, RBI_EVENT_TUNEIN, (IARM_EventHandler_t)tuneInHandler);
   IARM_Bus_RegisterEventHandler(RBI_RPC_NAME, RBI_EVENT_TUNEOUT, (IARM_EventHandler_t)tuneOutHandler);
   IARM_Bus_RegisterEventHandler(RBI_RPC_NAME, RBI_EVENT_INSERTION_OPPORTUNITY, (IARM_EventHandler_t)opportunityHandler);
   IARM_Bus_RegisterEventHandler(RBI_RPC_NAME, RBI_EVENT_INSERTION_STATUS, (IARM_EventHandler_t)insertionStatusHandler);
   IARM_Bus_RegisterEventHandler(RBI_RPC_NAME, RBI_EVENT_TIMEZONE_CHANGE, (IARM_EventHandler_t)updateTimezone);
   IARM_Bus_RegisterEventHandler(RBI_RPC_NAME, RBI_EVENT_RETRYCOUNT_CHANGE, (IARM_EventHandler_t)setRetryCount);

   if ( g_showARMPrivateData )
   {
      IARM_Bus_RegisterEventHandler(RBI_RPC_NAME, RBI_EVENT_PRIVATE_DATA, (IARM_EventHandler_t)privateDataHandler);

      // Setup private data monitoring
      configurePrivateData();
   }
   gInitialized= true;
}

static void shutDown()
{
   if ( gInitialized )
   {
      gInitialized= false;
      IARM_Bus_Disconnect();
      IARM_Bus_Term();
   }
}

SessionData *getSessionData( RBISessionID sessionId )
{
   SessionData *sessionData= 0;
   char work[64];

   uuid_unparse(sessionId, work);
   
   {
      pthread_mutex_lock( &g_sessionMutex );

      std::map<std::string, SessionData*>::iterator it= g_sessionDataMap.find( std::string(work) );
      if ( it == g_sessionDataMap.end() )
      {
         sessionData= (SessionData*)calloc( 1, sizeof(SessionData) );
         if ( sessionData )
         {
            memcpy( sessionData->sessionId, sessionId, sizeof(RBISessionID) );
                    
            g_sessionDataMap.insert( std::pair<std::string,SessionData*>(std::string(work),sessionData) );
         }
      }
      else
      {
         sessionData= (SessionData*)it->second;
      }

      pthread_mutex_unlock( &g_sessionMutex );
   }   
   
   return sessionData;
}

static void destroySession( RBISessionID sessionId )
{
   char work[64];

   uuid_unparse(sessionId, work);

   {
      pthread_mutex_lock( &g_sessionMutex );
      
      std::map<std::string, SessionData*>::iterator it= g_sessionDataMap.find( work );
      if ( it != g_sessionDataMap.end() )
      {
         SessionData *sessionData= (SessionData*)it->second;
         
         g_sessionDataMap.erase( std::string(work) );         
         
         free( sessionData );
      }
      
      pthread_mutex_unlock( &g_sessionMutex );
   }
}

bool configurePrivateData()
{
   bool result= false;
   IARM_Result_t iarmrc;
   
   // Setup private data stream specifications
   RBI_MonitorPrivateData_Args mon;
   
   mon.numStreams= 4;
   
   mon.specs[0].streamType= 0x05;
   mon.specs[0].matchTag= true;
   mon.specs[0].descrTag= 0xBF;
   mon.specs[0].descrDataLen= 6;
   strcpy( (char*)mon.specs[0].descrData, (const char*)"Invidi" );

   mon.specs[1].streamType= 0xC0;
   mon.specs[1].matchTag= true;
   mon.specs[1].descrTag= 0xBF;
   mon.specs[1].descrDataLen= 6;
   strcpy( (char*)mon.specs[1].descrData, (const char*)"Invidi" );

   mon.specs[2].streamType= 0x05;
   mon.specs[2].matchTag= true;
   mon.specs[2].descrTag= 0xC0;
   mon.specs[2].descrDataLen= 6;
   strcpy( (char*)mon.specs[2].descrData, (const char*)"Invidi" );

   mon.specs[3].streamType= 0xC0;
   mon.specs[3].matchTag= true;
   mon.specs[3].descrTag= 0xC0;
   mon.specs[3].descrDataLen= 6;
   strcpy( (char*)mon.specs[3].descrData, (const char*)"Invidi" );
   
   // Enable to receive raw SCTE-35 packets
   #if 1
   mon.numStreams= 5;      
   mon.specs[4].streamType= 0x86;
   mon.specs[4].matchTag= false;
   mon.specs[4].descrDataLen= 0;
   #endif

   printf( "sending private data stream specs...\n" );
   
   iarmrc= IARM_Bus_Call( RBI_RPC_NAME, RBI_MONITOR_PRIVATEDATA, &mon, sizeof(mon) );
   if ( iarmrc == IARM_RESULT_SUCCESS )
   {
      result= true;
   }
   
   return result;
}

void tuneInHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
{
   RBI_TuneIn_Data *tuneIn= (RBI_TuneIn_Data*)data;
   char sessionId[64];

   if ( g_showARMTuning )
   {
      uuid_unparse( tuneIn->sessionId, sessionId );
      printf( "-----------------------------------------------------------------------------\n" );
      printf( "RBI_EVENT_TUNEIN event received:\n  sessionId %s deviceId %02X:%02X:%02X:%02X:%02X:%02X utcTime: %llu (%d) uri %s\n\n", 
              sessionId, 
              tuneIn->deviceId[0],
              tuneIn->deviceId[1],
              tuneIn->deviceId[2],
              tuneIn->deviceId[3],
              tuneIn->deviceId[4],
              tuneIn->deviceId[5],
              tuneIn->utcTime,
              g_tzOffset,
              tuneIn->serviceURI );
   }   
   
   SessionData *sessionData= getSessionData( tuneIn->sessionId );
   if ( sessionData )
   {
      strcpy( sessionData->tunedUri, tuneIn->serviceURI ); 
   }
}

void tuneOutHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
{
   RBI_TuneOut_Data *tuneOut= (RBI_TuneOut_Data*)data;
   char sessionId[64];
   
   if ( g_showARMTuning )
   {
      uuid_unparse( tuneOut->sessionId, sessionId );
      printf( "-----------------------------------------------------------------------------\n" );
      printf( "RBI_EVENT_TUNEOUT event received:\n  sessionId %s deviceId %02X:%02X:%02X:%02X:%02X:%02X utcTime: %llu (%d) uri %s\n\n", 
              sessionId, 
              tuneOut->deviceId[0],
              tuneOut->deviceId[1],
              tuneOut->deviceId[2],
              tuneOut->deviceId[3],
              tuneOut->deviceId[4],
              tuneOut->deviceId[5],
              tuneOut->utcTime,
              g_tzOffset,
              tuneOut->serviceURI );
   }
   
   destroySession( tuneOut->sessionId );
}

void opportunityHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
{
   RBI_InsertionOpportunity_Data *opportunity= (RBI_InsertionOpportunity_Data*)data;
   char sessionId[64];

   if ( g_showARMOpportunity )
   {
      uuid_unparse( opportunity->sessionId, sessionId );
      printf( "-----------------------------------------------------------------------------\n" );
      printf( "RBI_EVENT_INSERTION_OPPORTUNITY event received:\n  sessionId %s triggerId %X service (%s) deviceId %02X:%02X:%02X:%02X:%02X:%02X now %llu start %llu\n",
              sessionId, 
              opportunity->triggerId, 
              opportunity->serviceURI,
              opportunity->deviceId[0],
              opportunity->deviceId[1],
              opportunity->deviceId[2],
              opportunity->deviceId[3],
              opportunity->deviceId[4],
              opportunity->deviceId[5],
              opportunity->utcTimeCurrent, 
              opportunity->utcTimeOpportunity );

      printf( "poDuration %lld poType %d poElementId %s poElementBreakId %s streamId %s contentId %s serviceSource %s elementId %s\n", 
              opportunity->poData.poDuration,
              opportunity->poData.poType,
              opportunity->poData.elementId,
              opportunity->poData.elementBreakId,
              opportunity->poData.streamId,
              opportunity->poData.contentId,
              opportunity->poData.serviceSource,
              opportunity->poData.elementId,
              opportunity->poData.elementBreakId );

      printf("eventId %d typeId %d segmentNum %d segmentsExpected %d totalSegments %d\n",
              opportunity->poData.segmentUpidElement.eventId,
              opportunity->poData.segmentUpidElement.typeId,
              opportunity->poData.segmentUpidElement.segmentNum,
              opportunity->poData.segmentUpidElement.segmentsExpected,
              opportunity->poData.segmentUpidElement.totalSegments);

      for(unsigned int i=0; i < opportunity->poData.segmentUpidElement.totalSegments; i++)
      {
         printf("SCTE-35 upidType %d %s\n",
            opportunity->poData.segmentUpidElement.segments[i].upidType,
            reinterpret_cast<const char*>(opportunity->poData.segmentUpidElement.segments[i].segmentUpid) );
      }
   }
   
   SessionData *sessionData= getSessionData( opportunity->sessionId );
   if ( sessionData )
   {
      sessionData->currentTriggerId= opportunity->triggerId;
      
         std::string *msg= createPlacementRequest( sessionData, opportunity );
         g_PlacementReqMsg = *msg;
         if ( msg )
         {
            if(opportunity->poData.segmentUpidElement.typeId == 0x36)
               sessionData->distributorSignal = true;
            else
               sessionData->distributorSignal = false;

            handleSCTE130PlacementRequest( sessionData, msg );
            delete msg;
            msg = 0;
         }
      }
}

void updateTimezone(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
{
   printf(" RBI_EVENT_TIMEZONE_CHANGE event received:\n");
   RBI_LSA_Timezone_Update *timezone= (RBI_LSA_Timezone_Update*)data;
   g_tzOffset = timezone->tzOffset;
   strcpy(g_tzName,timezone->tzName);
   setenv( "TZ",g_tzName,1);
   tzset();
}

void setRetryCount(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
{
   int count = *((int *)data);
   g_retryCount = count;
}

void insertionStatusHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
{
   RBI_InsertionStatus_Data *status= (RBI_InsertionStatus_Data*)data;
   char sessionId[64];
   
   if ( g_showARMStatus )
   {
      uuid_unparse( status->sessionId, sessionId );
      printf( "-----------------------------------------------------------------------------\n" );
      printf("RBI_EVENT_INSERTION_STATUS event received:\n  sessionId %s, triggeId %X deviceId %02X:%02X:%02X:%02X:%02X:%02X\n", 
              sessionId, 
              status->triggerId,
              status->deviceId[0],
              status->deviceId[1],
              status->deviceId[2],
              status->deviceId[3],
              status->deviceId[4],
              status->deviceId[5] );

      switch( status->event )
      {
         case RBI_SpotEvent_insertionStarted:
            printf( "  insertion STARTED: action %s start time %llu uri(%s)\n",
                    status->action == RBI_SpotAction_fixed ? "FIXED" : "REPLACE",
                    status->utcTimeSpot,
                    status->action == RBI_SpotAction_fixed ? "none" : status->uri );
            break;
         case RBI_SpotEvent_insertionStatusUpdate:
            printf( "  insertion Status Update: action %s start time %llu uri(%s)\n",
                    status->action == RBI_SpotAction_fixed ? "FIXED" : "REPLACE",
                    status->utcTimeSpot,
                    status->action == RBI_SpotAction_fixed ? "none" : status->uri );
            break;
         case RBI_SpotEvent_insertionStopped:
            printf( "  insertion STOPPED: action %s start time %llu stop time %llu uri(%s)\n",
                    status->action == RBI_SpotAction_fixed ? "FIXED" : "REPLACE",
                    status->utcTimeSpot,
                    status->utcTimeEvent,
                    status->action == RBI_SpotAction_fixed ? "none" : status->uri );
            break;
         case RBI_SpotEvent_insertionFailed:
            printf( "  insertion FAILED: code %d action %s start time %llu error time %llu uri(%s)\n",
                    status->detailCode,
                    status->action == RBI_SpotAction_fixed ? "FIXED" : "REPLACE",
                    status->utcTimeSpot,
                    status->utcTimeEvent,
                    status->action == RBI_SpotAction_fixed ? "none" : status->uri );
            break;
         case RBI_SpotEvent_endAll:
            printf( "  RBI_SpotEvent_endAll, event received : start time %llu error time %llu uri(%s)\n",
                    status->utcTimeSpot,
                    status->utcTimeEvent,
                    status->action == RBI_SpotAction_fixed ? "none" : status->uri );
            break;
         default:
            printf("  event %d - unknown event type\n", status->event );
            break;
      }
      printf("\n");
   }
   SessionData *sessionData= getSessionData( status->sessionId );
   bool placementResponseTimeout = false;
   if ( sessionData )
   {
      if (( g_PSNError != true ) || (( g_PSNError == true ) && (status->event == RBI_SpotEvent_insertionStatusUpdate)))
      {
         if(strlen(status->detailNoteString) > 0)
         {
            status->detailNotePtr = status->detailNoteString;
         }

         if (status->detailCode == RBI_DetailCode_responseTimeout )
         {
            placementResponseTimeout = true;
            printf("RBI LSA-PRE: Timeout/Opportunity Expired\n");
         }
         std::string *msg= createPlacementStatusNotification( sessionData, status, placementResponseTimeout );
         if ( msg )
         {
            handleSCTE130PlacementStatus( sessionData, msg );
            delete msg;
            msg = 0;
         }
         if ((placementResponseTimeout == true ) && (g_PlacementResponseReceived == true))
         {
            placementResponseTimeout = false;
            RBI_InsertionStatus_Data estatus;
            memset( &estatus, 0, sizeof(RBI_InsertionStatus_Data) );
            estatus.action = RBI_SpotAction_replace;
            estatus.event = RBI_SpotEvent_insertionFailed;
            estatus.detailCode = RBI_DetailCode_responseTimeout;
            memcpy( estatus.deviceId, g_deviceId, sizeof(g_deviceId) );
            uuid_t sessionId;
            uuid_parse( g_sessionId, sessionId );
            SessionData *session = getSessionData( sessionId );
            if ( session )
            {
               std::string str  = "Placement Response received too late. PlacementResponse message id= ";
               char work[256];
               uuid_unparse( session->prspMessageId, work );
               str.append(work);
               str.append(", timereceived= ");
               str.append(g_PResptime);
               if(str.size() > 0 )
               {
                  estatus.detailNotePtr = (char*)calloc(str.size()+1, sizeof(char));
                  if(estatus.detailNotePtr)
                  {
                     strncpy(estatus.detailNotePtr, str.c_str(), str.size());
                     estatus.detailNotePtr[str.size()]='\0';
                  }
                  else
                     printf("insertionStatusHandler:: Error memory not allocated. Req memory %d\n", str.size()+1);
               }
               std::string *msg= createPlacementStatusNotification( session, &estatus,false );
               if ( msg )
               {
                  handleSCTE130PlacementStatus( session, msg );
                  delete msg;
                  msg = 0;
               }
               if(estatus.detailNotePtr)
               {
                  free(estatus.detailNotePtr);
                  estatus.detailNotePtr = NULL;
               }
            }
         }
      }
   }
}

void privateDataHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
{
   RBI_PrivateData_Data *privateData= (RBI_PrivateData_Data*)data;
   char sessionId[64];

   SessionData *sessionData= getSessionData( privateData->sessionId );
   if ( sessionData )
   {
      bool displayPacket= false;
      unsigned char *packet= privateData->packet;
      int pid= (((packet[1] << 8) | packet[2]) & 0x1FFF);

      if ( !sessionData->privateDataPid[pid] )
      {
         displayPacket= true;
         sessionData->privateDataPid[pid]= 1;
      }

      if ( g_showARMPrivateData )
      {
         if ( (sessionData->privateDataPacketCount % 100) == 0 )
         {
            displayPacket= true;
         }
         
         if ( displayPacket )
         {
            uuid_unparse( privateData->sessionId, sessionId );
            printf( "-----------------------------------------------------------------------------\n" );
            printf("RBI_EVENT_PRIVATE_DATA event receive:\n  sessionId %s deviceId %02X:%02X:%02X:%02X:%02X:%02X streamType %02X packetCount %d:\n", 
                   sessionId, 
                   privateData->deviceId[0],
                   privateData->deviceId[1],
                   privateData->deviceId[2],
                   privateData->deviceId[3],
                   privateData->deviceId[4],
                   privateData->deviceId[5],
                   privateData->streamType,
                   sessionData->privateDataPacketCount+1 ); 
            dumpPacket( privateData->packet, sizeof(privateData->packet) );      
         }
      }      
      
      ++sessionData->privateDataPacketCount;
   }
}

bool parseTimeString( const char *s, bool useUtc, long long *utcTime)
{
   bool result= false;
   struct tm time;
   int year, month, day;
   int hour, minute, second, millis;
   int tzhour, tzminute;
   time_t t;
   
   if ( sscanf( s, "%d-%d-%dT%d:%d:%d.%d%d:%d",
        &year,
        &month,
        &day,
        &hour,
        &minute,
        &second,
        &millis,
        &tzhour,
        &tzminute ) == 9 )
   {
      time.tm_sec= second;
      time.tm_min= minute;
      time.tm_hour= hour;
      time.tm_mday= day;
      time.tm_mon= month-1;
      time.tm_year= year-1900;
      time.tm_isdst= -1;
      
      if (useUtc)
      {
         t= timegm( &time );
      }
      else
      {
         t= mktime( &time );
      }
      *utcTime= t*1000LL + millis;
      
      result= true;
   }
   return result;
}

bool parseXmlPREQPlayPositionStart( TiXmlElement *pElement, SessionData *session, RBI_InsertionOpportunity_Data *opportunity )
{
   bool result= false;
   
   if ( opportunity && pElement )
   {
      result= true;

      TiXmlAttribute* pAttrib=pElement->FirstAttribute();
      while (pAttrib)
      {
         const char *name= pAttrib->Name();
         const char *value= pAttrib->Value();
               
         printf( "parseXmlPREQPlayPositionStart: ignoring unknown cmcst:PlayPositionStart attribute %s: value=[%s]\n",  name, value);
         
         pAttrib=pAttrib->Next();
      }
   }

   if ( result )
   {
      TiXmlNode* pChild;
      for ( pChild = pElement->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) 
      {
         const char *childName= pChild->Value();
         
         if ( strcmp( childName, "cmcst:SignalId" ) == 0 )
         {
            const char *text= pChild->ToElement()->GetText();

            int len = strlen(text);
            if ( len > RBI_MAX_DATA )
            {
               printf("parseXmlPREQPlayPositionStart: warning: truncating signalId value from %d to %d bytes\n", len, RBI_MAX_DATA );
               len= RBI_MAX_DATA;
            }
            strncpy( (char *)opportunity->poData.signalId, text, len );
            opportunity->poData.signalId[len]= '\0';
         }
         else if ( strcmp( childName, "cmcst:UTCTime" ) == 0 )
         {
            long long utcTimeOpportunity;
            const char *text= pChild->ToElement()->GetText();

            if ( !parseTimeString(text, true, &utcTimeOpportunity) )
            {
               printf("parseXmlPREQPlayPositionStart: bad cmcst:UTCTime value: %s\n", text);
               result= false;
            }
            opportunity->utcTimeOpportunity= utcTimeOpportunity;
         }
         else
         {
            printf( "parseXmlPREQPlayPositionStart: ignoring unknown cmcst:PlayPositionStart child Element [%s]\n", childName );
         }
      }
   }

   return result;
}

bool parseXmlPREQPlayPositionsAvailBinding( TiXmlElement *pElement, SessionData *session, RBI_InsertionOpportunity_Data *opportunity )
{
   bool result= false;
   
   if ( opportunity && pElement )
   {
      result= true;

      TiXmlAttribute* pAttrib=pElement->FirstAttribute();
      while (pAttrib)
      {
         const char *name= pAttrib->Name();
         const char *value= pAttrib->Value();
               
         printf( "parseXmlPREQPlayPositionsAvailBinding: ignoring unknown cmcst:PlayPositionsAvailBinding attribute %s: value=[%s]\n",  name, value);
         
         pAttrib=pAttrib->Next();
      }
   }

   if ( result )
   {
      TiXmlNode* pChild;
      for ( pChild = pElement->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) 
      {
         const char *childName= pChild->Value();
         
         if ( strcmp( childName, "cmcst:PlayPositionStart" ) == 0 )
         {
            result= parseXmlPREQPlayPositionStart( pChild->ToElement(), session, opportunity );
         }
         else
         {
            printf( "parseXmlPREQPlayPositionsAvailBinding: ignoring unknown cmcst:PlayPositionsAvailBinding child Element [%s]\n", childName );
         }
      }
   }

   return result;
}

bool parseXmlPREQCoreExt( TiXmlElement *pElement, SessionData *session, RBI_InsertionOpportunity_Data *opportunity )
{
   bool result= false;
   
   if ( opportunity && pElement )
   {
      result= true;

      TiXmlAttribute* pAttrib=pElement->FirstAttribute();
      while (pAttrib)
      {
         const char *name= pAttrib->Name();
         const char *value= pAttrib->Value();
               
         printf( "parseXmlPREQCoreExt: ignoring unknown core:Ext attribute %s: value=[%s]\n",  name, value);
         
         pAttrib=pAttrib->Next();
      }
   }

   if ( result )
   {
      TiXmlNode* pChild;
      for ( pChild = pElement->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) 
      {
         const char *childName= pChild->Value();
         
         if ( strcmp( childName, "cmcst:PlayPositionsAvailBinding" ) == 0 )
         {
            result= parseXmlPREQPlayPositionsAvailBinding( pChild->ToElement(), session, opportunity );
         }
         else
         {
            printf( "parseXmlPREQCoreExt: ignoring unknown core:Ext child Element [%s]\n", childName );
         }
      }
   }

   return result;
}

bool parseXmlPREQContent( TiXmlElement *pElement, SessionData *session, RBI_InsertionOpportunity_Data *opportunity )
{
   bool result= false;
   
   if ( opportunity && pElement )
   {
      result= true;

      TiXmlAttribute* pAttrib=pElement->FirstAttribute();
      while (pAttrib)
      {
         const char *name= pAttrib->Name();
         const char *value= pAttrib->Value();

         pAttrib=pAttrib->Next();
      }
   }

   if ( result )
   {
      TiXmlNode* pChild;
      for ( pChild = pElement->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) 
      {
         const char *childName= pChild->Value();
         
         if ( strcmp( childName, "core:SegmentationUpid" ) != 0 )
         {
            printf( "parseXmlPREQPlayPositionsAvailBinding: ignoring unknown core:SegmentationUpid child Element [%s]\n", childName );
         }
      }
   }

   return result;
}

bool parseXmlPREQCoreEntertainment( TiXmlElement *pElement, SessionData *session, RBI_InsertionOpportunity_Data *opportunity )
{
   bool result= false;
   
   if ( opportunity && pElement )
   {
      result= true;

      TiXmlAttribute* pAttrib=pElement->FirstAttribute();
      while (pAttrib)
      {
         const char *name= pAttrib->Name();
         const char *value= pAttrib->Value();
               
         printf( "parseXmlPREQCoreEntertainment: ignoring unknown core:Entertainment attribute %s: value=[%s]\n",  name, value);
         
         pAttrib=pAttrib->Next();
      }
   }

   if ( result )
   {
      TiXmlNode* pChild;
      for ( pChild = pElement->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) 
      {
         const char *childName= pChild->Value();
         
         if ( strcmp( childName, "core:Content" ) == 0 )
         {
            result= parseXmlPREQContent( pChild->ToElement(), session, opportunity );
         }
         else
         {
            printf( "parseXmlPREQCoreExt: ignoring unknown core:Content child Element [%s]\n", childName );
         }
      }
   }

   return result;
}


bool parseXmlPREQSystemContext( TiXmlElement *pElement, SessionData *session, RBI_InsertionOpportunity_Data *opportunity )
{
   bool result= false;
   
   if ( opportunity && pElement )
   {
      result= true;

      TiXmlAttribute* pAttrib=pElement->FirstAttribute();
      while (pAttrib)
      {
         const char *name= pAttrib->Name();
         const char *value= pAttrib->Value();
               
         printf( "parseXmlPREQSystemContext: ignoring unknown adm:SystemContext attribute %s: value=[%s]\n",  name, value);
         
         pAttrib=pAttrib->Next();
      }
   }

   if ( result )
   {
      TiXmlNode* pChild;
      for ( pChild = pElement->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) 
      {
         const char *childName= pChild->Value();
         
         if ( strcmp( childName, "adm:Session" ) == 0 )
         {
            uuid_t sessionId;
            
            const char *text= pChild->ToElement()->GetText();
            
            if ( uuid_parse( text, sessionId ) != 0 )
            {
               printf("parseXmlPREQSystemContext: bad sessionId value: %s\n", text);
               result= false;
               break;
            }
            memcpy( session->sessionId, sessionId, sizeof(uuid_t) );
         }
         else if ( strcmp( childName, "adm:Zone" ) == 0 )
         {
         }
         else
         {
            printf( "parseXmlPREQSystemContext: ignoring unknown adm:SystemContext child Element [%s]\n", childName );
         }
      }      
   }
   
   return result;
}

bool parseXmlPREQService( TiXmlElement *pElement, SessionData *session, RBI_InsertionOpportunity_Data *opportunity )
{
   bool result= false;
   
   if ( opportunity && pElement )
   {
      result= true;

      TiXmlAttribute* pAttrib=pElement->FirstAttribute();
      while (pAttrib)
      {
         const char *name= pAttrib->Name();
         const char *value= pAttrib->Value();
               
         if ( strcmp( name, "id" ) == 0 )
         {
            int serviceId;
            
            if (( sscanf( value, "urn:cibo:sourceid:%d", &serviceId ) != 1 ) && (sscanf( value, "urn:merlin:linear:stream:%d",&serviceId ) != 1 ))
            {
               printf("parseXmlPREQService: bad id value: %s\n", value);
               result= false;
               break;
            }
         }
         else
         {
            printf( "parseXmlPREQService: ignoring unknown Service attribute %s: value=[%s]\n",  name, value);
         }
         
         pAttrib=pAttrib->Next();
      }
   }

   if ( result )
   {
      TiXmlNode* pChild;
      for ( pChild = pElement->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) 
      {
         const char *childName= pChild->Value();
         
         if ( strcmp( childName, "adm:ProductType" ) == 0 )
         {
            const char *text= pChild->ToElement()->GetText();

            if ( strcmp( text, "LINEAR_T6" ) != 0 )
            {
               printf("parseXmlPREQService: bad adm:ProductType value: %s\n", text);
               result= false;
               break;
            }
         }
         else if ( strcmp( childName, "core:Ext" ) == 0 )
         {
         }
         else
         {
            printf( "parseXmlPREQService: ignoring unknown Service child Element [%s]\n", childName );
         }
      }      
   }
   
   return result;
}

bool parseXmlPREQClient( TiXmlElement *pElement, SessionData *session, RBI_InsertionOpportunity_Data *opportunity )
{
   bool result= false;
   
   if ( opportunity && pElement )
   {
      result= true;

      TiXmlAttribute* pAttrib=pElement->FirstAttribute();
      while (pAttrib)
      {
         const char *name= pAttrib->Name();
         const char *value= pAttrib->Value();
               
         printf( "parseXmlPREQClient: ignoring unknown adm:Client attribute %s: value=[%s]\n",  name, value);
         
         pAttrib=pAttrib->Next();
      }
   }

   if ( result )
   {
      TiXmlNode* pChild;
      for ( pChild = pElement->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) 
      {
         const char *childName= pChild->Value();
         
         if ( strcmp( childName, "core:CurrentDateTime" ) == 0 )
         {
            long long utcTimeCurrent;
            
            const char *text= pChild->ToElement()->GetText();
            
            if ( !parseTimeString(text, false, &utcTimeCurrent) )
            {
               printf("parseXmlPREQClient: bad core:CurrentDateTime value: %s\n", text);
               result= false;
               break;
            }
            opportunity->utcTimeCurrent= utcTimeCurrent;
         }
         else if ( strcmp( childName, "adm:TerminalAddress" ) == 0 )
         {
            const char *text= pChild->ToElement()->Attribute("type");
            if ( !text )
            {
               printf("parseXmlPREQClient: missing adm:TerminalAddress type\n");
               result= false;
            }
            else if ( strcmp( text, "DEVICEID" ) != 0 )
            {
               printf("parseXmlPREQClient: bad adm:TerminalAddress type value: %s\n", text);
               result= false;            
            }
            if ( result )
            {
               long long value;
               text= pChild->ToElement()->GetText();
               if ( sscanf( text, "%llx", &value ) == 1 )
               {
                  opportunity->deviceId[0]= ((value>>40)&0xFF);
                  opportunity->deviceId[1]= ((value>>32)&0xFF);
                  opportunity->deviceId[2]= ((value>>24)&0xFF);
                  opportunity->deviceId[3]= ((value>>16)&0xFF);
                  opportunity->deviceId[4]= ((value>>8)&0xFF);
                  opportunity->deviceId[5]= (value&0xFF);
                  
                  memcpy( session->deviceId, opportunity->deviceId, sizeof(opportunity->deviceId) );
               }
               else
               {
                  printf("parseXmlPREQClient: bad adm:TerminalAddress type value: %s\n", text);
                  result= false;            
               }
            }
         }
         else if ( strcmp( childName, "adm:TargetCode" ) == 0 )
         {
            const char *text= pChild->ToElement()->Attribute("key");
            if ( !text )
            {
               printf("parseXmlPREQClient: missing adm:TargetCode key\n");
               result= false;
            }
            else if (strcmp( text, ALTCON_RECEIVER.value ) != 0)
            {
               printf("parseXmlPREQClient: bad adm:TargetCode key value: %s\n", text);
               result= false;
            }
         }
         else
         {
            printf( "parseXmlPREQClient: ignoring unknown adm:SystemContext child Element [%s]\n", childName );
         }
      }      
   }
   
   return result;
}

bool parseXmlPREQPlacementOpportunity( TiXmlElement *pElement, SessionData *session, RBI_InsertionOpportunity_Data *opportunity )
{
   bool result= false;
   
   if ( opportunity && pElement )
   {
      result= true;

      TiXmlAttribute* pAttrib=pElement->FirstAttribute();
      while (pAttrib)
      {
         const char *name= pAttrib->Name();
         const char *value= pAttrib->Value();
               
         if ( strcmp( name, "serviceRegistrationRef") == 0 )
         {
            if ( strcmp( value, "-" ) != 0 )
            {
               printf("parseXmlPREQPlacementOpportunity: bad serviceRegistrationRef value: %s\n", value );
               result= false;
            }
         }
         else if ( strcmp( name, "id" ) == 0 )
         {
            uuid_t id;
            if (session->distributorSignal && ( uuid_parse( value, id ) == 0 ))
            {
               memcpy( session->popId, id, sizeof(uuid_t) );
            }
            else if (!session->distributorSignal && (strstr( value, "PO:" ) != NULL))
            {
               strcpy(session->poElementId, value);
            }
            else
            {
               printf("parseXmlPREQPlacementOpportunity: bad id value: %s %d\n", value, session->distributorSignal);
               result= false;
               break;
            }
         }
         else if ( strcmp( name, "breakId" ) == 0 )
         {
         }
         else
         {
            printf( "parseXmlPREQPlacementOpportunity: ignoring unknown adm:PlacementOpportunity attribute %s: value=[%s]\n",  name, value);
         }
         
         pAttrib=pAttrib->Next();
      }
   }

   if ( result )
   {
      TiXmlNode* pChild;
      for ( pChild = pElement->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) 
      {
         const char *childName= pChild->Value();

         if ( strcmp( childName, "adm:OpportunityConstraints" ) == 0 )
         {
            result= parseXmlPREQOpportunityConstraints( pChild->ToElement(), session );
         }
         else if (session->distributorSignal && ( strcmp( childName, "core:Ext" ) == 0 ))
         {
            result= parseXmlPREQCoreExt( pChild->ToElement(), session, opportunity );
         }
         else if (!session->distributorSignal && ( strcmp( childName, "adm:Entertainment" ) == 0 ))
         {
            result= parseXmlPREQCoreEntertainment( pChild->ToElement(), session, opportunity );
         }
         else
         {
            printf( "parseXmlPREQPlacementOpportunity: ignoring unknown adm:PlacementOpportunity child Element [%s]\n", childName );
         }
      }
   }

   return result;
}

bool parseXmlPREQPlacementRequest( TiXmlElement *pElement, SessionData *session, RBI_InsertionOpportunity_Data *opportunity )
{
   bool result= false;
   
   if ( opportunity && pElement )
   {
      result= true;

      TiXmlAttribute* pAttrib=pElement->FirstAttribute();
      while (pAttrib)
      {
         const char *name= pAttrib->Name();
         const char *value= pAttrib->Value();
               
         if ( strcmp( name, "xmlns:xsi" ) == 0 )
         {
            if ( strcmp( value, XMLSCHEMA_INSTANCE.value ) != 0 )
            {
               printf("parseXmlPREQPlacementRequest: bad xmlns:xsi value: %s\n", value);
               result= false;
               break;
            }
         }
         else if ( strcmp( name, "xsi:schemaLocation" ) == 0 )
         {
            // ignore attribute
         }
         else if ( strcmp( name, "xmlns:adm" ) == 0 )
         {
            if ( strcmp( value, SCHEMAS_ADMIN.value ) != 0 )
            {
               printf("parseXmlPREQPlacementRequest: bad xmlns:adm value: %s\n", value);
               result= false;
               break;
            }
         }
         else if ( strcmp( name, "xmlns:core" ) == 0 )
         {
            if ( strcmp( value, SCHEMAS_CORE.value ) != 0 )
            {
               printf("parseXmlPREQPlacementRequest: bad xmlns:core value: %s\n", value);
               result= false;
               break;
            }
         }
         else if ( strcmp( name, XMLNS_SCHEMA.value ) == 0 )
         {
            if ( strcmp( value, SCHEMAS_NGOD.value ) != 0 )
            {
               printf("parseXmlPREQPlacementRequest: unexpected %s value: %s\n", XMLNS_SCHEMA.value, value);
               break;
            }
         }
         else if ( strcmp( name, "identity" ) == 0 )
         {
            if ( strcmp( value, "86CF2E98-AEBA-4C3A-A3D4-616CF7D74A79" ) != 0 )
            {
               printf("parseXmlPREQPlacementRequest: bad identity value: %s\n", value);
               result= false;
               break;
            }
         }
         else if ( strcmp( name, "version" ) == 0 )
         {
            if ( strcmp( value, "1.1" ) != 0 )
            {
               printf("parseXmlPREQPlacementRequest: bad version value: %s\n", value);
               result= false;
               break;
            }
         }
         else if ( strcmp( name, "system" ) == 0 )
         {
            if ( strcmp( value, "QAMRDK" ) != 0 )
            {
               printf("parseXmlPREQPlacementRequest: bad system value: %s\n", value);
               result= false;
               break;
            }
         }
         else if ( strcmp( name, "messageId" ) == 0 )
         {
            uuid_t messageId;
            
            if ( uuid_parse( value, messageId ) != 0 )
            {
               printf("parseXmlPREQPlacementRequest: bad messageId value: %s\n", value);
               result= false;
               break;
            }
            memcpy( session->preqMessageId, messageId, sizeof(uuid_t) );
         }
         else
         {
            printf( "parseXmlPREQPlacementRequest: ignoring unknown adm:PlacementRequest attribute %s: value=[%s]\n",  name, value);
         }
         
         if ( !result ) break;
         
         pAttrib=pAttrib->Next();
      }
   }

   if ( result )
   {
      TiXmlNode* pChild;
      for ( pChild = pElement->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) 
      {
         const char *childName= pChild->Value();
         
         if ( strcmp( childName, "adm:SystemContext" ) == 0 )
         {
            result= parseXmlPREQSystemContext( pChild->ToElement(), session, opportunity );
         }
         else if ( strcmp( childName, "adm:Service" ) == 0 )
         {
            result= parseXmlPREQService( pChild->ToElement(), session, opportunity );
         }
         else if ( strcmp( childName, "adm:Client" ) == 0 )
         {
            result= parseXmlPREQClient( pChild->ToElement(), session, opportunity );
         }
         else if ( strcmp( childName, "adm:PlacementOpportunity" ) == 0 )
         {
            result= parseXmlPREQPlacementOpportunity( pChild->ToElement(), session, opportunity );
         }
         else
         {
            printf( "parseXmlPREQPlacementRequest: ignoring unknown adm:PlacementRequest Element [%s]\n", childName );
         }
      }      
   }
   
   return result;
}

static int parseDuration(const char *s)
{
   const char *p, *end = NULL;
   long hour = 0, minute = 0;
   float second = 0;
   size_t len = 0;

   if (s == NULL)
      return 0;

   len = strlen(s);
   if (len < 4 || s[0] != 'P' || s[1] != 'T')
      return 0;

   p = s + 2;
   end = p + 1 ;
   for (; *end >= '0' && *end <= '9' && end < s + len; end ++);
   if (*end == 'H')
   {
      hour = strtol(p, NULL, 10);
      p = end + 1;
   }

   if (p >= s + len)
   {
      return hour * 3600;
   }

   end  = p + 1;
   for (; *end >= '0' && *end <= '9' && end < s + len; end ++);
   if (*end == 'M')
   {
      minute = strtol(p, NULL, 10);
      p = end + 1;
   }

   if (p >= s + len)
   {
      return hour * 3600 + minute * 60;
   }

   end  = p + 1;
   for (; (*end >= '0' && *end <= '9' || *end == '.') &&  end < s + len; end ++);
   if (*end == 'S')
   {
      second = strtof(p, NULL);
      p = end + 1;
   }

   if (p >= s + len)
   {
      return hour * 3600 + minute * 60 + (int)second;
   }

   return 0;
}

bool parseXmlPRSPContent( TiXmlElement *pElement, SessionData *session, RBI_DefineSpots_Args *spotDfn, 
                          int *duration, char *assetID, char *providerID, char *uri, char *trackingIdString, bool* assetFound, RBI_SpotAction action )
{
   bool result= false;
   bool contentLocationFound= false;
   RBI_InsertionStatus_Data status;
   memset( &status, 0, sizeof(RBI_InsertionStatus_Data) );
   int detailCode=0;
   
   if ( spotDfn && pElement )
   {
      result= true;

      TiXmlAttribute* pAttrib=pElement->FirstAttribute();
      while (pAttrib)
      {
         const char *name= pAttrib->Name();
         const char *value= pAttrib->Value();

         printf( "parseXmlPRSPContent: ignoring unknown attribute %s: value=[%s]\n",  name, value);
         
         pAttrib=pAttrib->Next();
      }
   }
   
   if ( result )
   {
      TiXmlNode* pChild;
      for ( pChild = pElement->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) 
      {
         int len;
         const char *childName= pChild->Value();
         
         if ( strcmp( childName, "core:ContentLocation" ) == 0 && *assetFound == false)
         {
            contentLocationFound = true;
            const char *text= pChild->ToElement()->Attribute("mediaType");
            if ( !text )
            {
               printf( "parseXmlPRSPContent: missing mediaType\n");
               result= false;
               detailCode = RBI_DetailCode_missingMediaType;
               printf("RBI LSA-SE : Missing Media Type for %s spot %d\n", (action == RBI_SpotAction_replace) ? "Replace" : "Fixed", spotDfn->numSpots);
            }
            else
            {
               if (  strncmp( text, "video/mp2ts", 11 ) != 0 )
               {
                  printf( "parseXmlPRSPContent: bad mediaType:value=[%s]\n",  text);
                  result= false;
                  detailCode = RBI_DetailCode_matchingAssetNotFound;
                  printf("RBI LSA-SE : Matching Asset Not Found for %s spot %d\n", (action == RBI_SpotAction_replace) ? "Replace" : "Fixed", spotDfn->numSpots);
               }
               else
               {
                  if ( strstr(text, "avc1.640028") != NULL )
                  {
                     printf( "parseXmlPRSPContent: Asset Found:%s\n", text );
                     if( strstr(text, "ac-3") == NULL )
                     {
                        printf("parseXmlPRSPContent: audioType mismatch: %s\n",text);
                        detailCode = RBI_DetailCode_PRSPAudioAssetNotFound;
                        printf("RBI LSA-SE : Audio Asset Not Found for %s spot %d\n", (action == RBI_SpotAction_replace) ? "Replace" : "Fixed", spotDfn->numSpots);
                     }
                     text= pChild->ToElement()->GetText();
                     len= strlen(text);
                     if ( (len == 0) || (len > RBI_MAX_CONTENT_URI_LEN) )
                     {
                        printf( "parseXmlPRSPContent: bad content uri len; %d\n", len );
                        result= false;
                     }
                     strncpy( uri, text, RBI_MAX_CONTENT_URI_LEN );
                     uri[RBI_MAX_CONTENT_URI_LEN]= '\0';
                     printf( "parseXmlPRSPContent: Asset URL:%s\n", text );
                     *assetFound = true;
                  }
               }
            }
         }
         else if ( strcmp( childName, "core:AssetRef" ) == 0 )
         {
            const char *text= pChild->ToElement()->Attribute("assetID");
            if ( !text )
            {
               printf( "parseXmlPRSPContent: core:AssetRef missing assetID\n");
               g_failureReason = "parseXmlPRSPContent: core:AssetRef missing assetID.";
               result= false;
            }
            strncpy( assetID, text, RBI_MAX_ASSETID_LEN );
            assetID[RBI_MAX_ASSETID_LEN]= '\0';
            strncpy( status.assetID, assetID, RBI_MAX_ASSETID_LEN+1 );
            text= pChild->ToElement()->Attribute("providerID");
            if ( !text )
            {
               printf( "parseXmlPRSPContent: core:AssetRef missing providerID\n");
               g_failureReason = "parseXmlPRSPContent: core:AssetRef missing providerID.";
               result= false;
            }
            strncpy( providerID, text, RBI_MAX_PROVIDERID_LEN );
            providerID[RBI_MAX_PROVIDERID_LEN]= '\0';
            strncpy( status.providerID, providerID, RBI_MAX_PROVIDERID_LEN+1 );
         }
         else if ( strcmp( childName, "core:Duration" ) == 0 )
         {
            const char *text= pChild->ToElement()->GetText();
            *duration = parseDuration(text) * 1000;
            if( *duration == 0 )
            {
               printf( "parseXmlPRSPContent: bad duration: value=[%s]\n",  text);
               g_failureReason = "parseXmlPRSPContent: bad duration.";
               result= false;
               g_PRespError = true;
               detailCode = RBI_DetailCode_messageMalformed;
               goto exit;
            }
            session->duration= *duration;
         }
         else if ( strcmp( childName, "core:Tracking" ) == 0 )
         {
            const char *text= pChild->ToElement()->GetText();
            if( !text )
            {
               printf("parseXmlPRSPContent: core:Tracking missing trackingID\n");
               g_failureReason = "parseXmlPRSPContent: core:Tracking missing trackingID.";
               result= false;
            }
            strncpy( trackingIdString, text, RBI_MAX_TRACKINGID_LEN );
            trackingIdString[RBI_MAX_TRACKINGID_LEN] = '\0';
            strncpy( status.trackingIdString, trackingIdString, RBI_MAX_TRACKINGID_LEN+1 );
         }
         else
         {
            printf( "parseXmlPRSPContent: ignoring unknown core:Content child Element [%s]\n", childName );
         }
      }
      if (( !contentLocationFound ) && ( *assetFound == false ))
      {
         printf("RBI LSA-SE : Content Location Missing for %s spot %d\n", (action == RBI_SpotAction_replace) ? "Replace" : "Fixed", spotDfn->numSpots);
         detailCode = RBI_DetailCode_missingContentLocation;
         result= false;
      }
      else if (( contentLocationFound ) && ( *assetFound == false ) && (detailCode == 0 ))
      {
         printf("RBI LSA-SE : Matching Asset Not Found for %s spot %d\n", (action == RBI_SpotAction_replace) ? "Replace" : "Fixed", spotDfn->numSpots);
         result= false;
         detailCode = RBI_DetailCode_matchingAssetNotFound;
      }

      exit:
      if((result != true) || (detailCode != 0 ))
      {
         memcpy( status.sessionId, session->sessionId, sizeof(RBISessionID) );
         memcpy( status.deviceId, session->deviceId, sizeof(unsigned char[6]) );
         g_PRespSpotError = true;
         status.triggerId = session->currentTriggerId;
         if (spotDfn->spots[spotDfn->numSpots].detailCode == RBI_DetailCode_NoError)
         {
            spotDfn->spots[spotDfn->numSpots].detailCode = (RBI_DetailCode)detailCode;
         }
      }
   }

   return result;
}

bool parseXmlPRSPPlacement( TiXmlElement *pElement, SessionData *session, RBI_DefineSpots_Args *spotDfn, int index )
{
   bool result= false;
   RBI_SpotAction action;
   int duration;
   char assetID[RBI_MAX_ASSETID_LEN+1];
   char providerID[RBI_MAX_PROVIDERID_LEN+1];
   char uri[RBI_MAX_CONTENT_URI_LEN+1];
   char trackingIdString[RBI_MAX_TRACKINGID_LEN+1];
   bool assetFound;
   RBI_InsertionStatus_Data status;
   memset( &status, 0, sizeof(RBI_InsertionStatus_Data) );
   int detailCode=0;

   action= RBI_SpotAction_unknown;
   duration= 0;
   assetID[0]= '\0';
   providerID[0]= '\0';
   uri[0]= '\0';
   trackingIdString[0]= '\0';
   
   if ( spotDfn && pElement )
   {
      result= true;
      spotDfn->spots[spotDfn->numSpots].validSpot = RBI_Spot_valid;
      spotDfn->spots[spotDfn->numSpots].detailCode = RBI_DetailCode_NoError;

      TiXmlAttribute* pAttrib=pElement->FirstAttribute();
      while (pAttrib)
      {
         const char *name= pAttrib->Name();
         const char *value= pAttrib->Value();
         
         if ( strcmp( name, "action" ) == 0 )
         {
            if ( strcmp( value, "fixed" ) == 0 )
            {
               spotDfn->spots[index].action = action = RBI_SpotAction_fixed;
            }
            else if ( strcmp( value, "replace" ) == 0 )
            {
               spotDfn->spots[index].action = action= RBI_SpotAction_replace;
            }
            else
            {
               printf( "parseXmlPRSPPlacement: unsupported value for action: %s\n", value );
               g_failureReason = "parseXmlPRSPPlacement: unsupported value for action.";
               result= false;
               break;
            }
         }
         else if ( strcmp( name, "id" ) == 0 )
         {
            uuid_t id;
            if ( uuid_parse( value, id ) != 0 )
            {
               printf("parseXmlPRSPPlacement: bad id value: %s\n", value);
               g_failureReason = "parseXmlPRSPPlacement: bad id value.";
               result= false;
               break;
            }
         }
         else
         {
            printf( "parseXmlPRSPPlacement: ignoring unknown adm:Placement attribute %s: value=[%s]\n",  name, value);
         }
         
         pAttrib=pAttrib->Next();
      }
   }
   printf("RBI LSA-SP : Insertion Attempt for %s spot %d\n", (spotDfn->spots[index].action == RBI_SpotAction_replace) ? "Replace" : "Fixed", spotDfn->numSpots);

   if ( result )
   {
      TiXmlNode* pChild;
      pChild = pElement->FirstChild();
      if (pChild != NULL)
      {
         for ( pChild = pElement->FirstChild(); pChild != 0; pChild = pChild->NextSibling())
         {
            const char *childName= pChild->Value();

            if ( strcmp( childName, "core:Content" ) == 0 )
            {
               assetFound = false;
               result= parseXmlPRSPContent( pChild->ToElement(), session, spotDfn, &duration, assetID, providerID, uri, trackingIdString, &assetFound, action );
               if((result == false) && (spotDfn->spots[spotDfn->numSpots].detailCode == RBI_DetailCode_messageMalformed))
               {
                  break;
               }
            }
            else
            {
               if( action == RBI_SpotAction_replace )
               {
                  if( strcmp( childName, "core:Tracking" ) == 0 ) // As per Linear+QAM+Player+Requirements ignore the PlacementDecision/adm:Placement/Tracking
                  {
                     printf( "parseXmlPRSPPlacement: ignoring adm:Placement attribute [%s]\n",  childName );
                  }
                  else
                  {
                     printf( "parseXmlPRSPPlacement: Missing attribute core:Content\n" );
                     detailCode = RBI_DetailCode_missingContent;
                     printf("RBI LSA-SE : Missing Content for Replace spot %d\n",  spotDfn->numSpots);
                     result= false;
                  }
               }
            }
         }
      }
      else
      {
         if( action == RBI_SpotAction_replace )
         {
            printf( "parseXmlPRSPPlacement: Missing PlacementElement core:Content\n" );
            detailCode = RBI_DetailCode_missingContent;
            printf("RBI LSA-SE : Missing Content for Replace spot %d\n", spotDfn->numSpots);
            result= false;
         }
      }

      if(result == false)
      {
         spotDfn->spots[spotDfn->numSpots].validSpot = RBI_Spot_invalid;
         if (spotDfn->spots[spotDfn->numSpots].detailCode == RBI_DetailCode_NoError)
         {
            spotDfn->spots[spotDfn->numSpots].detailCode = (RBI_DetailCode)detailCode;
         }
      }
      if ( result )
      {
         // Check if we have all we need to add a new spot to definition
         if ( ((action == RBI_SpotAction_fixed) || (action == RBI_SpotAction_replace)) &&
              (duration > 0 ) &&
              ((strlen(assetID) > 0 )||(action == RBI_SpotAction_fixed)) &&
              ((strlen(providerID) > 0 )||(action == RBI_SpotAction_fixed)) &&
              ((strlen(uri) > 0 )||(action == RBI_SpotAction_fixed)) )
         {
            int i= spotDfn->numSpots;
            int len;
            spotDfn->spots[i].action= action;
            spotDfn->spots[i].duration= duration;
            memset( spotDfn->spots[i].trackingIdString, 0, RBI_MAX_TRACKINGID_LEN );
            memcpy( spotDfn->spots[i].trackingIdString, trackingIdString, sizeof(trackingIdString) );
            spotDfn->spots[i].trackingIdString[RBI_MAX_TRACKINGID_LEN] = '\0';
            if (( action == RBI_SpotAction_replace ) || (action == RBI_SpotAction_fixed))
            {
               if( assetFound )
               {
                  memcpy( spotDfn->spots[i].assetID, assetID, sizeof(assetID) );
                  memcpy( spotDfn->spots[i].providerID, providerID, sizeof(providerID) );
               
                  len= strlen(uri);
                  if ( (len > 0) && (len < RBI_MAX_URI_DATA))
                  {
                     strncpy( spotDfn->spots[i].uriData, uri, len );
                     spotDfn->spots[i].uriData[len]= '\0';
                  }
                  else
                  {
                     printf( "parseXmlPRSPPlacement: exceeded maximum total uri size with spot %d url len %d\n", i, len);
                     result= false;
                  }
               }
               else
               {
                  printf( "parseXmlPRSPPlacement: mediaType MPEG4 720p asset NOT FOUND\n");
               }
            }
            if ( result && assetFound )
            {
               spotDfn->numSpots += 1;
            }

         }
      }
      else
      {
         if ( ((action == RBI_SpotAction_fixed) || (action == RBI_SpotAction_replace)) && \
            (spotDfn->spots[spotDfn->numSpots].validSpot == RBI_Spot_invalid) && \
            (strlen(assetID) > 0 ) && \
            (strlen(providerID) > 0 ) && \
            (duration > 0 ) )
         {
            int i= spotDfn->numSpots;
            spotDfn->spots[i].action= action;
            spotDfn->spots[i].duration= duration;
            memcpy( spotDfn->spots[i].assetID, assetID, sizeof(assetID) );
            memcpy( spotDfn->spots[i].providerID, providerID, sizeof(providerID) );
            memset( spotDfn->spots[i].uriData, 0, RBI_MAX_URI_DATA);
            memcpy( spotDfn->spots[i].trackingIdString, trackingIdString, sizeof(trackingIdString) );
            spotDfn->numSpots += 1;
         }
      }
      if((result != true) && (detailCode != 0 ))
      {
         memcpy( status.sessionId, session->sessionId, sizeof(RBISessionID) );
         memcpy( status.deviceId, session->deviceId, sizeof(unsigned char[6]) );
         g_PRespSpotError = true;
         status.triggerId = session->currentTriggerId;
         if (spotDfn->spots[spotDfn->numSpots].detailCode == RBI_DetailCode_NoError)
         {
            spotDfn->spots[spotDfn->numSpots].detailCode = (RBI_DetailCode)detailCode;
         }
      }
   }
   return result;
}

bool parseXmlPRSPPlayPositionStart( TiXmlElement *pElement, SessionData *session, RBI_DefineSpots_Args *spotDfn )
{
   bool result= false;
   
   if ( spotDfn && pElement )
   {
      result= true;

      TiXmlAttribute* pAttrib=pElement->FirstAttribute();
      while (pAttrib)
      {
         const char *name= pAttrib->Name();
         const char *value= pAttrib->Value();
               
         printf( "parseXmlPRSPPlayPositionStart: ignoring unknown cmcst:PlayPositionStart attribute %s: value=[%s]\n",  name, value);
         
         pAttrib=pAttrib->Next();
      }
   }

   if ( result )
   {
      TiXmlNode* pChild;
      for ( pChild = pElement->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) 
      {
         const char *childName= pChild->Value();
         
         if ( strcmp( childName, "cmcst:SignalId" ) == 0 )
         {
            // Ignore
         }
         else if ( strcmp( childName, "cmcst:UTCTime" ) == 0 )
         {
            long long utcTimeStart;
            const char *text= pChild->ToElement()->GetText();

            if ( !parseTimeString(text, true, &utcTimeStart) )
            {
               printf("parseXmlPRSPPlayPositionStart: parseXmlPREQUTCTime: bad cmcst:UTCTime value: %s\n", text);
               g_failureReason = "parseXmlPRSPPlayPositionStart: parseXmlPREQUTCTime: bad cmcst:UTCTime value.";
               result= false;
            }
            formatTime(utcTimeStart, false, (char*)text);
            spotDfn->utcTimeStart= utcTimeStart;
         }
         else
         {
            printf( "parseXmlPRSPPlayPositionStart: ignoring unknown cmcst:PlayPositionStart child Element [%s]\n", childName );
         }
      }
   }

   return result;
}

bool parseXmlPRSPPlayPositionsAvailBinding( TiXmlElement *pElement, SessionData *session, RBI_DefineSpots_Args *spotDfn )
{
   bool result= false;
   
   if ( spotDfn && pElement )
   {
      result= true;

      TiXmlAttribute* pAttrib=pElement->FirstAttribute();
      while (pAttrib)
      {
         const char *name= pAttrib->Name();
         const char *value= pAttrib->Value();
               
         printf( "parseXmlPRSPPlayPositionsAvailBinding: ignoring unknown cmcst:PlayPositionsAvailBinding attribute %s: value=[%s]\n",  name, value);
         
         pAttrib=pAttrib->Next();
      }
   }

   if ( result )
   {
      TiXmlNode* pChild;
      for ( pChild = pElement->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) 
      {
         const char *childName= pChild->Value();
         
         if ( strcmp( childName, "cmcst:PlayPositionStart" ) == 0 )
         {
            result= parseXmlPRSPPlayPositionStart( pChild->ToElement(), session, spotDfn );
         }
         else
         {
            printf( "parseXmlPRSPPlayPositionsAvailBinding: ignoring unknown cmcst:PlayPositionsAvailBinding child Element [%s]\n", childName );
         }
      }
   }

   return result;
}

bool parseXmlPRSPCoreExt( TiXmlElement *pElement, SessionData *session, RBI_DefineSpots_Args *spotDfn )
{
   bool result= false;
   
   if ( spotDfn && pElement )
   {
      result= true;

      TiXmlAttribute* pAttrib=pElement->FirstAttribute();
      while (pAttrib)
      {
         const char *name= pAttrib->Name();
         const char *value= pAttrib->Value();
               
         printf( "parseXmlPRSPCoreExt: ignoring unknown core:Ext attribute %s: value=[%s]\n",  name, value);
         
         pAttrib=pAttrib->Next();
      }
   }

   if ( result )
   {
      TiXmlNode* pChild;
      for ( pChild = pElement->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) 
      {
         const char *childName= pChild->Value();
         
         if ( strcmp( childName, "cmcst:PlayPositionsAvailBinding" ) == 0 )
         {
            result= parseXmlPRSPPlayPositionsAvailBinding( pChild->ToElement(), session, spotDfn );
         }
         else
         {
            printf( "parseXmlPRSPCoreExt: ignoring unknown core:Ext child Element [%s]\n", childName );
         }
      }
   }

   return result;
}

bool parseXmlPREQOpportunityConstraints( TiXmlElement *pElement, SessionData *session )
{
   bool result= false;
   
   if ( pElement )
   {
      result= true;

      TiXmlAttribute* pAttrib=pElement->FirstAttribute();
      while (pAttrib)
      {
         const char *name= pAttrib->Name();
         const char *value= pAttrib->Value();
               
         printf( "parseXmlPREQOpportunityConstraints: ignoring unknown parseXmlPREQOpportunityConstraints attribute %s: value=[%s]\n",  name, value);
         
         pAttrib=pAttrib->Next();
      }
   }

   if ( result )
   {
      TiXmlNode* pChild;
      for ( pChild = pElement->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) 
      {
         const char *childName= pChild->Value();
         
         if ( strcmp( childName, "core:Duration" ) == 0 )
         {
            const char *text= pChild->ToElement()->GetText();
            if ( !parseDurationString(text) )
            {
               printf("parseXmlPREQOpportunityConstraints: bad core:Duration value: %s\n", text);
               result= false;
            }
         }
         else if ( strcmp( childName, "adm:Scope" ) == 0 )
         {
            const char *text= pChild->ToElement()->GetText();
            if( strcmp(text, DISTRIBUTOR) == 0 )
            {
               session->distributorSignal = true;
            }
            if ( ( strcmp(text, DISTRIBUTOR) != 0 ) && ( strcmp(text, PROVIDER) != 0 ))
            {
               printf("parseXmlPREQOpportunityConstraints: bad core:Duration value: %s\n", text);
               result= false;
            }
         }
         else
         {
            printf( "parseXmlPREQOpportunityConstraints: ignoring unknown adm:OpportunityConstraints child Element [%s]\n", childName );
         }
      }
   }

   return result;
}

bool parseDurationString( const char *s)
{
   bool result= false;
   int hour, minute, second, millis;
   
   if ( (sscanf( s, "PT%d.%dS", &second, &millis) == 2) || (sscanf( s, "PT%dM%d.%dS", &minute, &second, &millis) == 3) || (sscanf( s, "PT%dH%dM%d.%dS", &hour, &minute, &second, &millis) == 4) )
   {
      result= true;
   }
   return result;
}

bool parseXmlPRSPPlacementDecision( TiXmlElement *pElement, SessionData *session, RBI_DefineSpots_Args *spotDfn )
{
   bool result= false;
   
   if ( spotDfn && pElement )
   {
      result= true;

      TiXmlAttribute* pAttrib=pElement->FirstAttribute();
      while (pAttrib)
      {
         const char *name= pAttrib->Name();
         const char *value= pAttrib->Value();
               
         if ( strcmp( name, "placementOpportunityRef") == 0 )
         {
            uuid_t opportunityRef;
            if ( uuid_parse( value, opportunityRef ) == 0 )
            {
               if ( memcmp( session->popId, opportunityRef, sizeof(opportunityRef) ) != 0 )
               {
                  printf("parseXmlPRSPPlacementDecision: mismatched placementOpportunityRef value: %s\n", value);
                  g_failureReason = "parseXmlPRSPPlacementDecision: mismatched placementOpportunityRef value.";
                  result= false;
                  break;
               }
            }
            else if ( strstr( value, "PO:" ) != NULL )
            {
               if ( strcmp( session->poElementId, value) != 0 )
               {
                  printf("parseXmlPRSPPlacementDecision: mismatched placementOpportunityRef value: %s\n", value);
                  g_failureReason = "parseXmlPRSPPlacementDecision: mismatched placementOpportunityRef value.";
                  result= false;
                  break;
               }
            }
            else
            {
               printf("parseXmlPRSPPlacementDecision: bad placementOpportunityRef value: %s\n", value);
               g_failureReason = "parseXmlPRSPClient: bad adm:TerminalAddress type value.";
               result= false;
               break;
            }
         }
         else if ( strcmp( name, "id" ) == 0 )
         {
            uuid_t id;
            if ( uuid_parse( value, id ) != 0 )
            {
               printf("parseXmlPRSPPlacementDecision: bad id value: %s\n", value);
               g_failureReason = "parseXmlPRSPPlacementDecision: bad id value.";
               result= false;
               break;
            }
            memcpy(session->pdId, id, sizeof(uuid_t) );
         }
         else
         {
            printf( "parseXmlPRSPPlacementDecision: ignoring unknown adm:PlacementDecision attribute %s: value=[%s]\n",  name, value);
         }
         
         pAttrib=pAttrib->Next();
      }
   }

   if ( result )
   {
      TiXmlNode* pChild;
      int index = 0;
      for ( pChild = pElement->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) 
      {
         const char *childName= pChild->Value();
         
         if ( strcmp( childName, "adm:Placement" ) == 0 )
         {
            result= parseXmlPRSPPlacement( pChild->ToElement(), session, spotDfn, index );
            if((result == false) && (spotDfn->spots[spotDfn->numSpots].detailCode == RBI_DetailCode_messageMalformed))
            {
               break;
            }
         }
         else if ( strcmp( childName, "core:Ext" ) == 0 )
         {
            result= parseXmlPRSPCoreExt( pChild->ToElement(), session, spotDfn );
         }
         else
         {
            printf( "parseXmlPRSPPlacementDecision: ignoring unknown adm:PlacementDecision child Element [%s]\n", childName );
         }
         index++;
      }
   }

   return result;
}

bool parseXmlPRSPClient( TiXmlElement *pElement, SessionData *session, RBI_DefineSpots_Args *spotDfn )
{
   bool result= false;
   
   if ( spotDfn && pElement )
   {
      result= true;

      TiXmlAttribute* pAttrib=pElement->FirstAttribute();
      while (pAttrib)
      {
         const char *name= pAttrib->Name();
         const char *value= pAttrib->Value();
               
         printf( "parseXmlPRSPClient: ignoring unknown adm:Client attribute %s: value=[%s]\n",  name, value);
         
         pAttrib=pAttrib->Next();
      }
   }

   if ( result )
   {
      TiXmlNode* pChild;
      for ( pChild = pElement->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) 
      {
         const char *childName= pChild->Value();
         
         if ( strcmp( childName, "adm:TerminalAddress" ) == 0 )
         {
            const char *text= pChild->ToElement()->Attribute("type");
            if ( !text )
            {
               printf("parseXmlPRSPClient: missing adm:TerminalAddress type\n");
               g_failureReason = "parseXmlPRSPClient: missing adm:TerminalAddress type.";
               result= false;
            }
            else if ( strcmp( text, "DEVICEID" ) != 0 )
            {
               printf("parseXmlPRSPClient: bad adm:TerminalAddress type value: %s\n", text);
               g_failureReason = "parseXmlPRSPClient: bad adm:TerminalAddress type value.";
               result= false;            
            }
            if ( result )
            {
               long long value;
               text= pChild->ToElement()->GetText();
               if ( sscanf( text, "%llx", &value ) == 1 ) 
               {
                  unsigned char deviceId[6];
                  
                  deviceId[0]= ((value>>40)&0xFF);
                  deviceId[1]= ((value>>32)&0xFF);
                  deviceId[2]= ((value>>24)&0xFF);
                  deviceId[3]= ((value>>16)&0xFF);
                  deviceId[4]= ((value>>8)&0xFF);
                  deviceId[5]= (value&0xFF);
                  
                  if ( memcmp( session->deviceId, deviceId, sizeof(deviceId) ) != 0 )
                  {
                     printf("parseXmlPRSPClient: mismatched adm:TerminalAddress type value: %s\n", text);
                     g_failureReason = "parseXmlPRSPClient: mismatched adm:TerminalAddress type value.";
                     result= false;
                     break;
                  }
               }
               else
               {
                  printf("parseXmlPRSPClient: bad adm:TerminalAddress type value: %s\n", text);
                  g_failureReason = "parseXmlPRSPClient: bad adm:TerminalAddress type value.";
                  result= false;
                  break;
               }
            }
         }
         else
         {
            printf( "parseXmlPRSPClient: ignoring unknown adm:SystemContext child Element [%s]\n", childName );
         }
      }      
   }
   
   return result;
}

bool parseXmlPRSPSystemContext( TiXmlElement *pElement, SessionData *session, RBI_DefineSpots_Args *spotDfn )
{
   bool result= false;
   
   if ( spotDfn && pElement )
   {
      result= true;

      TiXmlAttribute* pAttrib=pElement->FirstAttribute();
      while (pAttrib)
      {
         const char *name= pAttrib->Name();
         const char *value= pAttrib->Value();
               
         printf( "parseXmlPRSPSystemContext: ignoring unknown adm:SystemContext attribute %s: value=[%s]\n",  name, value);
         
         pAttrib=pAttrib->Next();
      }
   }

   if ( result )
   {
      TiXmlNode* pChild;
      for ( pChild = pElement->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) 
      {
         const char *childName= pChild->Value();
         
         if ( strcmp( childName, "adm:Session" ) == 0 )
         {
            uuid_t sessionId;
            
            const char *text= pChild->ToElement()->GetText();
            
            if ( uuid_parse( text, sessionId ) != 0 )
            {
               printf("parseXmlPRSPSystemContext: bad sessionId value: %s\n", text);
               g_failureReason = "parseXmlPRSPSystemContext: bad sessionId value.";
               result= false;
               break;
            }
            if ( memcmp( session->sessionId, sessionId, sizeof(sessionId) ) != 0 )
            {
               printf("parseXmlPRSPSystemContext: mismatched sessionId value: %s\n", text);
               g_failureReason = "parseXmlPRSPSystemContext: mismatched sessionId value.";
               result= false;
               break;
            }
         }
         else
         {
            printf( "parseXmlPRSPSystemContext: ignoring unknown adm:SystemContext child Element [%s]\n", childName );
         }
      }      
   }
   
   return result;
}

bool parseXmlPRSPPlacementResponse( TiXmlElement *pElement, SessionData *session, RBI_DefineSpots_Args *spotDfn )
{
   bool result= false;
   int detailCode=0;
   char work[RBI_MAX_CONTENT_URI_LEN];
   std::string detailNote;
   RBI_InsertionStatus_Data status;
   memset( &status, 0, sizeof(RBI_InsertionStatus_Data) );
   
   if ( spotDfn && pElement )
   {
      result= true;

      TiXmlAttribute* pAttrib=pElement->FirstAttribute();
      while (pAttrib)
      {
         const char *name= pAttrib->Name();
         const char *value= pAttrib->Value();
               
         if ( strcmp( name, "xmlns:xsi" ) == 0 )
         {
            if ( strcmp( value, XMLSCHEMA_INSTANCE.value ) != 0 )
            {
               printf("parseXmlPRSPPlacementResponse: bad xmlns:xsi value: %s\n", value);
               g_failureReason = "parseXmlPRSPPlacementResponse: bad xmlns:xsi value.";
               result= false;
               break;
            }
         }
         else if ( strcmp( name, "xsi:schemaLocation" ) == 0 )
         {
            // ignore attribute
         }
         else if ( strcmp( name, "xmlns:adm" ) == 0 )
         {
            if ( strcmp( value, SCHEMAS_ADMIN.value ) != 0 )
            {
               printf("parseXmlPRSPPlacementResponse: bad xmlns:adm value: %s\n", value);
               g_failureReason = "parseXmlPRSPPlacementResponse: bad xmlns:adm value.";
               result= false;
               break;
            }
         }
         else if ( strcmp( name, "xmlns:core" ) == 0 )
         {
            if ( strcmp( value, SCHEMAS_CORE.value ) != 0 )
            {
               printf("parseXmlPRSPPlacementResponse: bad xmlns:core value: %s\n", value);
               g_failureReason = "parseXmlPRSPPlacementResponse: bad xmlns:core value";
               result= false;
               break;
            }
         }
         else if ( strcmp( name, XMLNS_SCHEMA.value ) == 0 )
         {
            if ( strcmp( value, SCHEMAS_NGOD.value ) != 0 )
            {
               printf("parseXmlPRSPPlacementResponse: unexpected %s value: %s\n", XMLNS_SCHEMA.value, value);
               break;
            }
         }
         else if ( strcmp( name, "version" ) == 0 )
         {
            if ( strcmp( value, "1.1" ) != 0 )
            {
               printf("RBI LSA-PRE: Invalid Version\n");
               result= false;
               detailCode = RBI_DetailCode_invalidVersion;
               detailNote = "Invalid version.";
               break;
            }
         }
         else if ( strcmp( name, "identity" ) == 0 )
         {
            if(strlen(value) > 0)
            {
               memset(session->prspIdentity, 0, sizeof(session->prspIdentity));
               strncpy(session->prspIdentity, value, sizeof(session->prspIdentity)-1);
            }
         }
         else if ( strcmp( name, "messageId" ) == 0 )
         {
            uuid_t messageId;
            uuid_parse( value, messageId );
            memcpy( session->prspMessageId, messageId, sizeof(uuid_t) );
         }
         else if ( strcmp( name, "messageRef" ) == 0 )
         {
            uuid_t messageRef;
            
            if ( uuid_parse( value, messageRef ) != 0 )
            {
               printf("RBI LSA-PRE: Bad Message Ref Format\n");
               result= false;
               detailCode = RBI_DetailCode_badMessageRefFormat;
               detailNote = "messageRef not in UUID format.";
               break;
            }

            if ( memcmp( session->preqMessageId, messageRef, sizeof(messageRef) ) != 0 )
            {
               printf("RBI LSA-PRE: Mismatched Message Ref\n");
               result= false;
               detailCode = RBI_DetailCode_messageRefMismatch;
               detailNote = "PlacementResponse - msg ref mismatch - expected ";
               uuid_unparse( session->preqMessageId, work );
               detailNote.append(work);
               detailNote.append(" got ");
               uuid_unparse( messageRef, work );
               detailNote.append(work);
               break;
            }
         }
         else
         {
            printf( "parseXmlPRSPPlacementResponse: ignoring unknown adm:PlacementResponse attribute %s: value=[%s]\n",  name, value);
         }
         
         if ( !result ) break;
         
         pAttrib=pAttrib->Next();
      }
   }

   if ( result )
   {
      TiXmlNode* pChild;
      for ( pChild = pElement->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) 
      {
         const char *childName= pChild->Value();
         
         if ( strcmp( childName, "core:StatusCode" ) == 0 )
         {
            const char *text= pChild->ToElement()->Attribute("class");
            if ( !text )
            {
               printf("parseXmlPRSPPlacementResponse: missing core:StatusCode class\n");
               g_failureReason = "parseXmlPRSPPlacementResponse: missing core:StatusCode class.";
               result= false;
               break;
            }
            if ( strcmp( text, "0" ) != 0 )
            {
               printf("parseXmlPRSPPlacementResponse: non zero StatusCode class\n");
               g_failureReason = "parseXmlPRSPPlacementResponse: non zero StatusCode class.";
               result= false;
               break;
            }
         }
         else if ( strcmp( childName, "adm:SystemContext" ) == 0 )
         {
            result= parseXmlPRSPSystemContext( pChild->ToElement(), session, spotDfn );
         }
         else if ( strcmp( childName, "adm:Client" ) == 0 )
         {
            result= parseXmlPRSPClient( pChild->ToElement(), session, spotDfn );
         }
         else if ( strcmp( childName, "adm:PlacementDecision" ) == 0 )
         {
            result= parseXmlPRSPPlacementDecision( pChild->ToElement(), session, spotDfn );
            if((result == false) && (spotDfn->spots[spotDfn->numSpots].detailCode == RBI_DetailCode_messageMalformed))
            {
               break;
            }
         }
         else
         {
            printf( "parseXmlPRSPPlacementResponse: ignoring unknown adm:PlacementResponse Element [%s]\n", childName );
         }
      }      
   }
   if ((result != true) && (detailCode != 0))
   {
      memcpy( status.sessionId, session->sessionId, sizeof(RBISessionID) );
      memcpy( status.deviceId, session->deviceId, sizeof(unsigned char[6]) );
      status.action = RBI_SpotAction_replace;
      status.event = RBI_SpotEvent_insertionFailed;
      status.detailCode = detailCode;
      if(detailNote.length() > 0)
      {
         status.detailNotePtr = (char*)calloc(detailNote.length() + 1, sizeof(char));
         if(status.detailNotePtr)
         {
            strncpy(status.detailNotePtr, detailNote.c_str(), detailNote.length());
         }
         else
         {
            printf("parseXmlPRSPPlacementResponse:: Error memory not allocated. Req memory %d\n", detailNote.length() + 1);
         }
      }

      g_PRespError = true;
      g_PSNError = true;
      status.triggerId = session->currentTriggerId;
      SessionData *sessionData= getSessionData( session->sessionId );
      if ( sessionData )
      {
         std::string *msg= createPlacementStatusNotification(sessionData, &status,false );
         if ( msg )
         {
            handleSCTE130PlacementStatus( sessionData, msg );
            delete msg;
            msg = 0;
         }
      }
      if(status.detailNotePtr)
      {
         free(status.detailNotePtr);
         status.detailNotePtr = NULL;
      }
   }
   return result;
}

SessionData *getSessionFromPlacementResponse( std::string *msg )
{
   SessionData *session= 0;
   TiXmlDocument doc;
   
   if ( msg )
   {
      doc.Parse( msg->c_str() );
      
      if ( !doc.Error() )
      {
         TiXmlNode *pParent, *pChild, *pChild1, *pChild2;
         
         pParent= &doc;
         int t = pParent->Type();

         if ( t == TiXmlNode::TINYXML_DOCUMENT )
         {
            for ( pChild = pParent->FirstChild(); pChild != 0; pChild = pChild->NextSibling())
            {
               if ( pChild->Type() == TiXmlNode::TINYXML_ELEMENT )
               {
                  if ( strcmp(pChild->Value(), "adm:PlacementResponse") == 0 )
                  {
                     TiXmlElement *pElement= pChild->ToElement();
                     for ( pChild1 = pElement->FirstChild(); pChild1 != 0; pChild1 = pChild1->NextSibling())
                     {
                        const char *childName= pChild1->Value();
                        if ( strcmp( childName, "adm:SystemContext" ) == 0 )
                        {
                           pElement= pChild1->ToElement();
                           for ( pChild2 = pElement->FirstChild(); pChild2 != 0; pChild2 = pChild2->NextSibling())
                           {
                              const char *childName= pChild2->Value();
                              
                              if ( strcmp( childName, "adm:Session" ) == 0 )
                              {
                                 uuid_t sessionId;
                                 
                                 const char *text= pChild2->ToElement()->GetText();

                                 if ( uuid_parse( text, sessionId ) == 0 )
                                 {
                                    session= getSessionData( sessionId );
                                    goto exit;
                                 }
                              }
                           }      
                        }
                     }
                  }
               }
            }
         }
      }
   }
   
exit:
   return session;
}

/*
 * Process incoming SCTE-130 Placement Request message.  This message encapsulates
 * information from the RBIFilter about an upcoming insertion opportunity.
 */
void handleSCTE130PlacementRequest( SessionData *session, std::string *msg )
{
   bool result= false;
   TiXmlDocument doc;
   RBI_InsertionOpportunity_Data opportunity;
   
   if ( msg )
   {
      printf( "-----------------------------------------------------------------------------\n" );
      printf( "%s\n", msg->c_str() );
      
      doc.Parse( msg->c_str() );
      
      if ( !doc.Error() )
      {
         TiXmlNode *pParent, *pChild;
         
         pParent= &doc;
         int t = pParent->Type();

         if ( t == TiXmlNode::TINYXML_DOCUMENT )
         {
            for ( pChild = pParent->FirstChild(); pChild != 0; pChild = pChild->NextSibling())
            {
               if ( pChild->Type() == TiXmlNode::TINYXML_ELEMENT )
               {
                  if ( strcmp(pChild->Value(), "adm:PlacementRequest") == 0 )
                  {
                     result= parseXmlPREQPlacementRequest( pChild->ToElement(), session, &opportunity );
                     break;
                  }
                  else
                  {
                     printf( "ignoring unknown Element [%s]\n", pChild->Value() );
                  }
               }
            }
            
            if ( result )
            {
                printf("RBI LSA-PRQ: Placement Request\n");

                std::string rsp;
                g_PRespError = false;
                g_PSNError = false;
                g_PRespSpotError = false;
                g_PlacementResponseReceived = false;
                int error;
                error = postMessage( g_endPointRequest, msg, &rsp, true, false );
                /*
                 * Post placement request to configured end-point and get response
                 */
                if ( 0 == error )
                {
                   handleSCTE130PlacementResponse( &rsp );
                }
                else if (CURLE_COULDNT_CONNECT == error)
                {
                   g_PSNError=true;
                   printf( "RBI LSA-PRQ E: PlacementRequest Send Failure DeviceID=%02X%02X%02X%02X%02X%02X, timeSent=%s, messageId=%s, SessionId=%s serviceId=%d PlacementOpportunityid=%s\n",g_deviceId[0], g_deviceId[1], g_deviceId[2], g_deviceId[3], g_deviceId[4], g_deviceId[5], g_PReqtime.c_str(), g_preqMessageId, g_sessionId, g_serviceId, g_popId);
                }
                else
                {
                   printf("RBI LSA-PRQ: Placement Request Post Failed error =%d\n",error);
                }
            }
         }
      }
      else
      {
         printf("Error: %d, %d:%d\n", doc.ErrorId(), doc.ErrorRow(), doc.ErrorCol() );
      }
   }
}

/*
 * Process outgoing SCTE-130 Placement Response message.  To provide the RBIFilter
 * the information it needs for a placement opportunity the SCTE-130 Placement
 * response must be passed to this method.
 */
void handleSCTE130PlacementResponse( std::string *msg )
{
   bool result= false;
   SessionData *session= 0;
   TiXmlDocument doc;
   RBI_DefineSpots_Args spotDfn;
   g_PlacementResponseReceived = true;
   long long utcTimeCurrent;
   struct timeval tv;
   struct timezone tz;
   char work[256];
   gettimeofday( &tv, &tz );
   utcTimeCurrent = UTCMILLIS(tv);
   formatTime(utcTimeCurrent, false, work);
   g_PResptime = std::string(work);

   RBI_InsertionStatus_Data status;
   uint8_t *base64DataPtr = NULL;
   memset( &status, 0, sizeof(RBI_InsertionStatus_Data) );
   std::string subStr;

   if ( msg )
   {
      printf( "-----------------------------------------------------------------------------\n" );
      printf( "%s\n", msg->c_str() );
      
      session= getSessionFromPlacementResponse(msg);
      if ( !session )
      {
         uuid_t sessionId;
         if ( uuid_parse( g_sessionId, sessionId ) != 0 )
         {
            printf("handleSCTE130PlacementResponse bad sessionId value: %s\n", g_sessionId);
            result= false;
         }
         else
         {
            memcpy( status.sessionId, sessionId, sizeof(RBISessionID) );
         }
      }
      else
      {
         memcpy( status.sessionId, session->sessionId, sizeof(RBISessionID) );
      }
         doc.Parse( msg->c_str() );
         
         if ( !doc.Error() )
         {
            TiXmlNode *pParent, *pChild;
            
            pParent= &doc;
            int t = pParent->Type();

            if ( t == TiXmlNode::TINYXML_DOCUMENT )
            {
               for ( pChild = pParent->FirstChild(); pChild != 0; pChild = pChild->NextSibling())
               {
                  if ( pChild->Type() == TiXmlNode::TINYXML_ELEMENT )
                  {
                     if ( strcmp(pChild->Value(), "adm:PlacementResponse") == 0 )
                     {
                        memset( &spotDfn, 0, sizeof(spotDfn) );
                        result= parseXmlPRSPPlacementResponse( pChild->ToElement(), session, &spotDfn );
                        if(!result && (spotDfn.spots[spotDfn.numSpots].detailCode == RBI_DetailCode_messageMalformed))
                        {
                           subStr = " reason=";
                           subStr = subStr + '\"';
                           size_t bytesReq = b64_encoded_size(g_failureReason.length());
                           base64DataPtr = (uint8_t*)calloc(bytesReq+1, sizeof(uint8_t));
                           if(base64DataPtr)
                           {
                              base64_encode((const  uint8_t*) g_failureReason.c_str(), g_failureReason.length(), base64DataPtr);
                              base64DataPtr[bytesReq] = '\0';
                              subStr = subStr + (char*)base64DataPtr;
                              free(base64DataPtr);
                              base64DataPtr = NULL;
                           }
                           else
                           {
                              printf("handleSCTE130PlacementResponse x :: Error memory not allocated. Req memory %d\n", g_failureReason.length() + 1);
                           }
                           subStr = subStr + '\"';
                           status.detailCode = RBI_DetailCode_placementResponseError;

                           printf("RBI LSA-PRE: Placement Response Error\n");
                        }
                        break;
                     }
                     else
                     {
                        printf( "ignorning unknown Element [%s]\n", pChild->Value() );
                     }
                  }
               }
               
               if ( result && spotDfn.numSpots > 0)
               {
                  printf("  sending spot definitions: spot count %d\n", spotDfn.numSpots );
                  
                  memcpy( spotDfn.sessionId, session->sessionId, sizeof(RBISessionID) );
                  spotDfn.triggerId= session->currentTriggerId;
                                
                  IARM_Bus_Call( RBI_RPC_NAME, RBI_DEFINE_SPOTS, &spotDfn, sizeof(spotDfn) );
               }
               if (spotDfn.numSpots <= 0)
               {
                  printf(" NOT sending spot definitions: spot count %d\n", spotDfn.numSpots );
                  g_PSNError = true;
               }
               if( result != true && g_PRespSpotError != true && g_PRespError != true)
               {
                  subStr = " reason=";
                  subStr = subStr + '\"';
                  size_t bytesReq = b64_encoded_size(msg->length());
                  base64DataPtr = (uint8_t*)calloc(bytesReq+1, sizeof(uint8_t));
                  if(base64DataPtr)
                  {
                     base64_encode((const  uint8_t*) msg->c_str(), msg->length(), base64DataPtr);
                     base64DataPtr[bytesReq] = '\0';
                     subStr = subStr + (char*)base64DataPtr;
                     free(base64DataPtr);
                     base64DataPtr = NULL;
                  }
                  else
                  {
                     printf("handleSCTE130PlacementResponse 1 :: Error memory not allocated. Req memory %d\n", msg->length() + 1);
                  }
                  subStr = subStr + '\"';
                  status.detailCode = RBI_DetailCode_placementResponseError;

                  printf("RBI LSA-PRE: Placement Response Error\n");
               }
            }
         }
         else
         {
            subStr = " reason=";
            subStr = subStr +'\"';
            size_t bytesReq = b64_encoded_size(msg->length());
            base64DataPtr = (uint8_t*)calloc(bytesReq+1, sizeof(uint8_t));
            if(base64DataPtr)
            {
               base64_encode((const  uint8_t*) msg->c_str(), msg->length(), base64DataPtr);
               base64DataPtr[bytesReq] = '\0';
               subStr = subStr + (char*)base64DataPtr;
               free(base64DataPtr);
               base64DataPtr = NULL;
            }
            else
            {
               printf("handleSCTE130PlacementResponse 2 :: Error memory not allocated. Req memory %d\n", msg->length() + 1);
            }
            subStr = subStr + '\"';
            status.detailCode = RBI_DetailCode_invalidXMLInformation;

            printf("RBI LSA-PRE: Invalid XML Information\n");
            result = false;
         }
      }
      if(( !result && g_PRespSpotError != true && g_PRespError != true) || (!result && (spotDfn.spots[spotDfn.numSpots].detailCode == RBI_DetailCode_messageMalformed)))
      {
         status.action = RBI_SpotAction_replace;
         status.event = RBI_SpotEvent_insertionFailed;
         memcpy( status.deviceId, g_deviceId, sizeof(g_deviceId) );
         std::string str = "Message malformed - PlacementRequest message id=";
         str.append(g_preqMessageId);
         str = str + subStr;
         subStr = " PlacementRequestMessage=";
         subStr = subStr + '\"';

         size_t bytesReq = b64_encoded_size(g_PlacementReqMsg.length());
         base64DataPtr = (uint8_t*)calloc(bytesReq+1, sizeof(uint8_t));
         if(base64DataPtr)
         {
            base64_encode((const  uint8_t*) g_PlacementReqMsg.c_str(), g_PlacementReqMsg.length(), base64DataPtr);
            subStr = subStr + (char*)base64DataPtr;
            free(base64DataPtr);
            base64DataPtr = NULL;
         }
         else
         {
            printf("handleSCTE130PlacementResponse 3 :: Error memory not allocated. Req memory %d\n", g_PlacementReqMsg.length() + 1);
         }
         subStr = subStr + '\"';
         str = str + subStr;

         if(str.size() > 0)
         {
            status.detailNotePtr = (char*)calloc(str.size() + 1, sizeof(char));
            if(status.detailNotePtr)
            {
               strncpy(status.detailNotePtr, str.c_str(), str.size());
            }
            else
            {
               printf("handleSCTE130PlacementResponse 4:: Error memory not allocated. Req memory %d\n", str.size() + 1);
            }
         }
         g_PRespError = true;
         g_PSNError = true;
         std::string *msg= createPlacementStatusNotification(session, &status,false );
         if ( msg )
         {
            handleSCTE130PlacementStatus( session, msg );
         }
         if(status.detailNotePtr)
         {
            free(status.detailNotePtr);
            status.detailNotePtr = NULL;
         }
      }
}

/* 
 * Process incomming SCTE-130 Placement Status Notification
 */
void handleSCTE130PlacementStatus( SessionData *session, std::string *msg )
{
   printf( "-----------------------------------------------------------------------------\n" );
   printf( "%s\n", msg->c_str() );

   std::string response;
      
   if ( postMessage( g_endPointStatus, msg, &response, false, true ) )
   {
      printf( "-----------------------------------------------------------------------------\n" );
      printf( "%s\n", response.c_str() );
   }
}

char* formatTime( long long utcTime, bool useUtc, char *buffer )
{
   char *timeString= 0;
   time_t tv_sec;
   int tv_usec; 
   tv_sec= utcTime/1000LL;
   tv_usec= utcTime%1000LL;
   struct tm tmTime;
   int tzOffset = g_tzOffset;
   if ( useUtc )
   {
      if ( gmtime_r( &tv_sec, &tmTime) )
      {
         sprintf( buffer, "%4d-%02d-%02dT%02d:%02d:%02d.%03d+00:00",
                  tmTime.tm_year+1900,
                  tmTime.tm_mon+1,
                  tmTime.tm_mday,
                  tmTime.tm_hour,
                  tmTime.tm_min,
                  tmTime.tm_sec,
                  tv_usec );
      }
      timeString= buffer;
   }
   else
   {
      if ( localtime_r( &tv_sec, &tmTime) )
      {
         sprintf( buffer, "%4d-%02d-%02dT%02d:%02d:%02d.%03d%c%02d:00",
                  tmTime.tm_year+1900,
                  tmTime.tm_mon+1,
                  tmTime.tm_mday,
                  tmTime.tm_hour,
                  tmTime.tm_min,
                  tmTime.tm_sec,
                  tv_usec,
                  ((tzOffset >= 0) ? '+' : '-'), //tzoffset is minutes west, so earlier than Greenwich
                  ((tzOffset >= 0) ? tzOffset/60 : -tzOffset/60)
                   );
      }
      
      timeString= buffer;
   }
   
   return timeString;
}

char* formatDuration( long long duration, char *buffer )
{
    int durationInSecs = 0;
    int secs = 0;
    int mins = 0;
    int hrs = 0;
    int millisecs = 0;
    
    char *timeString= 0;
   
    
    durationInSecs = (duration/90000LL);
    millisecs = ((duration/90000.0) - durationInSecs) * 1000;
    secs = durationInSecs % 60;
    mins = (durationInSecs/60) % 60;
    hrs = (durationInSecs/3600) % 24;
            
    if(hrs == 0)
    {
        if(mins == 0)
        {
            sprintf(buffer, "PT%02d.%03dS", secs, millisecs);
        }
        else
        {
            sprintf(buffer, "PT%02dM%02d.%03dS",  mins, secs, millisecs);
        }
    }
    else
    {
        sprintf(buffer, "PT%02dH%02dM%02d.%03dS", hrs, mins, secs, millisecs);
    }
   timeString= buffer;
   
   return timeString;
}

const int getDetailCode( int detailCode )
{
   int code;

   switch( detailCode )
   {
      case RBI_DetailCode_assetUnreachable:
      case RBI_DetailCode_matchingAudioAssetNotFound:
      case RBI_DetailCode_PRSPAudioAssetNotFound:
      case RBI_DetailCode_matchingAssetNotFound:
      case RBI_DetailCode_missingContentLocation:
      case RBI_DetailCode_missingContent:
      case RBI_DetailCode_missingMediaType:
      case RBI_DetailCode_httpResponseTimeout:
      case RBI_DetailCode_assetDurationExceeded:
      case RBI_DetailCode_assetDurationInadequate:
         code= 11;
         break;

      case RBI_DetailCode_messageRefMismatch:
      case RBI_DetailCode_badMessageRefFormat:
         code= 14;
         break;

      case RBI_DetailCode_responseTimeout:
      case RBI_DetailCode_unableToSplice_missed_prefetchWindow:
      case RBI_DetailCode_unableToSplice_missed_spotWindow:
         code= 17;
         break;

      case RBI_DetailCode_invalidVersion:
      case RBI_DetailCode_invalidXMLInformation:
      case RBI_DetailCode_placementResponseError:
         code= 2;
         break;

      case RBI_DetailCode_insertionTimeDeltaExceeded:
         code= 18;
         break;

      case RBI_DetailCode_patPmtChanged:
      case RBI_DetailCode_assetCorrupted:
         code= 3011;
         break;

      case RBI_DetailCode_abnormalTermination:
         code= 3004;
         break;

      default:
         code = 0;
         break;
   }

   return code;
}

std::string* createPlacementRequest( SessionData *session, RBI_InsertionOpportunity_Data *opportunity )
{
   std::string *msg= 0;
   int errorLine;
   TiXmlDocument doc;
   TiXmlDeclaration decl( "1.0", "UTF-8", "" );
   char work[RBI_MAX_DATA] = { 0 };
   TiXmlElement *elmnt= 0;
   TiXmlText *text= 0;
   TiXmlElement *systemContext= 0;
   TiXmlElement *serviceElmnt= 0;
   TiXmlElement *clientElmnt= 0;
   TiXmlElement *placementOpportunity= 0;
   TiXmlElement *coreExt= 0;
   TiXmlElement *playPositionsAvailBinding= 0;
   TiXmlElement *playPositionStart= 0;
   TiXmlElement *opportunityEntertainment = 0;
   TiXmlElement *opportunityConstraints = 0;
   TiXmlElement *content= 0;
   std::string liveDVRString;
   unsigned int i=0;
   bool distributorSignal = false;

   uuid_generate( session->preqMessageId );
   uuid_generate( session->popId );

   formatTime(opportunity->utcTimeOpportunity, true, work);
   if(opportunity->poData.poType == 0x36)
      distributorSignal = true;

   printf("utcTimeOpportunity %s\n", work);
   if ( doc.InsertEndChild( decl ) )
   {
      TiXmlElement *preq= new TiXmlElement( "adm:PlacementRequest" );
      if ( !preq )
      {
         errorLine= __LINE__;
         goto exit;
      }
      
      preq->SetAttribute("xmlns:xsi", XMLSCHEMA_INSTANCE.value );
      preq->SetAttribute("xsi:schemaLocation", SCHEMA_LOCATION.value );
      preq->SetAttribute("xmlns:adm", SCHEMAS_ADMIN.value );
      preq->SetAttribute("xmlns:core", SCHEMAS_CORE.value );
      preq->SetAttribute(XMLNS_SCHEMA.value, SCHEMAS_NGOD.value );
      
      uuid_unparse( session->preqMessageId, work );
      g_preqMessageId= strdup(work);
      preq->SetAttribute("messageId", work );
      preq->SetAttribute("identity", "86CF2E98-AEBA-4C3A-A3D4-616CF7D74A79" );
      preq->SetAttribute("version", "1.1");
      preq->SetAttribute("system", "QAMRDK");
      
      systemContext= new TiXmlElement("adm:SystemContext");
      if ( !systemContext )
      {
         errorLine= __LINE__;
         goto exit;
      }

      if((strlen(opportunity->poData.zone) > 0) && strncmp(opportunity->poData.zone, ADZONE_STR, strlen(ADZONE_STR)) == 0)
      {
         elmnt= new TiXmlElement("adm:Zone");
         if ( !elmnt )
         {
            errorLine= __LINE__;
            goto exit;
         }
         text= new TiXmlText(strtok(opportunity->poData.zone, ADZONE_STR));
         if ( !text )
         {
            errorLine= __LINE__;
            goto exit;
         }
         elmnt->LinkEndChild(text);
         text= 0;
         systemContext->LinkEndChild(elmnt);
         elmnt= 0;
      }

      elmnt= new TiXmlElement("adm:Session");
      if ( !elmnt )
      {
         errorLine= __LINE__;
         goto exit;
      }
      memcpy( g_sessionId, session->sessionId, sizeof(RBISessionID) );
      uuid_unparse( session->sessionId, g_sessionId);
      uuid_unparse( session->sessionId, work );
      text= new TiXmlText(work);
      if ( !text )
      {
         errorLine= __LINE__;
         goto exit;
      }
      elmnt->LinkEndChild(text);
      text= 0;
      systemContext->LinkEndChild(elmnt);
      elmnt= 0;
      preq->LinkEndChild(systemContext);
      systemContext= 0;
      
      serviceElmnt= new TiXmlElement("adm:Service");
      if ( !serviceElmnt )
      {
         errorLine= __LINE__;
         goto exit;
      }
      int serviceid= -1;
      if ( sscanf( opportunity->serviceURI, "ocap://0x%X", &serviceid ) != 1 )
      {
         errorLine= __LINE__;
         goto exit;
      }
      if((strlen(opportunity->poData.streamId) > 0) && strncmp(opportunity->poData.streamId, MERLIN_STR, strlen(MERLIN_STR)) == 0)
      {
         printf("opportunity->poLen %d %s\n", strlen(opportunity->poData.streamId), opportunity->poData.streamId);
         serviceElmnt->SetAttribute("id", opportunity->poData.streamId );
      }
      else
      {
         sprintf(work,"urn:cibo:sourceid:%d",serviceid);
         serviceElmnt->SetAttribute("id", work );
      }
      g_serviceId = serviceid;
 
      elmnt= new TiXmlElement("adm:ProductType");
      if ( !elmnt )
      {
         errorLine= __LINE__;
         goto exit;
      }
      text= new TiXmlText("LINEAR_T6");
      if ( !text )
      {
         errorLine= __LINE__;
         goto exit;
      }
      elmnt->LinkEndChild(text);
      text= 0;
      serviceElmnt->LinkEndChild(elmnt);         
      elmnt= 0;

      if( strlen(opportunity->poData.serviceSource) > 0 )
      {
         coreExt= new TiXmlElement("core:Ext");
         if ( !coreExt )
         {
            errorLine= __LINE__;
            goto exit;
         }
         elmnt= new TiXmlElement("cmcst:ServiceSource");
         if ( !elmnt )
         {
            errorLine= __LINE__;
            goto exit;
         }
         elmnt->SetAttribute("id", opportunity->poData.serviceSource );
         coreExt->LinkEndChild(elmnt);
         elmnt= 0;
         serviceElmnt->LinkEndChild(coreExt);
         coreExt=0;
      }

      preq->LinkEndChild(serviceElmnt);
      serviceElmnt= 0;

      clientElmnt= new TiXmlElement("adm:Client");
      if ( !clientElmnt )
      {
         errorLine= __LINE__;
         goto exit;
      }
      elmnt= new TiXmlElement("core:CurrentDateTime");
      if ( !elmnt )
      {
         errorLine= __LINE__;
         goto exit;
      }
      if ( !formatTime(opportunity->utcTimeCurrent, false, work) )
      {
         errorLine= __LINE__;
         goto exit;
      }
      g_PReqtime = std::string(work);
      text= new TiXmlText(work);
      if ( !text )
      {
         errorLine= __LINE__;
         goto exit;
      }
      elmnt->LinkEndChild(text);
      text= 0;
      clientElmnt->LinkEndChild(elmnt);
      elmnt= 0;
      elmnt= new TiXmlElement("adm:TerminalAddress");
      if ( !elmnt )
      {
         errorLine= __LINE__;
         goto exit;
      }
      elmnt->SetAttribute("type","DEVICEID");
      sprintf(work,"%02X%02X%02X%02X%02X%02X",
              opportunity->deviceId[0],
              opportunity->deviceId[1],
              opportunity->deviceId[2],
              opportunity->deviceId[3],
              opportunity->deviceId[4],
              opportunity->deviceId[5] );
      memcpy( g_deviceId, opportunity->deviceId,sizeof(opportunity->deviceId) );
      text= new TiXmlText(work);
      if ( !text )
      {
         errorLine= __LINE__;
         goto exit;
      }
      elmnt->LinkEndChild(text);
      text= 0;
      clientElmnt->LinkEndChild(elmnt);
      elmnt= 0;

      for(i = 0; i < opportunity->totalReceivers; i++)
      {
         elmnt= new TiXmlElement("adm:TargetCode");
         if ( !elmnt )
         {
            errorLine= __LINE__;
            goto exit;
         }
         elmnt->SetAttribute("key",ALTCON_RECEIVER.value);

         if(strcmp(opportunity->recActivity[i].activityState, "Active") == 0 )
            elmnt->SetAttribute("activityState", "ACTIVE");
         else
            elmnt->SetAttribute("activityState", "INACTIVE");

         elmnt->SetAttribute("tuneTime", opportunity->recActivity[i].tuneTime);
         elmnt->SetAttribute("lastActivityTime", opportunity->recActivity[i].lastActivityTime );
         if(opportunity->recActivity[i].liveDVRState.DVR)
         {
            switch(opportunity->recActivity[i].liveDVRState.Live_TSB)
            {
               case RBI_LIVE:
                  liveDVRString = std::string("LIVE-DVR");
                  break;

               case RBI_TSB:
                  liveDVRString = std::string("TSB-DVR");
                  break;

               default:
                  liveDVRString = std::string("DVR");
                  break;               
            }
         }
         else
         {
            switch(opportunity->recActivity[i].liveDVRState.Live_TSB)
            {
               case RBI_LIVE:
                  liveDVRString = std::string("LIVE");
                  break;

               case RBI_TSB:
                  liveDVRString = std::string("TSB");
                  break;
			}
         }
         elmnt->SetAttribute("liveDVRState", liveDVRString.c_str());

         text= new TiXmlText(opportunity->recActivity[i].receiverId);
         if ( !text )
         {
            errorLine= __LINE__;
            goto exit;
         }
         elmnt->LinkEndChild(text);
         text= 0;
         clientElmnt->LinkEndChild(elmnt);
         elmnt= 0;
      }
      preq->LinkEndChild(clientElmnt);
      clientElmnt= 0;

      placementOpportunity= new TiXmlElement("adm:PlacementOpportunity");
      if ( !placementOpportunity )
      {
         errorLine= __LINE__;
         goto exit;
      }

      if(distributorSignal)
      {
         uuid_unparse( session->popId, work );
         g_popId= strdup(work);
         placementOpportunity->SetAttribute("id", work);
      }
      else
      {
         placementOpportunity->SetAttribute("id", opportunity->poData.elementId);
      }

      placementOpportunity->SetAttribute("serviceRegistrationRef","-");

      if (distributorSignal == false)
      {
         if(strlen(opportunity->poData.elementBreakId) > 0)
            placementOpportunity->SetAttribute("breakId", opportunity->poData.elementBreakId);

	  /** Add Entertainment */
         opportunityEntertainment = new TiXmlElement("adm:Entertainment");
         if ( !opportunityEntertainment )
         {
            errorLine= __LINE__;
            goto exit;
         }
         content= new TiXmlElement("core:Content");
         if ( !content )
         {
            errorLine= __LINE__;
            goto exit;
         }
         if(strlen((char*)opportunity->poData.contentId) > 0 )
         {
            sprintf(work,"0x%s", opportunity->poData.contentId);
            content->SetAttribute("id", work);
         }

         i=0;
         do
         {
            elmnt= new TiXmlElement("core:SegmentationUpid");
            if ( !elmnt )
            {
               errorLine= __LINE__;
               goto exit;
            }
            sprintf(work,"%d", opportunity->poData.segmentUpidElement.segments[i].upidType );
            elmnt->SetAttribute("type", work);

            if(i == 0)
            {
               sprintf(work,"%u", opportunity->poData.segmentUpidElement.eventId);
               elmnt->SetAttribute("eventID", work);

               sprintf(work,"0x%X", opportunity->poData.segmentUpidElement.typeId);
               elmnt->SetAttribute("typeID", work);

               sprintf(work,"%d", opportunity->poData.segmentUpidElement.segmentNum);
               elmnt->SetAttribute("segmentNum", work);

               sprintf(work,"%d", opportunity->poData.segmentUpidElement.segmentsExpected);
               elmnt->SetAttribute("segmentsExpected", work);
            }

            if(strlen(reinterpret_cast<const char*>(opportunity->poData.segmentUpidElement.segments[i].segmentUpid)) > 0)
            {
               text= new TiXmlText(reinterpret_cast<const char*>(opportunity->poData.segmentUpidElement.segments[i].segmentUpid));
               if ( !text )
               {
                  errorLine= __LINE__;
                  goto exit;
               }
               elmnt->LinkEndChild(text);
               text= 0;
            }
            content->LinkEndChild(elmnt);
            elmnt= 0; 
            i++;
         }while(i < opportunity->poData.segmentUpidElement.totalSegments);

         opportunityEntertainment->LinkEndChild(content);
         content= 0;

         placementOpportunity->LinkEndChild(opportunityEntertainment);
         opportunityEntertainment= 0;
      }

      /** Add OpportunityConstraints */  
      {
            opportunityConstraints = new TiXmlElement("adm:OpportunityConstraints");
            if ( !opportunityConstraints )
            {
               errorLine= __LINE__;
               goto exit;
            }            
            elmnt= new TiXmlElement("core:Duration");
            if ( !elmnt )
            {
               errorLine= __LINE__;
               goto exit;
            }
            if ( !formatDuration(opportunity->poData.poDuration, work) )
            {
               errorLine= __LINE__;
               goto exit;
            }
            text= new TiXmlText(work);
            if ( !text )
            {
               errorLine= __LINE__;
               goto exit;
            }
            elmnt->LinkEndChild(text);
            text= 0;
            opportunityConstraints->LinkEndChild(elmnt);
            elmnt= 0;

            elmnt= new TiXmlElement("adm:Scope");
            if ( !elmnt )
            {
               errorLine= __LINE__;
               goto exit;
            }
            text = 0;
            if( distributorSignal )
            {
               text= new TiXmlText(DISTRIBUTOR);
            }
            else
            {
               text= new TiXmlText(PROVIDER);
            }
            if ( !text )
            {
               errorLine= __LINE__;
               goto exit;
            }
            elmnt->LinkEndChild(text);
            text= 0;
            opportunityConstraints->LinkEndChild(elmnt);
            elmnt= 0;

            placementOpportunity->LinkEndChild(opportunityConstraints);
            opportunityConstraints= 0;
      }

      if( distributorSignal )
      {
         coreExt= new TiXmlElement("core:Ext");
         if ( !coreExt )
         {
            errorLine= __LINE__;
            goto exit;
         }

         playPositionsAvailBinding= new TiXmlElement("cmcst:PlayPositionsAvailBinding");
         if ( !playPositionsAvailBinding )
         {
            errorLine= __LINE__;
            goto exit;
         }

         playPositionStart= new TiXmlElement("cmcst:PlayPositionStart");
         if ( !playPositionStart )
         {
            errorLine= __LINE__;
            goto exit;
         }

         elmnt= new TiXmlElement("cmcst:SignalId");
         if ( !elmnt )
         {
            errorLine= __LINE__;
            goto exit;
         }
         //Removes the "SIGNAL:" preamble text from the PlacementRequest SignalId attr if present
         //the assumption is the string appears at the beginning of the upid
         char signal[] = "SIGNAL:";
         int SIGNALLength = strlen(signal);
         char* pSig = strstr( (char*)opportunity->poData.signalId, (const char*)signal );
         int len = strlen((char*)opportunity->poData.signalId);
         if( !pSig )
         {
            memcpy( work, opportunity->poData.signalId, len );
            work[len] = '\0';
         }
         else
         {
            memcpy( work, opportunity->poData.signalId + SIGNALLength, len - SIGNALLength );
            work[len- SIGNALLength] = '\0';
         }
         text= new TiXmlText((const char*)work);
         if ( !text )
         {
            errorLine= __LINE__;
            goto exit;
         }
         elmnt->LinkEndChild(text); 
         playPositionStart->LinkEndChild(elmnt);
         text= 0;
         elmnt= 0;

         playPositionsAvailBinding->LinkEndChild(playPositionStart);
         coreExt->LinkEndChild(playPositionsAvailBinding);
         placementOpportunity->LinkEndChild(coreExt);
         playPositionStart= 0;
         playPositionsAvailBinding= 0;
         coreExt= 0;
      }

      preq->LinkEndChild(placementOpportunity);
      placementOpportunity= 0;
      
      doc.LinkEndChild(preq);
   }
   
   {
      TiXmlPrinter printer;
      doc.Accept( &printer );
      
      msg= new std::string( printer.CStr() );
   }

exit:

   if ( !msg )
   {
      printf("Error: unable to generate placement request msg: error %d\n", errorLine );
      
      if ( text )
      {
         delete text;
      }
      if ( elmnt )
      {
         delete elmnt;
      }
      if ( systemContext )
      {
         delete systemContext;
      }
      if ( serviceElmnt )
      {
         delete serviceElmnt;
      }
      if ( clientElmnt )
      {
         delete clientElmnt;
      }
      if ( placementOpportunity )
      {
         delete placementOpportunity;
      }
      if ( coreExt )
      {
         delete coreExt;
      }
      if ( playPositionsAvailBinding )
      {
         delete playPositionsAvailBinding;
      }
      if ( playPositionStart )
      {
         delete playPositionStart;
      }
      if ( opportunityConstraints )
      {
         delete opportunityConstraints;
      }      
   }
   
   return msg;
}

std::string* createPlacementStatusNotification( SessionData *session, RBI_InsertionStatus_Data *status,bool placementResponseTimeout )
{
   std::string *msg= 0;
   int errorLine;
   TiXmlDocument doc;
   TiXmlDeclaration decl( "1.0", "UTF-8", "" );
   uuid_t messageId;
   const char *noteText;
   char work[RBI_MAX_CONTENT_URI_LEN] = "";
   TiXmlElement *elmnt= 0;
   TiXmlText *text= 0;
   TiXmlElement *playData= 0;
   TiXmlElement *systemContext= 0;
   TiXmlElement *clientElmnt= 0;
   TiXmlElement *spotScopedEvents= 0;
   TiXmlElement *spot= 0;
   TiXmlElement *content= 0;
   TiXmlElement *events= 0;
   TiXmlElement *placementStatusEvent= 0;
   TiXmlElement *systemEvent= 0;
   TiXmlElement *statusCode= 0;
   int statusDetailCode=0;
   uuid_generate( messageId );
   uint8_t *protectedDataPtr = NULL;
   char *restoreDataPtr = NULL;
   bool restoreDataFlag = false;

   if ( doc.InsertEndChild( decl ) )
   {
      TiXmlElement *psn= new TiXmlElement( "adm:PlacementStatusNotification" );
      if ( !psn )
      {
         errorLine= __LINE__;
         goto exit;
      }
      
      uuid_unparse( messageId, work );
      psn->SetAttribute("messageId", work);
      psn->SetAttribute("identity", "86CF2E98-AEBA-4C3A-A3D4-616CF7D74A79");
      psn->SetAttribute("version", "1.1");
      psn->SetAttribute("system", "QAMRDK");
      psn->SetAttribute("xmlns:adm", SCHEMAS_ADMIN.value );
      psn->SetAttribute("xmlns:core", SCHEMAS_CORE.value );
      psn->SetAttribute("xmlns:xsi", XMLSCHEMA_INSTANCE.value );
      
      playData= new TiXmlElement("adm:PlayData");
      if ( !playData )
      {
         errorLine= __LINE__;
         goto exit;
      }
      if (session && (strlen(session->prspIdentity) > 0))
      {
         playData->SetAttribute("identityADS", session->prspIdentity);
      }
      else
      {
         printf("Using default identity. session is %s \n", (session == NULL) ? "NULL" : "Not NULL");
         playData->SetAttribute("identityADS","FreeWheel-ADS");
      }

      systemContext= new TiXmlElement("adm:SystemContext");
      if ( !systemContext )
      {
         errorLine= __LINE__;
         goto exit;
      }
      elmnt= new TiXmlElement("adm:Session");
      if ( !elmnt )
      {
         errorLine= __LINE__;
         goto exit;
      }
      if(session == NULL)
      {
         printf("Session is NULL\n");
         uuid_unparse( status->sessionId, work );
         status->assetID[0]= '\0';
         status->providerID[0]= '\0';
         status->trackingIdString[0]= '\0';
      }
      else
      {
         uuid_unparse( session->sessionId, work );
      }
      text= new TiXmlText(work);
      if ( !text )
      {
         errorLine= __LINE__;
         goto exit;
      }
      elmnt->LinkEndChild(text);
      text= 0;

      systemContext->LinkEndChild(elmnt);
      elmnt= 0;
      playData->LinkEndChild(systemContext);
      systemContext= 0;

      clientElmnt= new TiXmlElement("adm:Client");
      if ( !clientElmnt )
      {
         errorLine= __LINE__;
         goto exit;
      }
      elmnt= new TiXmlElement("adm:TerminalAddress");
      if ( !elmnt )
      {
         errorLine= __LINE__;
         goto exit;
      }
      elmnt->SetAttribute("type","DEVICEID");
      sprintf(work,"%02X%02X%02X%02X%02X%02X",
              status->deviceId[0],
              status->deviceId[1],
              status->deviceId[2],
              status->deviceId[3],
              status->deviceId[4],
              status->deviceId[5] );
      text= new TiXmlText(work);
      if ( !text )
      {
         errorLine= __LINE__;
         goto exit;
      }
      elmnt->LinkEndChild(text);
      text= 0;
      clientElmnt->LinkEndChild(elmnt);
      elmnt= 0;
      playData->LinkEndChild(clientElmnt);
      clientElmnt= 0;

      if ((status->event != RBI_SpotEvent_endAll ) &&
          (status->detailCode != RBI_DetailCode_insertionTimeDeltaExceeded ) &&
          (status->detailCode != RBI_DetailCode_invalidXMLInformation) &&
          (status->detailCode != RBI_DetailCode_messageRefMismatch) &&
          (status->detailCode != RBI_DetailCode_badMessageRefFormat) &&
          (status->detailCode != RBI_DetailCode_placementResponseError) &&
          (status->detailCode != RBI_DetailCode_responseTimeout) &&
          (status->detailCode != RBI_DetailCode_invalidVersion))
      {
          spotScopedEvents= new TiXmlElement("adm:SpotScopedEvents");
          if ( !spotScopedEvents )
          {
              errorLine= __LINE__;
              goto exit;
          }
          spot= new TiXmlElement("adm:Spot");
          if ( !spot )
          {
              errorLine= __LINE__;
              goto exit;
          }
          content= new TiXmlElement("core:Content");
          if ( !content )
          {
              errorLine= __LINE__;
              goto exit;
          }
          if (( status->action == RBI_SpotAction_replace ) || (status->action == RBI_SpotAction_fixed))
          {
              elmnt= new TiXmlElement("core:AssetRef");
              if ( !elmnt )
              {
                  errorLine= __LINE__;
                  goto exit;
              }
              elmnt->SetAttribute("assetID", status->assetID );
              elmnt->SetAttribute("providerID", status->providerID );
              content->LinkEndChild(elmnt);
              elmnt= 0;
          }

          elmnt= new TiXmlElement("core:Tracking");
          if ( !elmnt )
          {
              errorLine= __LINE__;
              goto exit;
          }

          text= new TiXmlText( status->trackingIdString );
          if ( !text )
          {
              errorLine= __LINE__;
              goto exit;
          }
          elmnt->LinkEndChild(text);
          text= 0;
          content->LinkEndChild(elmnt);
          elmnt= 0;

          spot->LinkEndChild(content);
          content= 0;
          spotScopedEvents->LinkEndChild(spot);
          spot= 0;
      }
      events= new TiXmlElement("adm:Events");
      if ( !events )
      {
         errorLine= __LINE__;
         goto exit;
      }
      if ( (status->event == RBI_SpotEvent_insertionStarted) ||
           (status->event == RBI_SpotEvent_endAll)||
           (status->event == RBI_SpotEvent_insertionStopped) )
      {
         placementStatusEvent= new TiXmlElement("adm:PlacementStatusEvent");
         if ( !placementStatusEvent )
         {
            errorLine= __LINE__;
            goto exit;
         }
         if ( !formatTime(status->utcTimeEvent, false, work) )
         {
            errorLine= __LINE__;
            goto exit;
         }
         placementStatusEvent->SetAttribute("time",work);
         if ( status->event == RBI_SpotEvent_insertionStarted )
         {
            placementStatusEvent->SetAttribute("type","startPlacement" );
         }
         else
         {
            if(status->event != RBI_SpotEvent_endAll)
            {
               placementStatusEvent->SetAttribute("type","endPlacement" );
            }
         }
         if (session)
         {
            uuid_unparse( session->preqMessageId, work );
            placementStatusEvent->SetAttribute("messageRef", work);
         }

         if(status->event == RBI_SpotEvent_endAll)
         {
            printf("RBI LSA-PRS: Event endAll\n");

            placementStatusEvent->SetAttribute("type","endAll" );
         }
         if ((status->event != RBI_SpotEvent_endAll ) && ( status->detailCode != RBI_DetailCode_insertionTimeDeltaExceeded ))
         {
            elmnt= new TiXmlElement("adm:SpotNPT");
            if ( !elmnt )
            {
               errorLine= __LINE__;
               goto exit;
            }
            sprintf( work,"%.1f", status->scale);
            elmnt->SetAttribute("scale",work);

            double spotNPT = (status->spotNPT/1000) + (status->spotNPT%1000)/1000.0;
            double percentage = status->spotDuration * 0.1; // 10%

            if ( status->event == RBI_SpotEvent_insertionStarted )
                strcpy(work, "0.0");
            else if ((status->detailCode != RBI_DetailCode_assetDurationInadequate) &&
                     (status->detailCode != RBI_DetailCode_assetDurationExceeded) &&
                     (INRANGE(spotNPT, (double)((status->spotDuration-percentage)/1000), (double)((status->spotDuration+percentage)/1000))))
               {
                  sprintf(work,"%d.0", status->spotDuration/1000 );
               }
            else
            {
               sprintf(work,"%d.%d", (int)(status->spotNPT/1000),(int)(10*(status->spotNPT % 1000)/1000.0));
            }
            text= new TiXmlText(work);
            elmnt->LinkEndChild(text);
            text= 0;
            placementStatusEvent->LinkEndChild(elmnt);
            elmnt= 0;
         }
         events->LinkEndChild(placementStatusEvent);
         placementStatusEvent= 0;

         if((status->event == RBI_SpotEvent_insertionStopped) &&
            ((status->detailCode == RBI_DetailCode_assetDurationInadequate) ||
             (status->detailCode == RBI_DetailCode_abnormalTermination) ||
             (status->detailCode == RBI_DetailCode_assetDurationExceeded)))
         {
            systemEvent= new TiXmlElement("adm:SystemEvent");
            if ( !systemEvent )
            {
               errorLine= __LINE__;
               goto exit;
            }
            systemEvent->SetAttribute("type","status");
            if ( !formatTime(status->utcTimeEvent, false, work) )
            {
               errorLine= __LINE__;
               goto exit;
            }
            systemEvent->SetAttribute("time",work);
            statusCode= new TiXmlElement("core:StatusCode");
            if ( !statusCode )
            {
               errorLine= __LINE__;
               goto exit;
            }
            statusCode->SetAttribute("class","1");
            statusDetailCode = getDetailCode(status->detailCode);
            sprintf(work,"%d",statusDetailCode);
            statusCode->SetAttribute("detail",work);

            elmnt= new TiXmlElement("core:Note");
            if ( !elmnt )
            {
               errorLine= __LINE__;
               goto exit;
            }
            noteText= status->detailNotePtr;
            if ( noteText )
            {
               text= new TiXmlText(noteText);
               elmnt->LinkEndChild(text);
               text= 0;
            }
            statusCode->LinkEndChild(elmnt);
            elmnt= 0;
            systemEvent->LinkEndChild(statusCode);
            statusCode= 0;
            events->LinkEndChild(systemEvent);
            systemEvent= 0;
         }
      }
      if ((status->event == RBI_SpotEvent_insertionFailed )||
          (status->event == RBI_SpotEvent_insertionStatusUpdate))
      {
        
         struct timeval tv;
         struct timezone tz;
         gettimeofday( &tv, &tz );
         status->utcTimeEvent= UTCMILLIS(tv);

         systemEvent= new TiXmlElement("adm:SystemEvent");
         if ( !systemEvent )
         {
            errorLine= __LINE__;
            goto exit;
         }
         systemEvent->SetAttribute("type","status");
         if ( !formatTime(status->utcTimeEvent, false, work) )
         {
            errorLine= __LINE__;
            goto exit;
         }
         systemEvent->SetAttribute("time",work);
         
         statusCode= new TiXmlElement("core:StatusCode");
         if ( !statusCode )
         {
            errorLine= __LINE__;
            goto exit;
         }
         statusCode->SetAttribute("class","1");
         if (( status->detailCode == RBI_DetailCode_responseTimeout ) &&
            placementResponseTimeout == true)
         {
            std::string str  = "Placement Response not received by client. PlacementRequest message id=";
            str.append(g_preqMessageId);
            str.append(", timesent=");
            str.append(g_PReqtime);
            str = str + " PlacementRequestMessage=";
            str = str + '\"';

            size_t bytesReq = b64_encoded_size(g_PlacementReqMsg.length());
            protectedDataPtr = (uint8_t*)calloc(bytesReq+1, sizeof(uint8_t));
            if(protectedDataPtr)
            {
               base64_encode((const  uint8_t*) g_PlacementReqMsg.c_str(), g_PlacementReqMsg.length(), protectedDataPtr);
               protectedDataPtr[bytesReq] = '\0';
               str = str + (char*)protectedDataPtr;
               free(protectedDataPtr);
               protectedDataPtr = NULL;
            }
            else
            {
               printf("createPlacementStatusNotification 1:: Error memory not allocated. Req memory %d\n", g_PlacementReqMsg.length() + 1);
            }
            str = str + '\"';
            if(str.size() > 0)
            {
               restoreDataPtr = status->detailNotePtr;
               status->detailNotePtr = NULL;

               status->detailNotePtr = (char*)calloc(str.size() + 1, sizeof(char));
               if(status->detailNotePtr)
               {
                  restoreDataFlag = true;
                  strncpy(status->detailNotePtr, str.c_str(), str.size());
               }
               else
               {
                  printf("createPlacementStatusNotification 2 :: Error memory not allocated. Req memory %d\n", str.size() + 1);
               }
            }
         }
         statusDetailCode = getDetailCode(status->detailCode);
         sprintf(work,"%d",statusDetailCode);
         statusCode->SetAttribute("detail",work);

         elmnt= new TiXmlElement("core:Note");
         if ( !elmnt )
         {
            errorLine= __LINE__;
            goto exit;
         }
         noteText= status->detailNotePtr;
         if ( noteText )
         {
            text= new TiXmlText(noteText);
            elmnt->LinkEndChild(text);
            text= 0;
         }
         statusCode->LinkEndChild(elmnt);
         elmnt= 0;
         systemEvent->LinkEndChild(statusCode);
         statusCode= 0;
         events->LinkEndChild(systemEvent);
         systemEvent= 0;
      }
      
      if ((status->event != RBI_SpotEvent_endAll ) &&
          (status->detailCode != RBI_DetailCode_insertionTimeDeltaExceeded ) &&
          (status->detailCode != RBI_DetailCode_invalidXMLInformation) &&
          (status->detailCode != RBI_DetailCode_messageRefMismatch) &&
          (status->detailCode != RBI_DetailCode_badMessageRefFormat) &&
          (status->detailCode != RBI_DetailCode_placementResponseError) &&
          (status->detailCode != RBI_DetailCode_responseTimeout) &&
          (status->detailCode != RBI_DetailCode_invalidVersion))
      {
         spotScopedEvents->LinkEndChild(events);
         events= 0;
         playData->LinkEndChild(spotScopedEvents);
         spotScopedEvents= 0;
      }
      else
      {
         playData->LinkEndChild(events);
         events= 0;
      }
      psn->LinkEndChild(playData);
      playData= 0;

      doc.LinkEndChild(psn);
   }

   {
      TiXmlPrinter printer;
      doc.Accept( &printer );
      
      msg= new std::string( printer.CStr() );
      std::string replace = "\"";
      std::string search = "&quot;";
      size_t pos = 0;
      while ((pos = msg->find(search, pos)) != std::string::npos)
      {
         msg->replace(pos, search.length(), replace);
         pos += replace.length();
      }
   }

exit:

   if ( !msg )
   {
      printf("Error: unable to generate placement status msg: error %d\n", errorLine );

      if ( elmnt )
      {
         delete elmnt;
      }
      if ( text )
      {
         delete text;
      }
      if ( playData )
      {
         delete playData;
      }
      if ( systemContext )
      {
         delete systemContext;
      }
      if ( clientElmnt )
      {
         delete clientElmnt;
      }
      if ( spotScopedEvents )
      {
         delete spotScopedEvents;
      }
      if ( spot )
      {
         delete spot;
      }
      if ( content )
      {
         delete content;
      }
      if ( events )
      {
         delete events;
      }
      if ( placementStatusEvent )
      {
         delete placementStatusEvent;
      }
      if ( statusCode )
      {
         delete statusCode;
      }
   }
   if(status->detailNotePtr && restoreDataFlag == true)
   {
      free(status->detailNotePtr);
      status->detailNotePtr = NULL;
   }
   if(restoreDataPtr)
       status->detailNotePtr = restoreDataPtr;

   return msg;
}

int postWriteCallback(char *data, size_t size, size_t nmemb, std::string *responseData )
{
   int rc= 0;
   int datalen= size*nmemb;
   
   if ( responseData ) 
   {
      responseData->append( data, datalen );
      rc= datalen;
   }

   return rc;
}

/* 
 * Post message to end-point.  If successful, the response body will be provided in 'response'.
 */
int postMessage( const char *endPoint, std::string *msg, std::string *response, bool isPReq, bool bRetry )
{
   bool result= false;
   CURLcode res = CURLE_OK;
   CURL *curl= 0;
   char errorBuffer[CURL_ERROR_SIZE];
   struct curl_slist *httpContentTypeHeader= NULL;
   int retryCount = g_retryCount;
   std::string line;
   std::string strToFind = "imagename:";
   std::ifstream versionFile ("/version.txt",std::ifstream::in);
   stringstream userAgent("QAMRDK/version unassigned");
   double totalTimeTaken = 0;

   if (versionFile.is_open())
   {
      while ( getline (versionFile,line) )
      {
         std::size_t found = line.find(strToFind);
         if (found!=std::string::npos)
         {
             userAgent << "QAMRDK/" << line.substr(strToFind.length());
         }
      }
      versionFile.close();
   }
   else
   {
      printf("Unable to open version.txt file\n");
   }

   do{
      errorBuffer[0]= '\0';

      response->clear();

      curl= curl_easy_init();
      if ( !curl )
      {
         printf("postMessage: unable to create curl session\n");
         goto exit;
      }

      curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
      curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
      curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, g_postTimeout);

      res = curl_easy_setopt(curl, CURLOPT_USERAGENT, (char*)userAgent.str().c_str());
      printf("set CURLOPT_USERAGENT to (%s)\n", (char*)userAgent.str().c_str() );
      if ( res != CURLE_OK )
      {
         printf("curl error %d setting CURLOPT_URL to (%s)\n", res, (char*)userAgent.str().c_str() );
         goto exit;
      }
      res= curl_easy_setopt(curl, CURLOPT_URL, endPoint );
      if ( res != CURLE_OK )
      {
         printf("postMessage: curl error %d setting CURLOPT_URL to (%s)\n", res, g_endPointRequest );
         goto exit;
      }

      httpContentTypeHeader= curl_slist_append( httpContentTypeHeader, "Expect:" );
      if( httpContentTypeHeader == NULL )
      {
         printf("Unable to append CURLOPT_HTTPHEADER Expect:\n");
         goto exit;
      }

      httpContentTypeHeader= curl_slist_append( httpContentTypeHeader, "Content-Type: application/xml" );
      if( httpContentTypeHeader == NULL )
      {
         printf("Unable to append CURLOPT_HTTPHEADER Content-Type attribute\n");
         goto exit;
      }
      res= curl_easy_setopt(curl, CURLOPT_HTTPHEADER, httpContentTypeHeader);
      if ( res != CURLE_OK )
      {
         printf("postMessage: curl error %d setting CURLOPT_HTTPHEADER to (%s)\n", res, g_endPointRequest );
         goto exit;
      }

      res= curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)msg->length());
      if ( res != CURLE_OK )
      {
         printf("postMessage: curl error %d setting CURLOPT_POSTFIELDSIZE to %d\n", res, msg->length() );
         goto exit;
      }

      res= curl_easy_setopt(curl, CURLOPT_POSTFIELDS, msg->c_str());
      if ( res != CURLE_OK )
      {
         printf("postMessage: curl error %d setting CURLOPT_POSTFIELDS to msg text\n", res );
         goto exit;
      }

      res= curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, postWriteCallback);
      if ( res != CURLE_OK )
      {
         printf("postMessage: curl error %d setting CURLOPT_WRITEFUNCTION\n", res );
         goto exit;
      }

      res= curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
      if ( res != CURLE_OK )
      {
         printf("postMessage: curl error %d setting CURLOPT_WRITEDATA\n", res );
         goto exit;
      }

      res= curl_easy_perform(curl);
      if ((res == CURLE_COULDNT_CONNECT) && (isPReq == true))
      {
         printf("postMessagePRQ: curl error %d from curl_easy_perform\n", res );
         goto exit;
      }
      else if ( res != CURLE_OK )
      {
         printf("postMessagePSN: curl error %d from curl_easy_perform\n", res );
         goto exit;
      }

      if(CURLE_OK == curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &totalTimeTaken))
      {
         printf("Time taken to post: %.1f\n", totalTimeTaken );
      }

      result= true;

      exit:

      if ( !result )
      {
         if ((bRetry) && (isPReq != true))
            --retryCount;

         if ( strlen(errorBuffer) > 0 )
         {
            printf("postMessage: post to (%s) failed\n", endPoint );
            printf("postMessage: curl error info: %s\n", errorBuffer );
         }
      }

      if ( curl )
      {
         curl_easy_cleanup( curl );
      }

      curl_slist_free_all( httpContentTypeHeader);
      httpContentTypeHeader = NULL;
      printf("Retry required[%s] Retry attempts remaining:[%d] Error[%d]\n", (((isPReq!= true) && (result != true)) ? "Yes" : "No"), (retryCount+1), !result);
   }while(bRetry && ((retryCount >= 0) && !result));
   if (bRetry && !result)
   {
      long long utcTime;
      struct timeval tv;
      struct timezone tz;
      char work[256];
      gettimeofday( &tv, &tz );
      utcTime = UTCMILLIS(tv);
      formatTime(utcTime, false, work);
      printf( "RBI LSA-PSN E: PSN Send Failure DeviceID=%02X%02X%02X%02X%02X%02X, timeSent=%s, messageId=%s, sessionId=%s\n",g_deviceId[0], g_deviceId[1], g_deviceId[2], g_deviceId[3], g_deviceId[4], g_deviceId[5], work, g_preqMessageId, g_sessionId);

   }
   return res;
}

int main( int argc, char** argv )
{
   int argidx= 1;
   char RFC_CALLERID_LSA[] = "LSA";
   char RFC_PARAM_ALTCON_RECEIVER[] = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.LSA.AltconReceiver";
   char RFC_PARAM_SCHEMAS_NGOD[] = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.LSA.Schemas_NGOD";
   char RFC_PARAM_SCHEMA_LOCATION[] = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.LSA.Schema_Location";
   char RFC_PARAM_XMLNS_SCHEMA[] = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.LSA.XMLNS_Schema";
   char RFC_PARAM_XMLSCHEMA_INSTANCE[] = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.LSA.XMLSchema_Instance";
   char RFC_PARAM_SCHEMAS_ADMIN[] = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.LSA.Schemas_admin";
   char RFC_PARAM_SCHEMAS_CORE[] = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.LSA.Schemas_core";
   WDMP_STATUS wdmpStatus;

   memset(&ALTCON_RECEIVER, 0, sizeof(ALTCON_RECEIVER));
   memset(&SCHEMAS_NGOD, 0, sizeof(SCHEMAS_NGOD));
   memset(&SCHEMA_LOCATION, 0, sizeof(SCHEMA_LOCATION));
   memset(&XMLNS_SCHEMA, 0, sizeof(XMLNS_SCHEMA));
   memset(&XMLSCHEMA_INSTANCE, 0, sizeof(XMLSCHEMA_INSTANCE));
   memset(&SCHEMAS_ADMIN, 0, sizeof(SCHEMAS_ADMIN));
   memset(&SCHEMAS_CORE, 0, sizeof(SCHEMAS_CORE));

   wdmpStatus = getRFCParameter(RFC_CALLERID_LSA, RFC_PARAM_ALTCON_RECEIVER, &ALTCON_RECEIVER);
   printf("getRFCParameter for %s[%s]\n", RFC_PARAM_ALTCON_RECEIVER, ((WDMP_SUCCESS == wdmpStatus) ? ALTCON_RECEIVER.value : ""));
   if(wdmpStatus != WDMP_SUCCESS)
      return 0;

   wdmpStatus = getRFCParameter(RFC_CALLERID_LSA, RFC_PARAM_SCHEMAS_NGOD, &SCHEMAS_NGOD);
   printf("getRFCParameter for %s[%s]\n", RFC_PARAM_SCHEMAS_NGOD, ((WDMP_SUCCESS == wdmpStatus) ? SCHEMAS_NGOD.value : ""));
   if(wdmpStatus != WDMP_SUCCESS)
      return 0;

   wdmpStatus = getRFCParameter(RFC_CALLERID_LSA, RFC_PARAM_SCHEMA_LOCATION, &SCHEMA_LOCATION);
   printf("getRFCParameter for %s[%s]\n", RFC_PARAM_SCHEMA_LOCATION, ((WDMP_SUCCESS == wdmpStatus) ? SCHEMA_LOCATION.value : ""));
   if(wdmpStatus != WDMP_SUCCESS)
      return 0;

   wdmpStatus = getRFCParameter(RFC_CALLERID_LSA, RFC_PARAM_XMLNS_SCHEMA, &XMLNS_SCHEMA);
   printf("getRFCParameter for %s[%s]\n", RFC_PARAM_XMLNS_SCHEMA, ((WDMP_SUCCESS == wdmpStatus) ? XMLNS_SCHEMA.value : ""));
   if(wdmpStatus != WDMP_SUCCESS)
     return 0;

   wdmpStatus = getRFCParameter(RFC_CALLERID_LSA, RFC_PARAM_XMLSCHEMA_INSTANCE, &XMLSCHEMA_INSTANCE);
   printf("getRFCParameter for %s[%s]\n", RFC_PARAM_XMLSCHEMA_INSTANCE, ((WDMP_SUCCESS == wdmpStatus) ? XMLSCHEMA_INSTANCE.value : ""));
   if(wdmpStatus != WDMP_SUCCESS)
     return 0;

   wdmpStatus = getRFCParameter(RFC_CALLERID_LSA, RFC_PARAM_SCHEMAS_ADMIN, &SCHEMAS_ADMIN);
   printf("getRFCParameter for %s[%s]\n", RFC_PARAM_SCHEMAS_ADMIN, ((WDMP_SUCCESS == wdmpStatus) ? SCHEMAS_ADMIN.value : ""));
   if(wdmpStatus != WDMP_SUCCESS)
     return 0;

   wdmpStatus = getRFCParameter(RFC_CALLERID_LSA, RFC_PARAM_SCHEMAS_CORE, &SCHEMAS_CORE);
   printf("getRFCParameter for %s[%s]\n", RFC_PARAM_SCHEMAS_CORE, ((WDMP_SUCCESS == wdmpStatus) ? SCHEMAS_CORE.value : ""));
   if(wdmpStatus != WDMP_SUCCESS)
     return 0;

   if ( argc > 1 )
   {
      while( argidx < argc )
      {
         /* using LSA configuration parameters from RFC */
         g_endPointRequest=  argv[argidx++];
         g_endPointStatus=  argv[argidx];
         printf(" rbiDaemon: configuration parameters:\nprURL[%s] \npsnURL[%s]\n", g_endPointRequest, g_endPointStatus);

         ++argidx;

         if ( argidx >= argc ) break;
      }
   }

   printf("rbiDaemon: starting\n");
   setupExit();

   startUp();
   g_ContinueMainLoop= true;
   while( g_ContinueMainLoop )
   {
      sleep(10);
   }

   shutDown();

exit:
   return 0;
}

