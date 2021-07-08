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

#include "rbi.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "rdk_debug.h"
#include "rmf_osal_socket.h"
#include "rmf_osal_sync.h"
#include "rmf_osal_thread.h"
#include "rmf_osal_util.h"
#include "rmf_osal_mem.h"
#if USE_SYSRES_MLT
#include "rpl_malloc.h"
#include "rpl_new.h"
#endif
#include "pthread.h"
#include "libIBus.h"
#include "libIARM.h"
#include "rbirpc.h"
#include "rfcapi.h"
#include "rmfqamsrc.h"

#include <unistd.h>
#include <sys/time.h>

#include <curl/curl.h>

#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <sstream>

//#define USE_DVBSAD
//#define XONE_STB
#define USE_SCTE35

#define RBI_DEFAULT_DEFINITION_REQUEST_TIMEOUT (5000)
#define RBI_DEFAULT_DEFINITION_SPLICE_TIMEOUT (15000)
#define RBI_DEFAULT_DEFINITION_BUFFER_SIZE (1000000)
#define RBI_DEFAULT_PSN_RETRY_COUNT (1)
#define RBI_DEFAULT_ASSET_DURATION_OFFSET (500)

#define UTCMILLIS(tv) (((tv).tv_sec*1000LL)+((tv).tv_usec/1000LL))

#define MAX_PACKETS (512)
#define MAX_PIDS (8)
#define MAX_90K (0x1FFFFFFFFLL)
#define PTS_IN_RANGE( p, s, e )  ((((s)<(e))&&((s)<=(p))&&((p)<(e))) || (((s)>(e))&&(((s)<=(p))||((p)<(e)))))

#define PSI_TABLE_HEADER_SIZE           3 // From table_id to section length.
#define POINTER_FIELD_SIZE              1
#define TS_HEADER_SIZE                  4
#define SCTE35_SECTION_LENGTH           240 //((240*3/4) = 180) (greater than max sectionLength 176+3)
#define INDEX(i) (base+i < m_packetSize-m_ttsSize-TS_HEADER_SIZE) ? (i) : (i+m_ttsSize+TS_HEADER_SIZE)

#define RFC_BUFFER_SIZE                 256

#define ADCACHE_SIZE                    1048576   // 1MB
#define AD_CACHE_BUFFER_SIZE            (16*1024) // 16 K Added similar to CURL_MAX_WRITE_SIZE (the usual default is 16K)
#define ADCACHE_LOCATION                "/opt/LSA_ADCache"
#define ADCACHE_SIZE_MB                 80

#define BYTE_RANGE_SPLIT                    (7 * 47 * 4096) // 1347584 //The rational number that is the least common multiple of 4096 and 1316 which is (188*7).
#define CURLE_COULDNT_CONNECT_RETRY_COUNT   2
#define CURLE_CONNECTION_TIMEOUT            2L

#define HTTPSRC_CLOSE_TIME_OUT (1000)
#define HTTPSRC_FULL_TIME_OUT (10000)

#define I_FRAME (0x1)
#define P_FRAME (0x2)
#define B_FRAME (0x4)

#define FILL_RGB (0x000000)

#define MAX_AUDIO_PID_COUNT (3)
#define MAX_PMT_SECTION_SIZE (1021)
#define MAX_SCTE35_SECTION_SIZE (1021)


#define MAX_TOTAL_READ_RETRY_COUNT 500
#define CURL_CONNECTTIMEOUT 3.5L  /**< Curl socket connection timeout */

#define TS_FORMATERROR_RETRYCOUNT      3

#define VIDEO_FRAME_RATE (59.94)

static char curlErrorCode[CURL_ERROR_SIZE];
static bool test_insert = false;
bool m_isAdCacheEnabled = false;

bool m_isByteRangeEnabled = false;

struct MemoryStruct
{
   MemoryStruct()
      : headerSize(0)
      , headerBuffer(NULL)
   {
      headerBuffer = (char*)malloc(1);
   }

   ~MemoryStruct()
   {
      if (headerBuffer != NULL)
      {
         free(headerBuffer);
         headerBuffer = NULL;
      }
   }

   size_t headerSize;
   char* headerBuffer;
};

char * trim(char *inputString)
{
   unsigned int start=0, length=0;

   // Trim.Start:
   length = strlen(inputString);
   while ((inputString)[start]==' ') start++;
   (inputString) += start;

   if (start < length) // Required for empty (ex. "	") input
   {
      // Trim.End:
      unsigned int end = strlen(inputString)-1; // Get string length again (after Trim.Start)
      while ((inputString)[end]==' ') end--;
      (inputString)[end+1] = 0;
   }
   return inputString;
}

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

static char* formatTime( long long utcTime, char *buffer )
{
   char *timeString= 0;
   time_t tv_sec;
   int tv_usec;
   tv_sec= utcTime/1000LL;
   tv_usec= utcTime%1000LL;
   struct tm tmTime;
   int tzOffset = RBIManager::getInstance()->getTimeZoneMinutesWest();
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
      timeString= buffer;
   }
   return timeString;
}

static void dumpPacket( unsigned char *packet, int packetSize )
{
	if ( rdk_dbg_enabled( "LOG.RDK.TEST", RDK_LOG_TRACE3 ) )
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
      TRACE3( "%s", buff );
	}
}

#ifdef USE_DVB
static void dumpData( unsigned char *data, int len )
{
	if ( rdk_dbg_enabled( "LOG.RDK.TEST", RDK_LOG_TRACE3 ) )
	{
      int i;
      char buff[2048];   
      
      int col= 0;
      int buffPos= 0;
      buffPos += snprintf(&buff[buffPos], sizeof(buff), "\n%04X: ", 0 );
      for( i= 0; i < len; ++i )
      {
          buffPos += snprintf(&buff[buffPos], sizeof(buff) - buffPos,"%02X ", data[i] );
          ++col;
          if ( col == 8 )
          {
             strncat( buff, " ", 1);
             buffPos += 1;
          }
          if ( col == 16 )
          {
             buffPos += snprintf(&buff[buffPos], sizeof(buff) - buffPos, "\n" );
             col= 0;
             if ( sizeof(buff)-buffPos < 100 )
             {
                TRACE3( "%s\n", buff );
                buffPos= 0;
             }
             buffPos += snprintf(&buff[buffPos], sizeof(buff) - buffPos,"%04X: ", i+1 );
          }
      }
      TRACE3( "%s\n", buff );
   }
}
#endif

typedef struct _VLCode
{
   int numBits;
   int value;
} VLCode;

static VLCode expGolombTable7[]=
{
   { -1, -1},   // 0000 000
   { -1, -1},   // 0000 001
   { -1, -1},   // 0000 010
   { -1, -1},   // 0000 011
   { -1, -1},   // 0000 100
   { -1, -1},   // 0000 101
   { -1, -1},   // 0000 110
   { -1, -1},   // 0000 111
   { 7,   7},   // 0001 000
   { 7,   8},   // 0001 001
   { 7,   9},   // 0001 010
   { 7,  10},   // 0001 011
   { 7,  11},   // 0001 100
   { 7,  12},   // 0001 101
   { 7,  13},   // 0001 110
   { 7,  14},   // 0001 111
   { 5,   3},   // 0010 000
   { 5,   3},   // 0010 001
   { 5,   3},   // 0010 010
   { 5,   3},   // 0010 011
   { 5,   4},   // 0010 100
   { 5,   4},   // 0010 101
   { 5,   4},   // 0010 110
   { 5,   4},   // 0010 111
   { 5,   5},   // 0011 000
   { 5,   5},   // 0011 001
   { 5,   5},   // 0011 010
   { 5,   5},   // 0011 011
   { 5,   6},   // 0011 100
   { 5,   6},   // 0011 101
   { 5,   6},   // 0011 110
   { 5,   6},   // 0011 111
   { 3,   1},   // 0100 000
   { 3,   1},   // 0100 001
   { 3,   1},   // 0100 010
   { 3,   1},   // 0100 011
   { 3,   1},   // 0100 100
   { 3,   1},   // 0100 101
   { 3,   1},   // 0100 110
   { 3,   1},   // 0100 111
   { 3,   1},   // 0101 000
   { 3,   1},   // 0101 001
   { 3,   1},   // 0101 010
   { 3,   1},   // 0101 011
   { 3,   1},   // 0101 100
   { 3,   1},   // 0101 101
   { 3,   1},   // 0101 110
   { 3,   1},   // 0101 111
   { 3,   2},   // 0110 000
   { 3,   2},   // 0110 001
   { 3,   2},   // 0110 010
   { 3,   2},   // 0110 011
   { 3,   2},   // 0110 100
   { 3,   2},   // 0110 101
   { 3,   2},   // 0110 110
   { 3,   2},   // 0110 111
   { 3,   2},   // 0111 000
   { 3,   2},   // 0111 001
   { 3,   2},   // 0111 010
   { 3,   2},   // 0111 011
   { 3,   2},   // 0111 100
   { 3,   2},   // 0111 101
   { 3,   2},   // 0111 110
   { 3,   2},   // 0111 111
   { 1,   0},   // 1000 000
   { 1,   0},   // 1000 001
   { 1,   0},   // 1000 010
   { 1,   0},   // 1000 011
   { 1,   0},   // 1000 100
   { 1,   0},   // 1000 101
   { 1,   0},   // 1000 110
   { 1,   0},   // 1000 111
   { 1,   0},   // 1001 000
   { 1,   0},   // 1001 001
   { 1,   0},   // 1001 010
   { 1,   0},   // 1001 011
   { 1,   0},   // 1001 100
   { 1,   0},   // 1001 101
   { 1,   0},   // 1001 110
   { 1,   0},   // 1001 111
   { 1,   0},   // 1010 000
   { 1,   0},   // 1010 001
   { 1,   0},   // 1010 010
   { 1,   0},   // 1010 011
   { 1,   0},   // 1010 100
   { 1,   0},   // 1010 101
   { 1,   0},   // 1010 110
   { 1,   0},   // 1010 111
   { 1,   0},   // 1011 000
   { 1,   0},   // 1011 001
   { 1,   0},   // 1011 010
   { 1,   0},   // 1011 011
   { 1,   0},   // 1011 100
   { 1,   0},   // 1011 101
   { 1,   0},   // 1011 110
   { 1,   0},   // 1011 111
   { 1,   0},   // 1100 000
   { 1,   0},   // 1100 001
   { 1,   0},   // 1100 010
   { 1,   0},   // 1100 011
   { 1,   0},   // 1100 100
   { 1,   0},   // 1100 101
   { 1,   0},   // 1100 110
   { 1,   0},   // 1100 111
   { 1,   0},   // 1101 000
   { 1,   0},   // 1101 001
   { 1,   0},   // 1101 010
   { 1,   0},   // 1101 011
   { 1,   0},   // 1101 100
   { 1,   0},   // 1101 101
   { 1,   0},   // 1101 110
   { 1,   0},   // 1101 111
   { 1,   0},   // 1110 000
   { 1,   0},   // 1110 001
   { 1,   0},   // 1110 010
   { 1,   0},   // 1110 011
   { 1,   0},   // 1110 100
   { 1,   0},   // 1110 101
   { 1,   0},   // 1110 110
   { 1,   0},   // 1110 111
   { 1,   0},   // 1111 000
   { 1,   0},   // 1111 001
   { 1,   0},   // 1111 010
   { 1,   0},   // 1111 011
   { 1,   0},   // 1111 100
   { 1,   0},   // 1111 101
   { 1,   0},   // 1111 110
   { 1,   0},   // 1111 111
};

static long long getCurrentTimeMicro()
{
   struct timeval tv;
   long long utcCurrentTimeMicro;

   gettimeofday(&tv,0);
   utcCurrentTimeMicro= tv.tv_sec*1000000LL+tv.tv_usec;

   return utcCurrentTimeMicro;
}

static void onPreDownloadComplete(rtFileDownloadRequest* fileDownloadRequest)
{
   if (fileDownloadRequest != NULL)
   {
      if (fileDownloadRequest->downloadStatusCode() == 0 &&
         ((fileDownloadRequest->httpStatusCode() == 200) || (fileDownloadRequest->isByteRangeEnabled() && fileDownloadRequest->httpStatusCode() == 206)))
      {
         if(fileDownloadRequest->isDataCached())
            INFO("onPreDownloadComplete file was already in cache for url %s\n", fileDownloadRequest->fileUrl().cString() );
         else
            INFO("onPreDownloadComplete file is downloaded from network for url %s\n", fileDownloadRequest->fileUrl().cString() );
      }
      else
      {
         ERROR("onPreDownloadComplete Resource Download Failed: %s Error: %s HTTP Status Code: %ld",
            fileDownloadRequest->fileUrl().cString(),
            fileDownloadRequest->errorString().cString(),
            fileDownloadRequest->httpStatusCode());
      }
   }
}

static IARM_Result_t rbiDefineSpotsHandler( void *data )
{
   IARM_Result_t result= IARM_RESULT_OOM;
   RBIInsertDefinitionSet *dfnSet= 0;
   long long utcTrigger, utcSplice, splicePTS;
   int sessionNumber;
   char sessionId[64];
   
   RBI_DefineSpots_Args *args= (RBI_DefineSpots_Args*)data;
   
   uuid_unparse( args->sessionId, sessionId );
   
   RBIManager *rbm= RBIManager::getInstance();
   if ( rbm )
   {
      sessionNumber= rbm->getSessionNumber(sessionId);

      INFO( "RBI RPC API DefineSpots called: sessionId %d,%s triggerId %X numSpots %d", sessionNumber, sessionId, args->triggerId, args->numSpots );
      
      if ( sessionNumber > 0 )
      {

         if ( args->numSpots > 0 )
         {
            if ( rbm->findInsertOpportunity( sessionId, utcTrigger, utcSplice, splicePTS ) )
            {
               struct timeval tv;
               long long utcCurrentTime;
            
               gettimeofday(&tv,0);
               utcCurrentTime= UTCMILLIS(tv);
            
               // Verify that the opportunty has not expired
               if ( utcCurrentTime > utcTrigger+rbm->getTriggerDefinitionTimeout() )
               {
                  ERROR( "RBI: Pending Opportunity Expired" );
               }
               else
               {
                  // Update splice time from args if it has been delayed
                  if ( args->utcTimeStart > utcSplice )
                  {
                     INFO("DefineSpots: session %d,%s advance break start time from %lld to %lld", sessionNumber, sessionId, utcSplice, args->utcTimeStart );
                     utcSplice= args->utcTimeStart;
                  }
                  dfnSet= new RBIInsertDefinitionSet( utcTrigger, utcSplice, splicePTS );
                  if ( dfnSet )
                  {
                     bool error= false;
                     for ( int i= 0; i < args->numSpots; ++i )
                     {
                        bool isReplace= (args->spots[i].action == RBI_SpotAction_replace);
                        const char *assetID;
                        const char *providerID;
                        const char *uri;
                        const char *trackingIdString;

                        trackingIdString= args->spots[i].trackingIdString;
                        assetID= args->spots[i].assetID;
                        providerID= args->spots[i].providerID;
                        uri= (const char*)args->spots[i].uriData;

                        INFO("DefineSpots: session %d,%s triggerId %X replace %d duration %d assetID (%s) providerID (%s) uri (%s)",
                        sessionNumber, sessionId, args->triggerId, isReplace, args->spots[i].duration, assetID, providerID, uri );
                        RBIInsertDefinition *dfn= new RBIInsertDefinition( args->triggerId,
                                                                           isReplace,
                                                                           args->spots[i].duration,
                                                                           assetID,
                                                                           providerID,
                                                                           uri,
                                                                           trackingIdString,
                                                                           args->spots[i].validSpot,
                                                                           args->spots[i].detailCode);

                        if(m_isByteRangeEnabled)
                        {
                           /* Predownload from spot-1 to spot-n, exclude spot-0 due to insufficient prefetch window time */
                           if(m_isAdCacheEnabled && isReplace && (i > 0))
                           {
                              rtString mUrl(uri);
                              rtFileDownloadRequest *downloadRequest = new rtFileDownloadRequest(mUrl, NULL);

                              if(downloadRequest)
                              {
                                 std::string byteRange("NULL");
                                 RBIManager *rbm= RBIManager::getInstance();

                                 INFO("addToDownloadQueue for spot %d url %s\n", i, downloadRequest->fileUrl().cString() );
                                 downloadRequest->setCurlDefaultTimeout(true);

                                 if(rbm)
                                    downloadRequest->setUserAgent(rbm->curlUserAgent());
                                 downloadRequest->setKeepTCPAlive(false);
                                 downloadRequest->setCROSRequired(false);
                                 downloadRequest->enableDownloadMetrics(false);
                                 downloadRequest->setRedirectFollowLocation(false);
                                 downloadRequest->setDownloadOnly(true);
                                 downloadRequest->setByteRangeEnable(true);
                                 downloadRequest->setByteRangeIntervals(BYTE_RANGE_SPLIT);
                                 downloadRequest->setCurlRetryEnable(true);
                                 downloadRequest->setConnectionTimeout(2L);
                                 downloadRequest->setCurlErrRetryCount(CURLE_COULDNT_CONNECT_RETRY_COUNT);
                                 downloadRequest->setCallbackFunction(onPreDownloadComplete);
                                 rtFileDownloader::instance()->addToDownloadQueue(downloadRequest);
                              }
                           }
                        }

                        if ( dfn )
                        {
                           dfnSet->addSpot( *dfn );
                        }
                        else
                        {
                           error= true;
                           break;
                        }
                     }

                     if ( !error )
                     {
                        int rc= rbm->registerInsertDefinitionSet( sessionId, *dfnSet );
                        if ( RBIResult_ok == rc )
                        {
                           result= IARM_RESULT_SUCCESS;
                        }
                        else
                        {
                           ERROR( "Error %d registering session %s,%d definition set", sessionNumber, sessionId, rc );
                        }
                     }
                  }
               }
            }
         }
         else
         {
            rbm->unregisterInsertOpportunity( sessionId );
         }
      }
      else
      {
         ERROR("Invalid sessionNumber - Unregister opportunity sessionId %s sessionNumber =%d\n", sessionId, sessionNumber);
         rbm->unregisterInsertOpportunity( sessionId );
      }
   }
   
   return result;
}

static IARM_Result_t rbiMonitorPrivateDataHandler( void *data )
{
   IARM_Result_t result= IARM_RESULT_OOM;
   RBI_MonitorPrivateData_Args *mon= (RBI_MonitorPrivateData_Args*)data;
   
   RBIPrivateDataMonitor monNew;

   memset( &monNew, 0, sizeof(RBIPrivateDataMonitor) );
   
   monNew.numStreams= mon->numStreams;
   TRACE3("New private data monitor definitions: numStreams %d", monNew.numStreams );
   for( int i= 0; i < monNew.numStreams; ++i )
   {
      monNew.specs[i].streamType= mon->specs[i].streamType;
      monNew.specs[i].matchTag= mon->specs[i].matchTag;
      monNew.specs[i].descrTag= mon->specs[i].descrTag;
      monNew.specs[i].descrDataLen= mon->specs[i].descrDataLen;
      if ( monNew.specs[i].descrDataLen > 32 )
      {
         monNew.specs[i].descrDataLen= 32;
      }
      TRACE3(" private data monitor definitions: spec %d: streamType %02X matchTag %d descrTag %02X descrDataLen %d",
           i, 
           monNew.specs[i].streamType,
           monNew.specs[i].matchTag,
           monNew.specs[i].descrTag,
           monNew.specs[i].descrDataLen );   
      if ( monNew.specs[i].descrDataLen > 0 )
      {         
         memcpy( &monNew.specs[i].descrData, &mon->specs[i].descrData, monNew.specs[i].descrDataLen );
         TRACE3(" private data monitor definitions: spec %d: descrData (0-8): %02X %02X %02X %02X %02X %02X %02X %02X ",
              i,
              monNew.specs[i].descrData[0],
              monNew.specs[i].descrData[1],
              monNew.specs[i].descrData[2],
              monNew.specs[i].descrData[3],
              monNew.specs[i].descrData[4],
              monNew.specs[i].descrData[5],
              monNew.specs[i].descrData[6],
              monNew.specs[i].descrData[7] );
      }
   }
   
   RBIManager::getInstance()->setPrivateDataMonitor( monNew );   
   
   return result;
}

typedef struct _RBIBitRateInfo
{
   int count;
   int bitRate;
} RBIBitRateInfo;

class RBIHTTPInsertSource : public RBIInsertSource, public RBIStreamProcessorEvents, public rtObject
{
   public:
      RBIHTTPInsertSource( int id, std::string uri, long long splicePTS );
      virtual ~RBIHTTPInsertSource();

      virtual long long splicePTS();
      virtual bool dataReady();
      virtual long long startPTS();      
      virtual long long startPCR();
      virtual int bitRate();
      virtual int avail();
      virtual int read( unsigned char *buffer, int length );
      virtual unsigned int getTotalRetryCount()
      {
         return m_totalRetryCount;
      }
      virtual RBIStreamProcessor& getStreamProcessor()
      {
         return m_streamProcessor;
      }
      virtual std::vector<int> getAdVideoExitFrameInfo()
      {
        return m_adVideoExitFrameInfo;
      }

      void acquiredPMT( RBIStreamProcessor *sp );
      void foundFrame( RBIStreamProcessor *sp, int frameType, long long frameStartOffset, long long pts, long long pcr );
      void foundBitRate( RBIStreamProcessor *sp, int bitRate, int chunkSize );
      void insertionStatusUpdate( RBIStreamProcessor *sp, int detailCode );
      bool getTriggerDetectedForOpportunity(void);  
      int getNextAdPacketPid();
  
   protected:
      static void onDownloadComplete(rtFileDownloadRequest* downloadRequest);

   private:
      virtual bool open();
      virtual void close();
      
      static void threadEntry( void *arg );
      static size_t dataReceived(void *ptr, size_t size, size_t nmemb, void *userData);
      static size_t HeaderCallback(void *ptr, size_t size, size_t nmemb, void *userData);
      static int progressCallback(void* ptr, double dltotal, double dlnow, double ultotal, double ulnow);

      void run();
      int receiveData( unsigned char *data, int dataLen );
      void updateBitrate( int consumed );

      std::string m_uri;
      long long m_splicePTS;
      RBIStreamProcessor m_streamProcessor;      
      pthread_mutex_t m_mutex;
      rmf_osal_Cond m_notFullCond;
      rmf_osal_Cond m_stopCond;
      bool m_threadCreated;
      bool m_threadRunning;
      bool m_threadStopRequested;
      rmf_osal_ThreadId m_threadId;
      int m_haveTSFormatError; // Reason for int type is combining error happened and retry count. If zeor, no error. If one, error happend first time. If two, error happened consecutive
      
      bool m_havePMT;
      bool m_haveIFrame;
      long long m_offsetIFrame;
      int m_lenIFrame;
      std::vector<RBIBitRateInfo> m_bitRateInfo;
      int m_bitRate;
      
      long long m_startPTS;
      long long m_startPCR;
      
      // write at head, read at tail
      int m_head;
      int m_tail;
      int m_capacity;
      int m_count;
      int m_adReadVideoFrameCnt;
      unsigned char* m_data;

      unsigned int m_totalRetryCount;

      std::vector<int> m_adVideoExitFrameInfo;

      friend class RBIInsertDefinition;
      friend class RBIManager;    
};

RBIHTTPInsertSource::RBIHTTPInsertSource( int id, std::string uri, long long splicePTS )
  : RBIInsertSource(id), m_uri(uri), m_splicePTS(splicePTS), m_streamProcessor(),
     m_threadCreated(false), m_threadRunning(false), m_threadStopRequested(false), m_haveTSFormatError(0),
     m_havePMT(false), m_haveIFrame(false), m_offsetIFrame(0LL), m_lenIFrame(0), 
     m_bitRate(0), m_startPTS(-1LL),
     m_head(0), m_tail(0), m_capacity(0), m_count(0), m_data(0), m_totalRetryCount(0), m_adReadVideoFrameCnt(0)
{
   pthread_mutex_init( &m_mutex, 0 );
   m_streamProcessor.setTriggerEnable( false );
   m_streamProcessor.setFrameDetectEnable( true );
   m_streamProcessor.setTrackContinuityCountersEnable( false );
   m_streamProcessor.setEvents( this );
}

RBIHTTPInsertSource::~RBIHTTPInsertSource()
{
   pthread_mutex_destroy( &m_mutex );
   if ( m_data )
   {
      free( m_data );
   }
}

void RBIHTTPInsertSource::threadEntry( void *arg )
{
   ((RBIHTTPInsertSource*)arg)->run();
}

size_t RBIHTTPInsertSource::dataReceived(void *ptr, size_t size, size_t nmemb, void *userData)
{
   return ((RBIHTTPInsertSource*)userData)->receiveData( (unsigned char*)ptr, size*nmemb );
}

int RBIHTTPInsertSource::progressCallback(void* ptr, double dltotal, double dlnow, double ultotal, double ulnow)
{
   int rc= 0;
   RBIHTTPInsertSource *src;

   src= (RBIHTTPInsertSource*)ptr;

   if (src->m_threadStopRequested)
   {
      INFO("RBIHTTPInsertSource::progressCallback m_threadStopRequested requested");
      /* Trigger end of session */
      rc = -1;
   }
   return rc;
}

bool RBIHTTPInsertSource::open()
{
   bool result= false;
	rmf_Error ret;

   m_data= (unsigned char *)malloc( m_maxBuffering );
   if ( m_data )
   {
      m_head= 0;
      m_tail= 0;
      m_capacity= m_maxBuffering;
      m_count= 0;
      m_totalRetryCount= 0;

      ret= rmf_osal_condNew( TRUE, FALSE, &m_notFullCond);
      if ( ret == RMF_SUCCESS )
      {
         ret= rmf_osal_condNew( TRUE, FALSE, &m_stopCond);
         if ( ret == RMF_SUCCESS )
         {
	         ret= rmf_osal_threadCreate( threadEntry, 
	                                     this,
	                                     RMF_OSAL_THREAD_PRIOR_DFLT, 
	                                     RMF_OSAL_THREAD_STACK_SIZE,
                                        &m_threadId,
                                        "RBIHTTPInsertSourceThread" );
            if ( ret == RMF_SUCCESS )
            {
               m_threadCreated= true;
               result= true;
            }
            else
            {
               ERROR("RBIHTTPInsertSource::open unable to start thread");
            }
         }
         else
         {
            ERROR("RBIHTTPInsertSource::open rmf_osal_condNew failed");
         }
      }
   }
   else
   {
      ERROR("RBIHTTPInsertSource::open unable to allocate buffer size %d", m_maxBuffering );
   }

      
   return result;
}

void RBIHTTPInsertSource::close()
{
   INFO( "RBIHTTPInsertSource::close: entry for m_uri(%s)", m_uri.c_str());
   m_threadStopRequested= true;

   // Wakeup receive thread so it sees stop request
   rmf_osal_condSet(m_notFullCond);
   INFO( "RBIHTTPInsertSource::close: m_notFullCond waiting for src thread to stop... for m_uri(%s)", m_uri.c_str());
   rmf_osal_condWaitFor( m_stopCond, HTTPSRC_CLOSE_TIME_OUT);
   INFO( "RBIHTTPInsertSource::close: m_stopCond src thread stopped for m_uri(%s)", m_uri.c_str());

   if ( m_data )
   {
      free( m_data );
      m_data= 0;
      m_head= m_tail= m_capacity= m_count= m_adReadVideoFrameCnt = 0;
      m_totalRetryCount= 0;
   }

   rmf_osal_condDelete( m_stopCond );
   m_stopCond = NULL;
   INFO( "RBIHTTPInsertSource::close: m_stopCond is rmf_osal_condDelete for m_uri(%s)", m_uri.c_str());
   rmf_osal_condDelete( m_notFullCond );
   m_notFullCond = NULL;
   INFO( "RBIHTTPInsertSource::close: m_notFullCond is rmf_osal_condDelete for m_uri(%s)", m_uri.c_str());
}

long long RBIHTTPInsertSource::splicePTS()
{
   return m_splicePTS;
}

bool RBIHTTPInsertSource::dataReady()
{
   bool ready= false;

   pthread_mutex_lock( &m_mutex );
   if ( m_havePMT && m_haveIFrame && (m_bitRate != 0) && (m_count > 100*1024) )
   {
      ready= true;
   }
   pthread_mutex_unlock( &m_mutex );

   return ready;
}

long long RBIHTTPInsertSource::startPTS()
{
   return m_startPTS;
}

long long RBIHTTPInsertSource::startPCR()
{
   return m_startPCR;
}

int RBIHTTPInsertSource::bitRate()
{
   int bitRate;
   pthread_mutex_lock( &m_mutex );   
   bitRate= m_bitRate;
   pthread_mutex_unlock( &m_mutex );   
   return bitRate;
}

int RBIHTTPInsertSource::avail()
{
   int avail= 0;

   {
      pthread_mutex_lock( &m_mutex );

      avail= m_count;
      
      pthread_mutex_unlock( &m_mutex );
   }
   
   return avail;
}

int RBIHTTPInsertSource::read( unsigned char *buffer, int length )
{
   int lenDidRead= 0;
   int retryCount= 10;

   if ( length )
   {
      int offset= 0;
      bool wasFull= false;
         
      pthread_mutex_lock( &m_mutex );
      
      if ( m_count+1 >= m_capacity )
      {
         wasFull= true;
      }

      while( retryCount > 0 )
      {
         if ( m_count >= length )
         {
            int consume= length-offset;
            
            while( consume )
            {
               int copylen= (m_tail < m_head) ? m_head-m_tail : m_capacity-m_tail;
               if ( copylen > consume )
               {
                  copylen= consume;
               }
               if ( copylen > 0 )
               {
                  if ( buffer )
                  {         
                     memcpy( &buffer[offset], &m_data[m_tail], copylen );
                  }
                  offset += copylen;      
                  consume -= copylen;
                  assert( consume >= 0);
                  m_tail += copylen;
                  m_count -= copylen;
                  if ( m_tail >= m_capacity ) m_tail= 0;
               }
               else
               {
                  break;
               }
            }
            
            // Done reading
            break;
         }
         else if ( --retryCount > 0 )
         {
            if ( m_threadRunning  )
            {
               if ( ++m_totalRetryCount >= MAX_TOTAL_READ_RETRY_COUNT )
               {
                   INFO("RBIHTTPInsertSource::read: Abnormal termination as server is not sending data fast enough");
                   break;
               }

               INFO("RBIHTTPInsertSource::read: no data: m_head %d m_tail %d m_count %d m_capacity %d length %d retriesLeft %d m_totalRetryCount %d",
                    m_head, m_tail, m_count, m_capacity, length, retryCount, m_totalRetryCount );
               pthread_mutex_unlock( &m_mutex );
               usleep( 10000 );
               pthread_mutex_lock( &m_mutex );
            }
            else
            {
               break;
            }
         }
      }

      int consumed= offset;
      updateBitrate( consumed );
      
      pthread_mutex_unlock( &m_mutex );
      
      lenDidRead= offset;
      
      if ( wasFull && lenDidRead )
      {
         rmf_osal_condSet(m_notFullCond);
      }
   }
   
   return lenDidRead;
}

void RBIHTTPInsertSource::updateBitrate( int consumed )
{  
   // Must call while mutex is owned
   while( consumed > 0 )
   {
      if ( m_bitRateInfo.size() > 0 )
      {
         if ( consumed >= m_bitRateInfo[0].count )
         {
            consumed -= m_bitRateInfo[0].count;
            m_bitRateInfo.erase( m_bitRateInfo.begin());
         }
         else
         {
            m_bitRateInfo[0].count -= consumed;
            consumed= 0;
         }
      }     
      else
      {
         break;
      }
   }      
   if ( m_bitRateInfo.size() > 0 )
   {
      m_bitRate= m_bitRateInfo[0].bitRate;
   }
}

// Static callback that gets called when fileDownloadRequest completes
void RBIHTTPInsertSource::onDownloadComplete(rtFileDownloadRequest* fileDownloadRequest)
{
    if (fileDownloadRequest != NULL && fileDownloadRequest->callbackData() != NULL)
    {
        RBIHTTPInsertSource* resp = (RBIHTTPInsertSource*)fileDownloadRequest->callbackData();

        if(!resp->m_threadStopRequested)
        {
            if (fileDownloadRequest->downloadStatusCode() == 0 &&
                ((fileDownloadRequest->httpStatusCode() == 200) || (fileDownloadRequest->isByteRangeEnabled() && fileDownloadRequest->httpStatusCode() == 206)) &&
                fileDownloadRequest->downloadedData() != NULL)
            {
               INFO("isDataCached %d for url %s\n", fileDownloadRequest->isDataCached(),fileDownloadRequest->fileUrl().cString() );
               INFO("RBI LSA-P: Ad Cache enabled, content from %s %s dataSize %d",
                   (fileDownloadRequest->isDataCached() == TRUE ? "Cache"  : "CDN"), fileDownloadRequest->fileUrl().cString(), fileDownloadRequest->downloadedDataSize());
            }
            else
            {
                ERROR("Resource Download Failed: %s Error: %s HTTP Status Code: %ld",
                    fileDownloadRequest->fileUrl().cString(),
                    fileDownloadRequest->errorString().cString(),
                    fileDownloadRequest->httpStatusCode());
            }

            CURLcode curl_code = (CURLcode)fileDownloadRequest->downloadStatusCode();

            INFO("RBIHTTPInsertSource::onDownloadComplete curl_code %d resp->m_haveTSFormatError %d", curl_code, resp->m_haveTSFormatError);

            if (resp->m_haveTSFormatError >= TS_FORMATERROR_RETRYCOUNT)
            {
                if ( resp->m_sourceEvents )
                   resp->m_sourceEvents->tsFormatError( resp );
            }
            else if (curl_code)
            {
               if ( !resp->m_threadStopRequested )
               {
                    char errorBuffer[CURL_ERROR_SIZE];

                    memset(errorBuffer, 0, sizeof(errorBuffer));
                    strncpy(errorBuffer, fileDownloadRequest->httpErrorBuffer(), CURL_ERROR_SIZE);
                    errorBuffer[CURL_ERROR_SIZE-1] ='\0';

                    if ( strlen(errorBuffer) > 0 )
                    {
                       strncpy(curlErrorCode,errorBuffer, CURL_ERROR_SIZE);
                       curlErrorCode[CURL_ERROR_SIZE-1] ='\0';
                       ERROR("RBIHTTPInsertSource::run: curlErrorCode: %s\n", curlErrorCode );
                    }
                    if ( resp->m_sourceEvents )
                    {
                        if (curl_code == CURLE_OPERATION_TIMEDOUT)
                            resp->m_sourceEvents->httpTimeoutError( resp );
                        else
                            resp->m_sourceEvents->sourceError( resp );
                    }
                }
            }
            INFO("RBIHTTPInsertSource::onDownloadComplete is about to end for (%s)", fileDownloadRequest->fileUrl().cString());
        }
        else
            INFO("m_threadStopRequested is TRUE for (%s)", fileDownloadRequest->fileUrl().cString());

        INFO("RBIHTTPInsertSource::onDownloadComplete going to set rmf_osal_condSet for (%s)", fileDownloadRequest->fileUrl().cString());
        rmf_osal_condSet(resp->m_stopCond);
        INFO("RBIHTTPInsertSource::onDownloadComplete set done rmf_osal_condSet for (%s)", fileDownloadRequest->fileUrl().cString());

        resp->m_threadRunning= false;
    }
    else
       ERROR("fileDownloadRequest or its callback is NULL");

    INFO("RBIHTTPInsertSource::onDownloadComplete done");
}

size_t RBIHTTPInsertSource::HeaderCallback(void *ptr, size_t size, size_t nmemb, void *userData)
{
   size_t downloadSize = size * nmemb;
   struct MemoryStruct *mem = (struct MemoryStruct *)userData;

   mem->headerBuffer = (char*)realloc(mem->headerBuffer, mem->headerSize + downloadSize + 1);
   if(mem->headerBuffer == NULL) {
      /* out of memory! */
      INFO("out of memory when downloading image");
      return 0;
   }

   memcpy(&(mem->headerBuffer[mem->headerSize]), ptr, downloadSize);
   mem->headerSize += downloadSize;
   mem->headerBuffer[mem->headerSize] = 0;

   return downloadSize;
}

void RBIHTTPInsertSource::run()
{
   m_threadRunning= true;
   RBIManager *rbm= RBIManager::getInstance();
   INFO("CURL_CONNECTTIMEOUT %ld", CURL_CONNECTTIMEOUT);

    if(m_isAdCacheEnabled)
    {
       rtString mUrl(m_uri.c_str());
       rtFileDownloadRequest *downloadRequest = new rtFileDownloadRequest(mUrl , this);

       if(downloadRequest)
       {
         downloadRequest->setCallbackFunction(RBIHTTPInsertSource::onDownloadComplete);
         downloadRequest->setExternalWriteCallback(RBIHTTPInsertSource::dataReceived, this);
         downloadRequest->setProgressCallback(RBIHTTPInsertSource::progressCallback, this);
         downloadRequest->setHTTPFailOnError(true);
         downloadRequest->setCurlDefaultTimeout(true);
         downloadRequest->setDeferCacheRead(true);
         downloadRequest->setCallbackData(this);
         downloadRequest->setCachedFileReadSize(AD_CACHE_BUFFER_SIZE);
         downloadRequest->setUseCallbackDataSize(true);
         if(rbm)
            downloadRequest->setUserAgent(rbm->curlUserAgent());
         if(m_isByteRangeEnabled)
         {
            downloadRequest->setKeepTCPAlive(false);
            downloadRequest->setCROSRequired(false);
            downloadRequest->enableDownloadMetrics(false);
            downloadRequest->setRedirectFollowLocation(false);
            downloadRequest->setByteRangeEnable(true);
            downloadRequest->setByteRangeIntervals(BYTE_RANGE_SPLIT);
            downloadRequest->setCurlRetryEnable(true);
            downloadRequest->setConnectionTimeout(CURL_CONNECTTIMEOUT);
            downloadRequest->setCurlErrRetryCount(CURLE_COULDNT_CONNECT_RETRY_COUNT);
            rtFileDownloader::instance()->downloadFileAsByteRange(downloadRequest);
            INFO( "RBIHTTPInsertSource::run: rtFileDownloader::downloadFileAsByteRange completed for (%s)", mUrl.cString());
         }
         else
         {
            rtFileDownloader::instance()->downloadFile(downloadRequest);
            INFO( "RBIHTTPInsertSource::run: rtFileDownloader::downloadFile completed for (%s)", mUrl.cString());
         }
      }
      else
         ERROR("RBIHTTPInsertSource::run rtFileDownloadRequest is NULL for (%s)", mUrl.cString());
   }
   else
   {
      CURL *curl= curl_easy_init();
      if ( curl )
      {
         CURLcode curl_code;
         size_t fileSize = 0;
         const char *pCurlUrl = m_uri.c_str();
         char errorBuffer[CURL_ERROR_SIZE] = "";
         errorBuffer[0]= '\0';
         double totalTimeTaken = 0;

         INFO("RBI LSA-P: Ad Cache not enabled, content from CDN %s", m_uri.c_str() );
         curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
         curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
         curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
         curl_easy_setopt(curl, CURLOPT_URL, pCurlUrl);
         curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dataReceived );
         curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
         curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progressCallback);
         curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, this);
         curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);

         if(rbm)
            curl_easy_setopt(curl, CURLOPT_USERAGENT, rbm->curlUserAgent());

         if(m_isByteRangeEnabled)
         {
            int multipleIntervals = 1;
            size_t startRange = 0;
            std::string byteRange("NULL");
            struct MemoryStruct chunk;
            std::string strLocation;
            unsigned int curlErrRetryCount = 0;
            bool reDirect = false;
            std::string newUrl;

            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0);
            curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
            curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)&chunk);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, CURL_CONNECTTIMEOUT);

            /* pxCore Copyright 2005-2018 John Robinson
               Licensed under the Apache License, Version 2.0 */
            for(int iLoop=0; iLoop<= multipleIntervals; /* Increment handled inside the body. *iamhere */)
            {
               if(curlErrRetryCount < CURLE_COULDNT_CONNECT_RETRY_COUNT )
               {
                  if(iLoop < multipleIntervals)
                  {
                     // Request for (0  to 8k) bytes to get http header, once it got success request for (8k+1 to BYTE_RANGE_SPLIT-1)bytes.
                     // Then there onwards request for BYTE_RANGE_SPLIT bytes until the end.
                     if(iLoop == 0)
                     {
                        byteRange = std::to_string(startRange) + "-" + std::to_string(8192-1);
                     }
                     else if(iLoop == 1)
                     {
                        startRange = 0;
                        byteRange = std::to_string(8192) + "-" + std::to_string(startRange + BYTE_RANGE_SPLIT-1);
                     }
                     else
                     {
                        byteRange = std::to_string(startRange) + "-" + std::to_string(startRange + BYTE_RANGE_SPLIT-1);
                     }
                  }
                  else if(iLoop == multipleIntervals)
                  {
                     if(fileSize % BYTE_RANGE_SPLIT)
                        byteRange = std::to_string(startRange) + "-" + std::to_string(fileSize-1);
                  }
                  INFO("byteRange request for (%s) Url (%s)", byteRange.c_str(), pCurlUrl);
                  curl_easy_setopt(curl, CURLOPT_RANGE, byteRange.c_str());
               }

               curl_code = curl_easy_perform(curl);
               if(curl_code != CURLE_OK)
               {
                  if(curl_code == CURLE_COULDNT_CONNECT)
                  {
                     // If there is above metioned error even after retry, quit from curl download.
                     if(curlErrRetryCount == CURLE_COULDNT_CONNECT_RETRY_COUNT)
                     {
                        INFO("After retry retryCount(%d). Error(%d) occured during curl download for byteRange(%s) Url (%s)", curlErrRetryCount, curl_code, byteRange.c_str(), pCurlUrl);
                        break;
                     }

                     // If there is above mentioned error, retry one more time. Don't increment, execute the loop for the same index.
                     curlErrRetryCount++;
                     INFO("Retry again retryCount(%d). Error(%d) occured during curl download for byteRange(%s) Url (%s)", curlErrRetryCount, curl_code, byteRange.c_str(), pCurlUrl);
                     continue;
                  }
                  else
                  {
                     INFO("No retry. Error(%d) occured during curl download for byteRange[%s] Url (%s)", curl_code, byteRange.c_str(), pCurlUrl);
                     break;
                  }
               }
               else
               {
                  if(CURLE_OK == curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &totalTimeTaken))
                  {
                     INFO("Time taken to complete curl_easy_perform is %.1f sec for byterange (%s) url (%s)\n", totalTimeTaken, byteRange.c_str(), pCurlUrl );
                  }
               }

               if((iLoop == 0) && (curl_code == CURLE_OK))
               {
                  size_t responseCode = 0;
                  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode );
                  INFO("responseCode %d\n", responseCode);
                  if(responseCode == 302)
                  {
                     char *reDirUrl = NULL;
                     curl_easy_getinfo(curl, CURLINFO_REDIRECT_URL, &reDirUrl );

                     if(reDirUrl)
                     {
                        newUrl = std::string(reDirUrl);
                        pCurlUrl = newUrl.c_str();
                        INFO("302 Found. Redirected curl URL [%s]", pCurlUrl);

                        if (chunk.headerBuffer != NULL)
                        {
                           free(chunk.headerBuffer);
                           chunk.headerBuffer = NULL;
                        }
                        // More than one redirection happened, CDN agreed not to have multiple redirection. Due to limited time, break the loop and not continue the download/insertion.
                        if(reDirect)
                        {
                           curl_code = CURLE_TOO_MANY_REDIRECTS;
                           strcpy(errorBuffer, "CURLE_TOO_MANY_REDIRECTS");
                           ERROR("LSA CURLE_TOO_MANY_REDIRECTS. More than one redirect, so forcing not to proceed.");
                           break;
                        }

                        curl_easy_cleanup(curl);
                        if (chunk.headerBuffer == NULL)
                        {
                           chunk.headerBuffer = (char*)malloc(1);
                           chunk.headerSize = 0;
                        }
                        curl= curl_easy_init();
                        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
                        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
                        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
                        curl_easy_setopt(curl, CURLOPT_URL, pCurlUrl);
                        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0);
                        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
                        curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)&chunk);
                        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dataReceived );
                        curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
                        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, CURL_CONNECTTIMEOUT);
                        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progressCallback);
                        curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, this);
                        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);

                        if(rbm)
                           curl_easy_setopt(curl, CURLOPT_USERAGENT, rbm->curlUserAgent());

                        startRange = 0;
                        reDirect = true;
                        continue;
                     }
                  }

                  std::string httpHeaderStr(reinterpret_cast< char const* >((unsigned char *)chunk.headerBuffer));

                  // Parsing total filesize from Content-Range.
                  size_t findContentRange = httpHeaderStr.find("Content-Range: bytes");
                  if(findContentRange != std::string::npos)
                  {
                     std::string strContentRange = httpHeaderStr.substr(findContentRange+1);
                     size_t findRange = strContentRange.find("/");

                     if(findRange != std::string::npos)
                     {
                        std::string substrRange = strContentRange.substr(findRange + 1);
                        std::string strRange = substrRange.substr(0, substrRange.find_first_of("\n\t")-1);
                        fileSize = atoi(strRange.c_str());
                        INFO("FileSize [%ld] from http header(Content-Range). Url[%s]", fileSize, m_uri.c_str());
                        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
                     }
                     else
                        INFO("Http header Content-Range is not in xxx/yyy format. Url[%s]", m_uri.c_str());
                  }
                  else
                     INFO("Http header doesn't have Content-Range. Url[%s]", m_uri.c_str());

                  if(fileSize == 0)
                  {
                     curl_code = CURLE_RANGE_ERROR;
                     strcpy(errorBuffer, "CURLE_RANGE_ERROR");
                     ERROR("LSA CURLE_RANGE_ERROR. Failed to obtain fileSize from http response header.");
                     break;
                  }
                  multipleIntervals = (fileSize >= BYTE_RANGE_SPLIT) ? (fileSize / BYTE_RANGE_SPLIT) : 0;
                  multipleIntervals++;
                  INFO("byteRange fileSize[%ld] multipleIntervals [%d]", fileSize, multipleIntervals);
               }
               startRange += BYTE_RANGE_SPLIT;

               curlErrRetryCount = 0;
               iLoop++; // *iamhere
            }
         }
         else
         {
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
            curl_code = curl_easy_perform(curl);
         }

         if (m_haveTSFormatError >= TS_FORMATERROR_RETRYCOUNT)
         {
           if ( m_sourceEvents )
           {
              m_sourceEvents->tsFormatError( this );
           }
         }
         else if (curl_code)
         {
            if ( !m_threadStopRequested )
            {
               ERROR("RBIHTTPInsertSource::run curl session error %d", curl_code);
               if ( strlen(errorBuffer) > 0 )
               {
                  strncpy(curlErrorCode, errorBuffer, CURL_ERROR_SIZE);
                  curlErrorCode[CURL_ERROR_SIZE-1] ='\0';
                  ERROR("RBIHTTPInsertSource::run: curlErrorCode: %s\n", curlErrorCode );
               }
               if ( m_sourceEvents )
               {
                  if (curl_code == CURLE_OPERATION_TIMEDOUT)
                  {
                     m_sourceEvents->httpTimeoutError( this );
                  }
                  else
                  {
                     m_sourceEvents->sourceError( this );
                  }
               }
            }
         }
         curl_easy_cleanup(curl);
      }
      else
      {
         ERROR("RBIHTTPInsertSource::run unable to create curl session");
      }

      rmf_osal_condSet(m_stopCond);
      INFO( "RBIHTTPInsertSource::run: stopping for (%s)", m_uri.c_str());
      m_threadRunning= false;
   }
}

int RBIHTTPInsertSource::receiveData( unsigned char *data, int dataLen )
{
   int consumed= 0, retProcessPackets = 0;
   rmf_Error ret;
   
   INFO( "RBIHTTPInsertSource::receivedData: got %d bytes", dataLen );

   if ( m_threadStopRequested )
   {
      DEBUG( "RBIHTTPInsertSource::receiveData: stop requested");
      consumed= 0;
   }
   else
   {
      int offset= 0;

      for( ; ; )
      {
         if(m_haveTSFormatError >= TS_FORMATERROR_RETRYCOUNT)
         {
             break;
         }
         else
         {
            pthread_mutex_lock( &m_mutex );

            int avail= m_capacity-m_count;
            int consume= dataLen-offset;
            if ( consume > avail )
            {
               consume= avail;
            }
            
            while( consume )
            {
               int copylen= (m_head >= m_tail) ? m_capacity-m_head : m_tail-m_head-1;
               if ( copylen > consume )
               {
                  copylen= consume;
               }         
               if ( copylen )
               {
                  memcpy( &m_data[m_head], &data[offset], copylen );
                  offset += copylen;
                  consume -= copylen;
                  m_head += copylen;
                  m_count += copylen;
                  if ( m_head >= m_capacity ) m_head= 0;
               }
               else
               {
                  break;
               }
            }

            pthread_mutex_unlock( &m_mutex );
         }
         
         if ( offset < dataLen )
         {
            ret= rmf_osal_condWaitFor( m_notFullCond, HTTPSRC_FULL_TIME_OUT);
            if ( ret != RMF_SUCCESS )
            {
               ERROR("RBIHTTPInsertSource::receiveData: stop requested: src full timout");
               break;
            }
         }
         else
         {
            break;
         }
         if ( m_threadStopRequested )
         {
            goto exit;
         }
      }

      retProcessPackets = m_streamProcessor.processPackets( data, offset, &m_haveTSFormatError );

      if((retProcessPackets == 0) && m_haveTSFormatError)
          INFO( "RBIHTTPInsertSource::receivedData: processPackets return false. m_haveTSFormatError %d", m_haveTSFormatError);
      
      consumed= offset;      
   }

exit:
   
   return consumed;
}

void RBIHTTPInsertSource::acquiredPMT( RBIStreamProcessor *sp )
{
   TRACE2( "RBIHTTPInsertSource::acquirePMT: have PMT from insert source");
   m_havePMT= true;   
}

void RBIHTTPInsertSource::insertionStatusUpdate( RBIStreamProcessor *sp, int detailCode )
{
   INFO("RBIHTTPInsertSource::insertionStatusUpdate");
   if ( m_sourceEvents )
   {
      m_sourceEvents->insertionStatusUpdate( sp, detailCode );
   }
}

bool RBIHTTPInsertSource::getTriggerDetectedForOpportunity(void)
{
   bool retVal = false;
   if ( m_sourceEvents )
   {
      retVal =  m_sourceEvents->getTriggerDetectedForOpportunity();
   }
   INFO("RBIHTTPInsertSource::getTriggerDetectedForOpportunity %d", retVal);
   return retVal;
}


void RBIHTTPInsertSource::foundFrame( RBIStreamProcessor *sp, int frameType, long long frameStartOffset, long long pts, long long pcr )
{
    
    m_adReadVideoFrameCnt++;
    int frameInfoSize = m_adVideoExitFrameInfo.size();
    if( frameInfoSize != 0 )
    {
        m_adVideoExitFrameInfo.pop_back();       
    }
    if(frameType == I_FRAME || frameType == P_FRAME)
    {        
        // Adding the previous frame number in decode order, the frame that we can exit Ad from
        m_adVideoExitFrameInfo.push_back(m_adReadVideoFrameCnt-1);        
    }    
    // for last video frame in the ad
    m_adVideoExitFrameInfo.push_back(m_adReadVideoFrameCnt);
    
    DEBUG("RBIHTTPInsertSource::foundFrame: Ad read frame Count %d sp %p type %d offset %llx pts %llx pcr %llx", m_adReadVideoFrameCnt, sp, frameType, frameStartOffset, pts, pcr);
  
   if ( frameType == I_FRAME )
   {
      if ( !m_haveIFrame )
      {
         pthread_mutex_lock( &m_mutex );

         m_haveIFrame= true;
         m_startPTS= pts;
         m_offsetIFrame= frameStartOffset;
         m_startPCR= pcr;
         INFO("RBIHTTPInsertSource::foundFrame: start pts %llx offset %llx pcr %llx", m_startPTS, m_offsetIFrame, m_startPCR );
         
         // Move read position to start of IFrame
         assert( m_count >= frameStartOffset );
         updateBitrate( frameStartOffset );
         
         // Make packet before I-frame contain a marker packet and
         // set read position to this packet so that the first
         // packet read from the insert source will be the marker packet         
         m_tail -= 188;
         if ( m_tail < 0 )
         {
            m_tail= m_capacity-188;
         }
         m_count += 188;
         if ( m_bitRateInfo.size() > 0 )
         {
            m_bitRateInfo[0].count += 188;
         }
         memset( &m_data[m_tail], 0xFF, 188 );
         m_data[m_tail+0]= 0x47;
         m_data[m_tail+1]= 0x1F;
         m_data[m_tail+3]= 0x00;
         m_data[m_tail+5]= 'M';
         m_data[m_tail+6]= 'a';
         m_data[m_tail+7]= 'r';
         m_data[m_tail+8]= 'k';
         m_data[m_tail+9]= 'e';
         m_data[m_tail+10]= 'r';
         m_data[m_tail+11]= 0;
         m_data[m_tail+12]= 'R';
         m_data[m_tail+13]= 'B';
         m_data[m_tail+14]= 'I';
         m_data[m_tail+15]= ' ';
         m_data[m_tail+16]= 's';
         m_data[m_tail+17]= 't';
         m_data[m_tail+18]= 'a';
         m_data[m_tail+19]= 'r';
         m_data[m_tail+20]= 't';

         pthread_mutex_unlock( &m_mutex );
      }
   }
   else
   {
      if ( m_haveIFrame )
      {
         if ( m_lenIFrame == 0 )
         {
            m_lenIFrame= frameStartOffset-m_offsetIFrame;

            TRACE2("time to edit first insert GOP: m_count %X m_lenIFrame %X m_tail %X m_head %X\n", m_count, m_lenIFrame, m_tail, m_head );
            if ( (m_count >= m_lenIFrame) && (m_tail < m_head) )
            {
               // Mark first GOP as broken and open
               RBIStreamProcessor tempProcessor;
               int retProcessPackets = 0;
               tempProcessor.useSI( sp );
               tempProcessor.setTriggerEnable( false );
               tempProcessor.setFrameDetectEnable( true );
               tempProcessor.setTrackContinuityCountersEnable( false );
               tempProcessor.markNextGOPBroken();

               TRACE2("edit GOP: m_lenIFrame %X\n", m_lenIFrame );
               pthread_mutex_lock( &m_mutex );
               retProcessPackets = tempProcessor.processPackets( &m_data[m_tail], m_lenIFrame, &m_haveTSFormatError );
               if((retProcessPackets == 0) && m_haveTSFormatError)
                   INFO( "RBIHTTPInsertSource::foundFrame tempProcessor.processPackets return false.m_haveTSFormatError %d", m_haveTSFormatError );
               pthread_mutex_unlock( &m_mutex );
            }
         }
      }
   }
}

void RBIHTTPInsertSource::foundBitRate( RBIStreamProcessor *sp, int bitRate, int chunkSize )
{
   RBIBitRateInfo info;
   
   TRACE2( "RBIHTTPInsertSource::foundBitRate: bitRate from insert source");
   
   info.count= chunkSize;
   info.bitRate= bitRate;
   
   pthread_mutex_lock( &m_mutex );   
   m_bitRateInfo.push_back( info );
   if ( m_bitRateInfo.size() == 1 )
   {
      m_bitRate= bitRate;
   }
   pthread_mutex_unlock( &m_mutex );
}

int RBIHTTPInsertSource::getNextAdPacketPid() {
    int pid = 0x1FFF;
    int txPktBytesCnt = 188;
    unsigned char packet[txPktBytesCnt];
    int tail_copy = m_tail;
    int offset = 0;
    
    while (txPktBytesCnt > 0)
    {
        int copylen = (tail_copy < m_head) ? m_head - tail_copy : m_capacity - tail_copy;
        if (copylen > 0)
        {
            if (copylen >= txPktBytesCnt) {
                copylen = txPktBytesCnt;
            }
            memcpy(&packet[offset], &m_data[tail_copy], copylen);
            offset += copylen;
            txPktBytesCnt -= copylen;
            tail_copy += copylen;
             if ( tail_copy >= m_capacity ) tail_copy= 0;
        }
        else
        {
           break;
        }        
    }

    if(txPktBytesCnt == 0)
    {
        pid = (((packet[1] << 8) | packet[2]) & 0x1FFF);            
    }

    return pid;
}

RBIInsertDefinition::RBIInsertDefinition( int triggerId, bool replace, int duration, const char *assetID, const char *providerID, const char *uri, const char *trackingIdString, bool validSpot, int spotDetailCode )
  : m_replace(replace), m_validSpot(validSpot), m_duration(duration), m_spotDetailCode(spotDetailCode), m_assetID(assetID),
    m_providerID(providerID), m_uri(uri), m_trackingIdString(trackingIdString)
{
}

RBIInsertSource* RBIInsertDefinition::getInsertSource() const
{
   RBIInsertSource *src= 0;

   if ( m_replace )
   {
      RBIHTTPInsertSource *httpsrc= new RBIHTTPInsertSource( m_triggerId, m_uri, 0 );
      if ( httpsrc )
      {
         if ( httpsrc->open() )
         {
            src= httpsrc;
         }
         else
         {
            ERROR( "RBIInsertDefinition::getInsertSource: unable to open insert src id %X uri(%s)\n", m_triggerId, m_uri.c_str() );
            delete httpsrc;
         }
      }
   }

   return src;   
}

RBIInsertDefinitionSet::RBIInsertDefinitionSet( long long utcTrigger, long long utcSplice, long long splicePTS )
  : m_refCount(1), m_utcTimeTrigger(utcTrigger), m_utcTimeSplice(utcSplice), m_splicePTS(splicePTS)
{
}

bool RBIInsertDefinitionSet::addSpot( RBIInsertDefinition spot )
{
   bool result= true;
   SpotTimeInfo timeInfo;
   long long utcTime= m_utcTimeSplice;
   long long ptsTime= m_splicePTS;
   long long duration;
   
   INFO("RBIInsertDefinitionSet::addSpot: break utcTimeSplice %lld ptsTime %llx", utcTime, ptsTime );
   try
   {
      m_spots.push_back( spot );
   }
   catch(...)
   {
      result= false;
   }
   
   if ( result )
   {
      int spotIdx= m_spots.size()-1;

      INFO("RBI LSA-P: Insertion Attempt for spot %d", spotIdx);
      for( int i= 0; i < spotIdx; ++i )
      {
         duration= m_spots[i].getDuration();
         utcTime += duration;
         ptsTime = ((ptsTime + (duration*90LL))&MAX_90K);
      }

      timeInfo.utcTimeStart= utcTime;
      timeInfo.startPTS= ptsTime;
      INFO("RBIInsertDefinitionSet::addSpot: spot utcTimeStart %lld startPTS %llx", timeInfo.utcTimeStart, timeInfo.startPTS );
      try
      {
         m_spotTimes.push_back( timeInfo );
      }
      catch(...)
      {
         m_spots.pop_back();
         result= false;
      }
   }
   
   assert( m_spots.size() == m_spotTimes.size() );
   
   return result;
}

const RBIInsertDefinition* RBIInsertDefinitionSet::getSpot( int index )
{
   RBIInsertDefinition *spot= 0;
   
   if ( (index >= 0) && ((unsigned int)index < m_spots.size()) )
   {
      spot= &m_spots[index];
   }
   
   return spot;
}

const SpotTimeInfo* RBIInsertDefinitionSet::getSpotTime( int index )
{
   SpotTimeInfo *time= 0;
   
   if ( (index >= 0) && ((unsigned int)index < m_spotTimes.size()) )
   {
      time= &m_spotTimes[index];
   }
   
   return time;
}

RBITrigger::RBITrigger( int ctx, int id, int idInstance, long long splicePTS, long long leadTime )
{
   struct timeval tv;
   
   gettimeofday( &tv, 0 );
   
   m_eventContext= ctx;
   m_eventId= id;
   m_eventIdInstance= idInstance;
   m_unifiedId= ((ctx<<24)|(id<<8)|(idInstance));
   m_splicePTS= splicePTS;
   m_utcTimeTrigger= UTCMILLIS(tv);
   m_utcTimeSplice= m_utcTimeTrigger+((leadTime+45LL)/90LL);
   m_poData = 0;
   INFO("RBITrigger ctor: splicePTS %llx m_utcTimeTrigger %llx m_utcTimeSplice %llx leadTime %llx", splicePTS, m_utcTimeTrigger, m_utcTimeSplice, leadTime );
}

RBIManager* RBIManager::m_instance= 0;
pthread_mutex_t RBIManager::m_mutex= PTHREAD_MUTEX_INITIALIZER;
static bool bRegisterTunerStatus=false;

static void tunerActivityCallback(const std::string &receiverId, const std::string &activityStatus,const std::string &lastActivityTimeStamp,const std::string &lastActivityAction)
{
    INFO("RBIFilter tunerActivityCallback receiverId %s tunerActivityStatus %s lastActivityTimeStamp:%s lastActivityAction: %s", receiverId.c_str(), activityStatus.c_str(),lastActivityTimeStamp.c_str(),lastActivityAction.c_str());
    RBIManager *rbiManager = RBIManager::getInstance();
    if ( rbiManager )
    {
        rbiManager->setSourceTunerStatus( receiverId.c_str(), activityStatus.c_str(), lastActivityTimeStamp.c_str(), lastActivityAction.c_str());
    }
}

RBIManager* RBIManager::getInstance()
{
   if ( !m_instance )
   {
      m_instance= new RBIManager();
      if(bRegisterTunerStatus == false)
      {
         RMFQAMSrc::registerUpdateTunerActivityCallback(tunerActivityCallback);
         bRegisterTunerStatus = true;
      }
   }
   return m_instance;
}

RBIManager::RBIManager()
  : m_definitionRequestTimeout(RBI_DEFAULT_DEFINITION_REQUEST_TIMEOUT), m_H264Enabled(false), m_captureEnabled(false), 
    m_audioReplicationEnabled(false), m_spliceOffset(0), m_spliceTimeout(RBI_DEFAULT_DEFINITION_SPLICE_TIMEOUT),
    m_marginOfError(RBI_DEFAULT_ASSET_DURATION_OFFSET), m_bufferSize(RBI_DEFAULT_DEFINITION_BUFFER_SIZE), m_retryCount(RBI_DEFAULT_PSN_RETRY_COUNT), 
    m_progammerEnablementEnabled(false), m_haveDeviceId(false), m_timeZoneMinutesWest(0), m_STTAcquired(false), m_nextSessionNumber(1)
{
   const char *env;
   RFC_ParamData_t rfcParam1, rfcParam2;
   WDMP_STATUS wdmpStatus1, wdmpStatus2;
   char RFC_CALLERID_LSA[] = "LSA";
   char RFC_PARAM_LSA_ENABLE[] = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.LSA.Enable";
   char RFC_PARAM_ADCACHE_ENABLE[] = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.LSA.AdCacheEnable";
   char RFC_PARAM_PROGRAMMER_ENABLE[] = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.LSA.ProgrammerEnable";
   char RFC_PARAM_BYTERANGE_DOWNLOAD[] = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.LSA.ByteRangeDownload";
   char RFC_LSA_ENABLE[] = "RFC_ENABLE_LSA";
   char RFC_LSA_ADCACHE_ENABLE[] = "RFC_DATA_LSA_AdCacheEnable";
   char RFC_LSA_PROGRAMMER_ENABLE[] = "RFC_DATA_LSA_ProgrammerEnablement";
   char RFC_LSA_BYTE_RANGE_DOWNLOAD[] = "RFC_DATA_LSA_ByteRangeDownload";

   //The main process must register with IARMBUS
   IARM_Bus_Init(RBI_RPC_NAME);
   IARM_Bus_Connect();

   memset( &m_mon, 0, sizeof(RBIPrivateDataMonitor) );
   memset( m_timeZoneName, 0, sizeof(m_timeZoneName));

   #ifdef XONE_STB
   INFO("Using Intel SOC config");
   #endif

   memset(&rfcParam1, 0, sizeof(rfcParam1));
   wdmpStatus1 = getRFCParameter(RFC_CALLERID_LSA, RFC_PARAM_LSA_ENABLE, &rfcParam1);
   memset(&rfcParam2, 0, sizeof(rfcParam2));
   wdmpStatus2 = getRFCParameter(RFC_CALLERID_LSA, RFC_LSA_ENABLE, &rfcParam2);

   if((WDMP_SUCCESS == wdmpStatus1) || (WDMP_SUCCESS == wdmpStatus2))
   {
      INFO ("getRFCParameter for %s[%s], %s[%s]\n",
         RFC_PARAM_LSA_ENABLE, ((WDMP_SUCCESS == wdmpStatus1) ? rfcParam1.value : ""),
         RFC_LSA_ENABLE, ((WDMP_SUCCESS == wdmpStatus2) ? rfcParam2.value : ""));

      if(((WDMP_SUCCESS == wdmpStatus1) && (0 == strcasecmp(rfcParam1.value, "TRUE"))) ||\
         ((WDMP_SUCCESS == wdmpStatus2) && (0 == strcasecmp(rfcParam2.value, "TRUE"))))
      {
         m_H264Enabled = true;
         INFO("RBI LSA-M: H.264 Enabled");
      }
   }
   else
   {
      ERROR ("Error getting LSA Enablement: %s, %s", getRFCErrorString(wdmpStatus1), getRFCErrorString(wdmpStatus2));
   }

   // If "FEATURE.RBI.ADCACHE.ENABLE" entry not available in rmfconfig.ini, then AdCache feature is not supported for that platforms.
   env= rmf_osal_envGet( "FEATURE.RBI.ADCACHE.ENABLE" );
   if(env)
   {
      memset(&rfcParam1, 0, sizeof(rfcParam1));
      wdmpStatus1 = getRFCParameter(RFC_CALLERID_LSA, RFC_PARAM_ADCACHE_ENABLE, &rfcParam1);
      memset(&rfcParam2, 0, sizeof(rfcParam2));
      wdmpStatus2 = getRFCParameter(RFC_CALLERID_LSA, RFC_LSA_ADCACHE_ENABLE, &rfcParam2);

      if((WDMP_SUCCESS == wdmpStatus1) || (WDMP_SUCCESS == wdmpStatus2))
      {
         INFO ("getRFCParameter for %s[%s], %s[%s]\n",
            RFC_PARAM_ADCACHE_ENABLE, ((WDMP_SUCCESS == wdmpStatus1) ? rfcParam1.value : ""),
            RFC_LSA_ADCACHE_ENABLE, ((WDMP_SUCCESS == wdmpStatus2) ? rfcParam2.value : ""));

         if(((WDMP_SUCCESS == wdmpStatus1) && (0 == strcasecmp(rfcParam1.value, "TRUE"))) ||\
            ((WDMP_SUCCESS == wdmpStatus2) && (0 == strcasecmp(rfcParam2.value, "TRUE"))))
         {
            m_isAdCacheEnabled = true;
            INFO("RBI LSA-M: AD Cache Enabled");
         }
      }
      else
         ERROR ("Error getting AD Cache Enablement: %s, %s", getRFCErrorString(wdmpStatus1), getRFCErrorString(wdmpStatus2));
   }
   else
   {
      INFO("FEATURE.RBI.ADCACHE.ENABLE entry not found in rmfconfig.ini");
   }

   if(m_isAdCacheEnabled == false)
      INFO("RBI LSA-M: AD Cache Not Enabled");

   if(m_isAdCacheEnabled)
   {
      rtLogSetLevel(RT_LOG_INFO);

      /*
       char systemCommand[50] = "";
       int ret = 0;

       // Delete the cache path if already exists.
       sprintf(systemCommand, "rm -rf %s", ADCACHE_LOCATION);
       ret = system(systemCommand);
       if (ret != 0) {
         ERROR ("System call failed on %s", systemCommand);
       }
      */
      rtFileCache::instance()->setCacheDirectory(ADCACHE_LOCATION);
      rtFileCache::instance()->setMaxCacheSize(rtFileCache::instance()->maxCacheSize() + (ADCACHE_SIZE_MB * ADCACHE_SIZE));
   }

   memset(&rfcParam1, 0, sizeof(rfcParam1));
   wdmpStatus1 = getRFCParameter(RFC_CALLERID_LSA, RFC_PARAM_PROGRAMMER_ENABLE, &rfcParam1);
   memset(&rfcParam2, 0, sizeof(rfcParam2));
   wdmpStatus2 = getRFCParameter(RFC_CALLERID_LSA, RFC_LSA_PROGRAMMER_ENABLE, &rfcParam2);

   if((WDMP_SUCCESS == wdmpStatus1) || (WDMP_SUCCESS == wdmpStatus2))
   {
      INFO ("getRFCParameter for %s[%s], %s[%s]\n",
         RFC_PARAM_PROGRAMMER_ENABLE, ((WDMP_SUCCESS == wdmpStatus1) ? rfcParam1.value : ""),
         RFC_LSA_PROGRAMMER_ENABLE, ((WDMP_SUCCESS == wdmpStatus2) ? rfcParam2.value : ""));
      if((0 == strcasecmp(rfcParam1.value, "TRUE")) || (0 == strcasecmp(rfcParam2.value, "TRUE")))
      {
         m_progammerEnablementEnabled = true;
         INFO("RBI LSA-M: Programmer Enablement Enabled");
      }
   }
   else
      ERROR ("Error getting Programmer Enablement: %s, %s", getRFCErrorString(wdmpStatus1), getRFCErrorString(wdmpStatus2));

   if(m_progammerEnablementEnabled == false)
      INFO("RBI LSA-M: Programmer Enablement Not Enabled");

   memset(&rfcParam1, 0, sizeof(rfcParam1));
   wdmpStatus1 = getRFCParameter(RFC_CALLERID_LSA, RFC_PARAM_BYTERANGE_DOWNLOAD, &rfcParam1);
   memset(&rfcParam2, 0, sizeof(rfcParam2));
   wdmpStatus2 = getRFCParameter(RFC_CALLERID_LSA, RFC_LSA_BYTE_RANGE_DOWNLOAD, &rfcParam2);

   if((WDMP_SUCCESS == wdmpStatus1) || (WDMP_SUCCESS == wdmpStatus2))
   {
      INFO ("getRFCParameter for %s[%s], %s[%s]\n",
         RFC_PARAM_BYTERANGE_DOWNLOAD, ((WDMP_SUCCESS == wdmpStatus1) ? rfcParam1.value : ""),
         RFC_LSA_BYTE_RANGE_DOWNLOAD, ((WDMP_SUCCESS == wdmpStatus2) ? rfcParam2.value : ""));
      if((0 == strcasecmp(rfcParam1.value, "TRUE")) || (0 == strcasecmp(rfcParam2.value, "TRUE")))
      {
         m_isByteRangeEnabled = true;
         INFO("RBI LSA-M: Byte Range Download Enabled");
      }
   }
   else
      ERROR ("Error getting Byte Range Download: %s, %s", getRFCErrorString(wdmpStatus1), getRFCErrorString(wdmpStatus2));

   if(m_isByteRangeEnabled == false)
      INFO("RBI LSA-M: Byte Range Download Not Enabled");

   env= rmf_osal_envGet( "FEATURE.RBI.CAPTURE" );
   if ( env )
   {
      INFO("Read (%s) from config for FEATURE.RBI.CAPTURE", env );
      if(0 == strcasecmp(env, "TRUE"))
      {
         INFO("Enabling RBI insertion capture.  Output will be /opt/rbicapture-in.ts and /opt/rbicapture-out.ts");
         m_captureEnabled= true;
      }
   }

   env= rmf_osal_envGet( "FEATURE.RBI.AUDIO.REPLICATE" );
   if ( env )
   {
      INFO("Read (%s) from config for FEATURE.RBI.AUDIO.REPLICATE", env );
      if(0 == strcasecmp(env, "TRUE"))
      {
         INFO("Enabling RBI Audio PID Replication");
         m_audioReplicationEnabled= true;
      }
   }

   IARM_Bus_RegisterCall(RBI_DEFINE_SPOTS, rbiDefineSpotsHandler);
   IARM_Bus_RegisterCall(RBI_MONITOR_PRIVATEDATA, rbiMonitorPrivateDataHandler);

   // Fetch the imagename from the /version.txt for curl userAgent
   {
      std::string line;
      std::string strToFind = "imagename:";
      std::ifstream versionFile ("/version.txt",std::ifstream::in);
      mUserAgent= "QAMRDK/version unassigned";

      if (versionFile.is_open())
      {
         while ( std::getline (versionFile,line) )
         {
            std::size_t found = line.find(strToFind);
            if (found!=std::string::npos)
            {
               mUserAgent = "QAMRDK/" + line.substr(strToFind.length());
            }
         }
         versionFile.close();
      }
      else
      {
         INFO("Unable to open version.txt file");
      }
      INFO("User Agent for curl request is (%s)", mUserAgent.c_str());
   }
}

RBIManager::~RBIManager()
{
   pthread_mutex_destroy( &m_mutex );   
}

RBIContext* RBIManager::getContext()
{
   RBIContext *ctx= 0;
   
   {
      ctx= new RBIContext();

      if ( !ctx->init() )
      {
          delete ctx;
          ctx = 0;
      }
      
      pthread_mutex_lock( &m_mutex );

      if ( ctx )
      {
         char work[64];
         
         ctx->acquire();

         ctx->setPrivateDataMonitor( m_mon );
         
         // Assign a sessionId and session number to the new context and add to map
         uuid_generate( ctx->m_sessionId );
         uuid_unparse( ctx->m_sessionId, work );
         ctx->m_sessionIdStr= std::string( work );
         ctx->m_sessionNumber= m_nextSessionNumber++;         
         m_sessionIdMap.insert( std::pair<std::string, RBIContext*>( ctx->m_sessionIdStr, ctx ) );
      }

      pthread_mutex_unlock( &m_mutex );
   }   
   
   return ctx;
}

void RBIManager::releaseContext( RBIContext *ctx )
{
   if ( ctx )
   {
      {
         pthread_mutex_lock( &m_mutex );
         
         if ( ctx->release() == 0 )
         {
            std::map<std::string, RBIContext*>::iterator it= m_sessionIdMap.find( ctx->m_sessionIdStr );
            if ( it != m_sessionIdMap.end() )
            {
               m_sessionIdMap.erase( ctx->m_sessionIdStr );
            }
         }
         
         pthread_mutex_unlock( &m_mutex );

         ctx->term();
         delete ctx;
      }
   }
}

int RBIManager::getSessionNumber( std::string sessionId )
{
   int sessionNumber= -1;
   
   {
      pthread_mutex_lock( &m_mutex );

      std::map<std::string, RBIContext*>::iterator it= m_sessionIdMap.find( sessionId );
      if ( it != m_sessionIdMap.end() )
      {
         RBIContext *ctx= (RBIContext*)it->second;
         sessionNumber= ctx->m_sessionNumber;
      }
      
      pthread_mutex_unlock( &m_mutex );
   }
   
   return sessionNumber;
}

void RBIManager::setPrivateDataMonitor( RBIPrivateDataMonitor mon )
{
   {
      pthread_mutex_lock( &m_mutex );
      
      m_mon= mon;
      std::map<std::string,RBIContext*>::iterator it= m_sessionIdMap.begin();
      while ( it != m_sessionIdMap.end() )
      {
         RBIContext *ctx= (RBIContext*)it->second;
         if ( ctx )
         {
            ctx->setPrivateDataMonitor( mon );
         }
         ++it;
      }
      
      pthread_mutex_unlock( &m_mutex );
   }
}

int RBIManager::registerInsertOpportunity( std::string sessionId, long long utcTrigger, long long utcSplice, long long splicePTS )
{
   int result;
   PendingOpportunity pending;
   
   {
      pthread_mutex_lock( &m_mutex );

      std::map<std::string, PendingOpportunity>::iterator it= m_pendingOpportunities.find( sessionId );
      if ( it == m_pendingOpportunities.end() )
      {
         if( (utcSplice - utcTrigger) < m_spliceTimeout)
         {
            pending.utcTrigger= utcTrigger;
            pending.utcSplice= utcSplice;
            pending.splicePTS= splicePTS;
            m_pendingOpportunities.insert( std::pair<std::string, PendingOpportunity>( sessionId, pending ) );
            INFO("RBIManager::registerInsertOpportunity utcTrigger %lld utcSplice %lld splicePTS %lld\n", utcTrigger,utcSplice,splicePTS );
            result= RBIResult_ok;
         }
         else
         {
           ERROR("RBIManager::registerInsertOpportunity leadTime %llu Cannot register insertionOpportunity\n", (utcSplice - utcTrigger) );
           result= RBIResult_spliceTimeout;
         } 
      }
      else
      {
         result= RBIResult_alreadyExists;
      }

      pthread_mutex_unlock( &m_mutex );
   }

   return result;
}

void RBIManager::unregisterInsertOpportunity( std::string sessionId )
{
   {
      pthread_mutex_lock( &m_mutex );
      
      std::map<std::string, PendingOpportunity>::iterator it= m_pendingOpportunities.find( sessionId );
      if ( it != m_pendingOpportunities.end() )
      {
         m_pendingOpportunities.erase( sessionId );
      }
      
      pthread_mutex_unlock( &m_mutex );
   }
}

bool RBIManager::findInsertOpportunity( std::string sessionId, long long& utcTrigger, long long& utcSplice, long long& splicePTS )
{
   bool opportunityFound= false;
   {
      pthread_mutex_lock( &m_mutex );

      std::map<std::string, PendingOpportunity>::iterator it= m_pendingOpportunities.find( sessionId );
      if ( it != m_pendingOpportunities.end() )
      {
         PendingOpportunity pending= (PendingOpportunity)it->second;
         
         opportunityFound= true;
         
         utcTrigger= pending.utcTrigger;
         utcSplice= pending.utcSplice;
         splicePTS= pending.splicePTS;
      }

      pthread_mutex_unlock( &m_mutex );
   }
   
   return opportunityFound;
}

int RBIManager::ageInsertOpportunities( std::string sessionId )
{
   int numPending= 0;
   struct timeval tv;
   long long utcTimeCurrent;
   
   {
      pthread_mutex_lock( &m_mutex );

      if ( m_pendingOpportunities.size() > 0 )
      {
         std::map<std::string, PendingOpportunity>::iterator it= m_pendingOpportunities.find( sessionId );
         if ( it != m_pendingOpportunities.end() )
         {
            PendingOpportunity pending= (PendingOpportunity)it->second;
            
            gettimeofday( &tv, 0 );
            utcTimeCurrent= UTCMILLIS(tv);
         
            if ( utcTimeCurrent >= pending.utcTrigger+m_definitionRequestTimeout )
            {
               INFO("RBI Pending opportunity erased, utcTimeCurrent[%llx] utcTrigger[%llx] m_definitionRequestTimeout[%d]", utcTimeCurrent, pending.utcTrigger, m_definitionRequestTimeout ); 
               m_pendingOpportunities.erase(it);
            }
            else
            {
               numPending= 1;
               INFO("RBI Pending opportunity good, utcTimeCurrent[%llx] utcTrigger[%llx] m_definitionRequestTimeout[%d]", utcTimeCurrent, pending.utcTrigger, m_definitionRequestTimeout );
            }
         }
      }      
      
      pthread_mutex_unlock( &m_mutex );
   }
   
   return numPending;
}

int RBIManager::registerInsertDefinitionSet(std::string sessionId, RBIInsertDefinitionSet dfnSet )
{
   int result;
 
   {
      pthread_mutex_lock( &m_mutex );

      std::map<std::string, RBIInsertDefinitionSet>::iterator it= m_insertDefinitions.find( sessionId );
      if ( it == m_insertDefinitions.end() )
      {
         m_insertDefinitions.insert( std::pair<std::string, RBIInsertDefinitionSet>( sessionId, dfnSet ) );

         result= RBIResult_ok;
      }
      else
      {
         result= RBIResult_alreadyExists;
      }

      pthread_mutex_unlock( &m_mutex );
   }

   return result;
}

void RBIManager::unregisterInsertDefinitionSet(std::string sessionId)
{
   {
      pthread_mutex_lock( &m_mutex );
      
      std::map<std::string, RBIInsertDefinitionSet>::iterator it= m_insertDefinitions.find( sessionId );
      if ( it != m_insertDefinitions.end() )
      {
         RBIInsertDefinitionSet *set= (RBIInsertDefinitionSet*)&it->second;
         assert( set->m_refCount > 0 );
         if ( --set->m_refCount == 0 )
         {
            m_insertDefinitions.erase( sessionId );
         }
      }
      
      pthread_mutex_unlock( &m_mutex );
   }
}

RBIInsertDefinitionSet* RBIManager::getInsertDefinitionSet( std::string sessionId )
{
   RBIInsertDefinitionSet *set= 0;
   
   {
      pthread_mutex_lock( &m_mutex );

      std::map<std::string, RBIInsertDefinitionSet>::iterator it= m_insertDefinitions.find( sessionId );
      if ( it != m_insertDefinitions.end() )
      {
         set= (RBIInsertDefinitionSet*)&it->second;
         ++set->m_refCount;
      }

      pthread_mutex_unlock( &m_mutex );
   }
   
   return set;
}

void RBIManager::releaseInsertDefinitionSet( std::string sessionId )
{
   {
      pthread_mutex_lock( &m_mutex );

      std::map<std::string, RBIInsertDefinitionSet>::iterator it= m_insertDefinitions.find( sessionId );
      if ( it != m_insertDefinitions.end() )
      {
         RBIInsertDefinitionSet *set= (RBIInsertDefinitionSet*)&it->second;
         assert( set->m_refCount > 0 );
         --set->m_refCount;
         if ( set->m_refCount == 0 )
         {
            m_insertDefinitions.erase( sessionId );
         }
      }

      pthread_mutex_unlock( &m_mutex );
   }
}

void RBIManager::releaseInsertSource( RBIInsertSource *is)
{
   if ( is )
   {
      is->close();
      delete is;
   }
}

size_t static writeCurlResponse(void *ptr, size_t size, size_t nmemb, std::string stream)
{
   size_t realsize = size * nmemb;
   std::string temp(static_cast<const char*>(ptr), realsize);
   stream.append(temp);
   return realsize;
}

void RBIManager::getDeviceId( unsigned char deviceId[6] )
{
   if ( !m_haveDeviceId )
   {
      char work[32];
      char work1[256];
      char work2[32];
      int values[6];
      
      {
         pthread_mutex_lock( &m_mutex );
         
         if ( !m_haveDeviceId )
         {
            int rc;
            const char *ifname= 0;
            FILE *pFile= 0;
            
            // Look for ESTB interface name in /etc/device.properties
            pFile= fopen( "/etc/device.properties", "r" );
            if ( pFile )
            {
               for( ; ; )
               {
                  if ( fscanf( pFile, "%256[^=]=%s\n", work1, work2 ) == 2 )
                  {
                     DEBUG("got %s=%s", work1, work2 );
                     int len= strlen(work1);
                     if ( (len == 15) && (strncmp( work1, "DEFAULT_ESTB_IF", 15 ) == 0) )
                     {                     
                        ifname= work2;
                        INFO("Getting ESTB ifname from /etc/device.properties: DEFAULT_ESTB_IF=%s", ifname );
                        break;
                     }
                  }
                  else
                  {
                     break;
                  }
               }
               fclose( pFile );
            }
            else
            {
               WARNING("Unable to access /etc/device.properties");
            }

            if ( !ifname )
            {
               const char *env= rmf_osal_envGet( "FEATURE.MRDVR.MEDIASERVER.IFNAME" );
               if ( env )
               {
                  ifname= env;
               }
               else
               {
                  WARNING("Unable to get interface name from FEATURE.MRDVR.MEDIASERVER.IFNAME");
               }
            }
            
            if ( !ifname )
            {
               ifname= "eth1";
            }
            
            rc= rmf_osal_socket_getMacAddress( (char*)ifname, work );
            INFO("Getting Mac address for ifname: %s rc %d", ifname, rc );
            
            if ( rc == 0 )
            {
               INFO("ifname %s Mac address %s", ifname, work );
               
               rc= sscanf( work, "%2X:%2X:%2X:%2X:%2X:%2X",
                           &values[0],
                           &values[1],
                           &values[2],
                           &values[3],
                           &values[4],
                           &values[5] );
               if ( rc == 6 )
               {
                  for( int i= 0; i < 6; ++i )
                  {
                     m_deviceId[i]= (unsigned char)(values[i]&0xFF);
                  }
                  INFO("Using deviceId %02X:%02X:%02X:%02X:%02X:%02X", 
                       m_deviceId[0],
                       m_deviceId[1],
                       m_deviceId[2],
                       m_deviceId[3],
                       m_deviceId[4],
                       m_deviceId[5] );
                       
                  m_haveDeviceId= true;
               }
               else
               {
                  ERROR("Error parsing mac address, rc=%d", rc );
               }               
            }
         }
         
         pthread_mutex_unlock( &m_mutex );
      }
   }
   
   if ( m_haveDeviceId )
   {
      memcpy( deviceId, m_deviceId, sizeof(m_deviceId) );
   }
}

void RBIManager::setTimeZoneName( )
{
   struct tm stm;
   time_t timeval;

   tzset();
   if(getenv("TZ") != NULL)
   {
      time(&timeval);
      localtime_r(&timeval, &stm);
      if(stm.tm_zone)
      {
         if(m_timeZoneMinutesWest != (stm.tm_gmtoff/3600)*60 )
         {
            m_timeZoneMinutesWest = (stm.tm_gmtoff/3600)*60;
            strcpy(m_timeZoneName,getenv("TZ"));

            RBI_LSA_Timezone_Update timeZone;
            memset(&timeZone, 0, sizeof(timeZone));
            timeZone.tzOffset= m_timeZoneMinutesWest;
            strcpy( timeZone.tzName, m_timeZoneName);
            IARM_Bus_BroadcastEvent(RBI_RPC_NAME, RBI_EVENT_TIMEZONE_CHANGE, &timeZone, sizeof(timeZone));
            INFO("Timezone received: (%s) stm.tm_gmtoff =%ld isdst = %d m_timeZoneName %s\n ", stm.tm_zone, stm.tm_gmtoff/3600, stm.tm_isdst, m_timeZoneName);
            m_STTAcquired = true;
         }
      }
      else
      {
         ERROR("Invalid time zone value");
      }
   }
}

void RBIManager::printTunerActivityStatusMap(void)
{
   std::string srcUriTemp;
   ReceiverList receiverIdList;
   XREActivity activity;
   std::string liveDVRString;

   INFO("Source URI map size %d", m_srcUriMap.size());
   for (SrcUriMap::iterator srcUriMap_iter = m_srcUriMap.begin(); srcUriMap_iter != m_srcUriMap.end(); srcUriMap_iter++)
   {
      srcUriTemp = srcUriMap_iter->first;
      INFO("\t%s", srcUriTemp.c_str());
      receiverIdList = srcUriMap_iter->second;

      for(ReceiverList::iterator receiverIdListIter = receiverIdList.begin(); receiverIdListIter != receiverIdList.end(); ++receiverIdListIter)
      {
         if((*receiverIdListIter).liveDVRState.DVR)
         {
            switch((*receiverIdListIter).liveDVRState.Live_TSB)
            {
               case RBI_LIVE:
                  liveDVRString = std::string("Live-DVR");
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
            switch((*receiverIdListIter).liveDVRState.Live_TSB)
            {
               case RBI_LIVE:
                  liveDVRString = std::string("Live");
                  break;

               case RBI_TSB:
                  liveDVRString = std::string("TSB");
                  break;
            }
         }
         INFO("\t\t%s\t%s\t%s\t%s\t%s",
            (*receiverIdListIter).receiverId,
            (*receiverIdListIter).activityState,
            (*receiverIdListIter).tuneTime,
            (*receiverIdListIter).lastActivityTime,
            liveDVRString.c_str());
      }
   }

   INFO("ReceiverId map size %d", m_ReceiverIdMap.size());
   for (ReceiverIdMap::iterator receiverIdMapIter = m_ReceiverIdMap.begin(); receiverIdMapIter != m_ReceiverIdMap.end(); receiverIdMapIter++)
   {
      srcUriTemp = receiverIdMapIter->first;
      activity = receiverIdMapIter->second;
      INFO("\t<%s, %s %s>", srcUriTemp.c_str(), activity.activityState.c_str(), activity.lastActivityTime.c_str());
   }
}

void RBIManager::setSourceTunerStatus( const char *receiverId, const char *tunerActivityStatus, const char *lastActivityTime, const char *lastActivityAction )
{
   INFO("setSourceTunerStatus receiverId %s tunerActivityStatus %s lastActivityTime %s lastActivityAction %s", receiverId, tunerActivityStatus, lastActivityTime, lastActivityAction);
   std::string strReceiverId(receiverId);
   XREActivity activity;

   activity.activityState = std::string(tunerActivityStatus);
   activity.lastActivityTime = std::string(lastActivityTime);

   pthread_mutex_lock( &m_mutex );

   ReceiverIdMap::iterator receiverIdMapIter = m_ReceiverIdMap.find(strReceiverId);

   if ( receiverIdMapIter == m_ReceiverIdMap.end() )
   {
      m_ReceiverIdMap.insert( std::make_pair( strReceiverId, activity ));
   }
   else
   {
      m_ReceiverIdMap[strReceiverId] = activity;
   }
   printTunerActivityStatusMap();
   pthread_mutex_unlock( &m_mutex );
}

void RBIManager::addReceiverId(const char *uri, const char *receiverId, int isLiveSrc)
{
   INFO("addReceiverId source URI %s ReceiverId %s isLiveSrc %d", uri, receiverId, isLiveSrc);
   std::string srcUri(uri);
   ReceiverActivity receiverActivity;
   struct timeval tv;
   RBI_LIVEDVR liveDVR = (RBI_LIVEDVR)isLiveSrc;

   gettimeofday( &tv, 0 );

   memset(&receiverActivity, 0, sizeof(receiverActivity));
   strncpy( receiverActivity.receiverId, receiverId, sizeof(receiverActivity.receiverId)-1);
   strncpy( receiverActivity.activityState, "Active", sizeof(receiverActivity.activityState)-1);
   snprintf( receiverActivity.tuneTime, sizeof(receiverActivity.tuneTime)-1, "%lld", UTCMILLIS(tv));

   if(liveDVR == RBI_DVR)
   {
      receiverActivity.liveDVRState.DVR = 1;
   }
   else
   {
      receiverActivity.liveDVRState.Live_TSB = liveDVR;
   }

   pthread_mutex_lock( &m_mutex );
   SrcUriMap::iterator srcUriMap_iter = m_srcUriMap.find(srcUri);
   if ( srcUriMap_iter == m_srcUriMap.end() )
   {
      ReceiverList receiverIdList;

      receiverIdList.push_back(receiverActivity);
      m_srcUriMap.insert( std::make_pair( srcUri, receiverIdList ));
   }
   else
   {
      ReceiverList receiverIdList = srcUriMap_iter->second;
      ReceiverList::iterator receiverIdListIter = receiverIdList.begin();

      for( ; receiverIdListIter != receiverIdList.end(); receiverIdListIter++)
      {
         if(strcmp((*receiverIdListIter).receiverId, receiverId) == 0)
         {
            INFO("ReceiverId %s already found in Source URI map", receiverId );

            snprintf( (*receiverIdListIter).tuneTime, sizeof((*receiverIdListIter).tuneTime)-1, "%lld", UTCMILLIS(tv));

            if(liveDVR == RBI_DVR)
               (*receiverIdListIter).liveDVRState.DVR = 1;
            else
               (*receiverIdListIter).liveDVRState.Live_TSB = liveDVR;

            break;
         }
      }
      if(receiverIdListIter == receiverIdList.end())
      {
         receiverIdList.push_back(receiverActivity);
      }
      m_srcUriMap[srcUri] = receiverIdList;
   }

   printTunerActivityStatusMap();
   pthread_mutex_unlock( &m_mutex );
}

void RBIManager::removeReceiverId(const char *uri, const char *receiverId, int isLiveSrc)
{
   INFO("removeReceiverId source URI %s ReceiverId %s isLiveSrc %d", uri, receiverId, isLiveSrc);
   std::string srcUri(uri);
   std::string strReceiverId(receiverId);
   RBI_LIVEDVR liveDVR = (RBI_LIVEDVR)isLiveSrc;

   pthread_mutex_lock( &m_mutex );
   SrcUriMap::iterator srcUriMap_iter = m_srcUriMap.find(srcUri);

   if ( srcUriMap_iter == m_srcUriMap.end() )
   {
      ERROR( "%s not found in Source URI map", srcUri.c_str());
   }
   else
   {
      ReceiverList receiverIdList = srcUriMap_iter->second;
      ReceiverList::iterator receiverIdListIter = receiverIdList.begin();

      for( ; receiverIdListIter != receiverIdList.end(); receiverIdListIter++)
      {
         if(strcmp((*receiverIdListIter).receiverId, receiverId) == 0)
         {
            if(liveDVR == RBI_DVR)
               (*receiverIdListIter).liveDVRState.DVR = 0;
            else
               (*receiverIdListIter).liveDVRState.Live_TSB = 0;

            if(((*receiverIdListIter).liveDVRState.Live_TSB == 0) && ((*receiverIdListIter).liveDVRState.DVR == 0))
               receiverIdListIter = receiverIdList.erase(receiverIdListIter);
         }
      }

      // If the list is empty, remove the key(uri) from the map.
      if(!receiverIdList.empty())
         m_srcUriMap[srcUri] = receiverIdList;
      else
         m_srcUriMap.erase(srcUriMap_iter);
   }

   printTunerActivityStatusMap();
   pthread_mutex_unlock( &m_mutex );
}

void RBIManager::getTunerStatus(std::string sourceUri, ReceiverActivity recActivity[], size_t *totalReceivers)
{
   pthread_mutex_lock( &m_mutex );
   SrcUriMap::iterator srcUriMap_iter = m_srcUriMap.find(sourceUri);

   if ( srcUriMap_iter == m_srcUriMap.end() )
   {
      INFO( "%s not found in Source URI map", sourceUri.c_str());
   }
   else
   {
      ReceiverList receiverIdList = srcUriMap_iter->second;
      XREActivity activity;

      for(ReceiverList::iterator receiverIdListIter = receiverIdList.begin(); receiverIdListIter != receiverIdList.end(); ++receiverIdListIter)
      {
         memcpy( &recActivity[*totalReceivers], &(*receiverIdListIter), sizeof(ReceiverActivity));

         if(m_ReceiverIdMap.size() > 0)
         {
            ReceiverIdMap::iterator receiverIdMapIter = m_ReceiverIdMap.find(std::string((*receiverIdListIter).receiverId));

            if ( receiverIdMapIter != m_ReceiverIdMap.end() )
            {
               activity = m_ReceiverIdMap[std::string((*receiverIdListIter).receiverId)];

               strcpy( recActivity[*totalReceivers].activityState, activity.activityState.c_str());
               strcpy( recActivity[*totalReceivers].lastActivityTime, activity.lastActivityTime.c_str());
            }
         }
         (*totalReceivers)++;
      }
   }
   INFO("getTunerStatus %ld", *totalReceivers);
   pthread_mutex_unlock( &m_mutex );
   printTunerActivityStatusMap();
}

const char* RBIManager::curlUserAgent(void)
{
   return mUserAgent.c_str();
}

RBIContext::RBIContext()
  : m_refCount(0), m_sessionNumber(0), m_insertPending(false),
    m_inserting(false), m_tuneInEmitted(false), m_insertionStartedEmitted(false), m_insertionStatusUpdateEmitted(false), m_triggerId(0),
    m_newMonPending(false), m_primaryStreamProcessor(0), m_dfnSet(0),
    m_dfnCurrent(0), m_dfnNext(0),  m_spotIndex(0), m_spotCount(0), m_spotIsReplace(false), 
    m_utcTimeSpot(0LL), m_utcSpliceTime(0LL), m_prefetchPTS(-1LL), m_startPTS(-1LL), m_endPTS(-1LL),
    m_prefetchPTSNext(-1LL), m_startPTSNext(-1LL), m_timeReady(0LL), m_timeExpired(0LL), m_detailCode(0), m_detailCodeNext(0),
    m_insertSrc(0), m_insertSrcNext(0), m_insertDataCapacity(0), m_insertDataSize(0), m_insertData(0), m_privateDataPacketCount(0),
    m_inFile(0), m_inDataCapacity(0), m_inData(0), m_triggerDetectedForOpportunity(false)
{
   pthread_mutex_init( &m_mutex, 0 );
   memset( &m_mon, 0, sizeof(RBIPrivateDataMonitor) );
   memset( m_deviceId, 0, sizeof(m_deviceId) );
   RBIManager::getInstance()->getDeviceId( m_deviceId );
   if ( RBIManager::getInstance()->getCaptureEnabled() )
   {
      m_inData= (unsigned char*)malloc( 400*1024 );
      if ( m_inData )
      {
         m_inDataCapacity= 400*1024;
      }
   }
   int retryCount = RBIManager::getInstance()->getRetryCount();
   IARM_Bus_BroadcastEvent(RBI_RPC_NAME, RBI_EVENT_RETRYCOUNT_CHANGE, &retryCount, sizeof(retryCount));
}

RBIContext::~RBIContext()
{
   if ( m_inData )
   {
      free( m_inData );
   }
}

bool RBIContext::init()
{
   if ( !m_primaryStreamProcessor )
   {
      m_primaryStreamProcessor= new RBIStreamProcessor();
      if ( !m_primaryStreamProcessor )
      {
         return false;
      }
      m_primaryStreamProcessor->setTriggerEnable( true );
      m_primaryStreamProcessor->setPrivateDataEnable( true );
      m_primaryStreamProcessor->setFrameDetectEnable( false );
      m_primaryStreamProcessor->setTrackContinuityCountersEnable( true );
      m_primaryStreamProcessor->setEvents( this );
   }
   return true;
}

void RBIContext::term()
{
   termInsertionAll();
   if ( m_tuneInEmitted )
   {
      sendTuneOutEvent();
   }
   if ( m_primaryStreamProcessor )
   {
      delete m_primaryStreamProcessor;
      m_primaryStreamProcessor= 0;
   }
}

void RBIContext::acquire()
{
   ++m_refCount;
}

int RBIContext::release()
{
   if ( m_refCount > 0 )
   {
      --m_refCount;
   }
   return m_refCount;
}

void RBIContext::setSourceURI( const char *uri )
{
   DEBUG("RBIContext::setSourceURI: %s", uri );
   if ( m_tuneInEmitted )
   {
      sendTuneOutEvent();
   }
   m_sourceUri= std::string( uri );
   m_tuneInEmitted= false;
}

void RBIContext::setPrivateDataMonitor( RBIPrivateDataMonitor mon )
{
   m_mon= mon;
   {
      pthread_mutex_lock( &m_mutex );
      m_newMonPending= true;
      pthread_mutex_unlock( &m_mutex );
   }
}

// Returns FALSE if buffer should be discarded.  Updates size
// to indicate how much of the buffer should be accepted.
bool RBIContext::processPackets( unsigned char* packets, int* size )
{
   bool result= true;
   bool newMonPending;
   int acceptedSize;
   bool captureEnabled;
   int isTSFormatError = 0;
   
   if ( !packets )
   {
      result= m_inserting;
      return result;
   }
   
   captureEnabled= RBIManager::getInstance()->getCaptureEnabled();
   if ( captureEnabled )
   {
      // Save un-altered input data
      if ( *size < m_inDataCapacity )
      {
         memcpy( m_inData, packets, *size );
      }
   }
   
   if ( !m_tuneInEmitted )
   {
      sendTuneInEvent();
      m_tuneInEmitted= true;
   }
 
   updateInsertionState(-1);  

   /*
    * Update private data monitor if required
    */
   {
      pthread_mutex_lock( &m_mutex );
      newMonPending= m_newMonPending;
      m_newMonPending= false;
      pthread_mutex_unlock( &m_mutex );
      if ( newMonPending )
      {
         m_primaryStreamProcessor->setPrivateDataMonitor( m_mon );
      }
   }

   bool insertPendingPrior= m_insertPending;
   int inSize= *size;
   
   pthread_mutex_lock( &m_mutex );
   acceptedSize= m_primaryStreamProcessor->processPackets( packets, *size, &isTSFormatError );
   pthread_mutex_unlock( &m_mutex );
   if ( acceptedSize != *size )
   {
      *size= acceptedSize;
      if ( acceptedSize == 0 )
      {
         result= false;
      }
   }
   
   if ( ((!insertPendingPrior && m_insertPending) || m_inserting || !result) && captureEnabled )
   {
      if ( !m_inFile )
      {
         m_inFile= fopen("/opt/rbicapture-in.ts","wb");
      }
      if ( m_inFile && (inSize > 0) )
      {
         fwrite( m_inData, 1, inSize, m_inFile );
      }
   }
   else if ( m_inFile && result && !m_insertPending )
   {
      if ( m_inFile && (inSize > 0) )
      {
         fwrite( m_inData, 1, inSize, m_inFile );
      }
      fclose( m_inFile );
      m_inFile= 0;
   }
   
   return result;
}

int RBIContext::getInsertionDataSize()
{
   int size= 0;
   
   pthread_mutex_lock( &m_mutex );
   if ( m_inserting )
   {
      size= m_primaryStreamProcessor->getInsertionDataSize();
   }
   pthread_mutex_unlock( &m_mutex );

   return size;
}

int RBIContext::getInsertionData( unsigned char* packets, int size )
{
   int insertedSize= 0;
   
   pthread_mutex_lock( &m_mutex );
   if ( m_inserting )
   {
      insertedSize= m_primaryStreamProcessor->getInsertionData( packets, size );
   }
   pthread_mutex_unlock( &m_mutex );
   
   return insertedSize;
}

std::string RBIContext::getSourceUri(void)
{
   return m_sourceUri;
}

bool RBIContext::getTriggerDetectedForOpportunity(void)
{
   return m_triggerDetectedForOpportunity;
}

void RBIContext::setTriggerDetectedForOpportunity(bool bTriggerDetectedForOpportunity)
{
   m_triggerDetectedForOpportunity = bTriggerDetectedForOpportunity;
   INFO("setTriggerDetectedForOpportunity %d RBIContext (%p)", m_triggerDetectedForOpportunity, this);
}

void RBIContext::updateInsertionState(long long startPTS)
{
   if ( m_insertPending )
   {
      int numPending= RBIManager::getInstance()->ageInsertOpportunities( m_sessionIdStr );
      if ( (numPending == 0) && (m_dfnSet == 0) )
      {
         /* Drop out of pending if no insertion is in progress and no more opportunities are pending */
         m_insertPending= false;
         m_detailCode = RBI_DetailCode_responseTimeout;
         sendInsertionStatusEvent( RBI_SpotEvent_insertionFailed );

         /* Discard any existing definition set (DELIA-18339) */
         RBIManager::getInstance()->unregisterInsertDefinitionSet( m_sessionIdStr );
         this->setTriggerDetectedForOpportunity(false);
      }
      if ( m_insertPending )
      {
         RBIManager *rbm= RBIManager::getInstance();
         
         if ( !m_dfnSet )
         {
            RBIInsertDefinitionSet *dfnSet= rbm->getInsertDefinitionSet( m_sessionIdStr );
            if ( dfnSet )
            {
               DEBUG( "RBIContext::updateInsertionState: got defnSet %p with %d spot(s)", dfnSet, dfnSet->getNumSpots() );
               m_dfnSet= dfnSet;            
               rbm->unregisterInsertOpportunity( m_sessionIdStr );
               m_spotIndex= 0;
               m_spotCount= m_dfnSet->getNumSpots();
            }
         }
         if ( m_dfnSet )
         {
            if ( !m_dfnCurrent )
            {
               m_dfnNext= 0;
               m_prefetchPTSNext= -1LL;
               m_dfnCurrent= m_dfnSet->getSpot( m_spotIndex );
               DEBUG("RBIContext::updateInsertionState: spot %d got defn %p", m_spotIndex, m_dfnCurrent );
               if ( m_dfnCurrent )
               {
                  const SpotTimeInfo *ti= m_dfnSet->getSpotTime( m_spotIndex );
                  m_utcTimeSpot= ti->utcTimeStart;                  
                    if(startPTS != -1)
                    {
                        m_startPTS= startPTS; 
                        INFO( "Adjusting Out point PTS for this spot to match that in the stream, estimated PTS %llX Adjusted PTS %llX ", ti->startPTS, m_startPTS );
                    }
                    else
                    {
                        m_startPTS= ti->startPTS;
                    }
                  m_prefetchPTS= ((m_startPTS - 4000*90LL)&MAX_90K);
                  m_endPTS= ((m_startPTS + (m_dfnCurrent->getDuration()*90LL))&MAX_90K);
                  m_spotIsReplace= m_dfnCurrent->isReplace();
                  m_primaryStreamProcessor->setBackToBack(false);
                  m_primaryStreamProcessor->setFrameDetectEnable(true);
                  INFO("RBIContext::updateInsertionState: spot %d startPTS %llx endPTS %llx isReplace %d",
                        m_spotIndex, m_startPTS, m_endPTS, m_spotIsReplace );
               }
               if ( m_spotIndex+1 < m_spotCount )
               {
                  m_dfnNext= m_dfnSet->getSpot( m_spotIndex+1 );
                  if ( m_dfnNext )
                  {
                     if(m_dfnNext->isSpotValid() == RBI_Spot_valid)
                     {
                        const SpotTimeInfo *tiNext= m_dfnSet->getSpotTime( m_spotIndex+1 );
                        if ( m_dfnNext->isReplace() )
                        {
                           if ( m_spotIsReplace )
                           {
                              m_primaryStreamProcessor->setBackToBack(true);
                              INFO("m_backToBack is set to TRUE for %s spot %d", (m_spotIsReplace == true) ? "Replace" : "Fixed", m_spotIndex);
                           }
                           m_startPTSNext= tiNext->startPTS;
                           m_prefetchPTSNext= ((tiNext->startPTS - 4000*90LL)&MAX_90K);
                           INFO("RBIContext::updateInsertionState: spot %d startPTS %llx prefetchPTS %llx isReplace 1",
                              m_spotIndex+1, m_startPTSNext, m_prefetchPTSNext );
                        }
                        else
                        {
                           m_prefetchPTSNext= -1LL;
                           INFO("RBIContext::updateInsertionState: spot %d startPTS %llx isReplace 0",
                              m_spotIndex+1, tiNext->startPTS );
                        }
                     }
                  }
               }
            }
            if ( m_dfnCurrent && m_spotIsReplace && !m_insertSrc )
            {
               bool start= true;
               
               if ( !m_insertSrc && !m_insertSrcNext )
               {
                  // Start http asset fetch for first spot when at the prefetch time.
                  // The fetches for subsequent spots will also start at appropriate
                  // times (see m_prefetchPTSNext).
                  long long currentPTS= m_primaryStreamProcessor->getCurrentPTS();
                  if ( PTS_IN_RANGE( currentPTS, m_prefetchPTS, m_startPTS ) )
                  {
                     start= true;
                  }
                  else
                  {
                     start= false;
                  }
               }
               if(m_dfnCurrent->isSpotValid() == RBI_Spot_invalid)
               {
                  start= false;
                  if (m_dfnCurrent->getSpotDetailCode() != 0)
                  {
                     insertionError( m_primaryStreamProcessor, m_dfnCurrent->getSpotDetailCode());
                  }
               }
               else if ((m_dfnCurrent->isSpotValid() == RBI_Spot_valid) && (m_dfnCurrent->getSpotDetailCode() == RBI_DetailCode_PRSPAudioAssetNotFound))
               {
                  if (m_insertionStatusUpdateEmitted !=true)
                  {
                     INFO("RBIContext::ERROR %d\n",m_dfnCurrent->getSpotDetailCode());
                     insertionStatusUpdate( m_primaryStreamProcessor, m_dfnCurrent->getSpotDetailCode());
                  }
               }
               if ( start )
               {
                  RBIInsertSource *src;
                  if ( m_insertSrcNext )
                  {
                     src= m_insertSrcNext;
                     m_insertSrcNext= 0;
                     m_detailCode= m_detailCodeNext;
                     m_detailCodeNext= 0;
                  }
                  else
                  {
                     src= m_dfnCurrent->getInsertSource();
                  }
                  if ( src )
                  {
                     m_insertSrc= src;
                     src->setEvents( this );
                  }
                  else
                  {
                     ERROR("Unable to create insertSrc");
                  }
                  if ((m_detailCode != 0) &&
                      (m_detailCode != RBI_DetailCode_assetDurationInadequate) &&
                      (m_detailCode != RBI_DetailCode_abnormalTermination) &&
                      (m_detailCode != RBI_DetailCode_assetDurationExceeded))
                  {
                     termInsertionSpot();
                  }
               }
               else
               {
                  if(m_dfnCurrent->isSpotValid() == RBI_Spot_invalid)
                  {
                     if (m_dfnCurrent->getSpotDetailCode() != 0)
                     {
                        insertionError( m_primaryStreamProcessor, m_dfnCurrent->getSpotDetailCode());
                     }
                  }
                  else if(m_dfnCurrent->isSpotValid() == RBI_Spot_valid)
                  {
                     // Check if currentPTS is already past startPTS
                     long long currentPTS= m_primaryStreamProcessor->getCurrentPTS();
                     long long abortPTS= ((m_startPTS + 10000LL*90LL) & MAX_90K);
                     if ( PTS_IN_RANGE( currentPTS, m_startPTS, abortPTS ) )
                     {
                        long long leadTime= 0;
                        leadTime= (currentPTS - m_startPTS)&MAX_90K;
                        m_timeReady = (m_utcTimeSpot+(leadTime/90LL));
                        m_timeExpired = m_utcTimeSpot;
                        ERROR("RBIContext::updateInsertionState: missed prefetch window: spot %d prefetchPTS %lld startPTS %lld currentPTS %lld", m_spotIndex, m_prefetchPTS, m_startPTS, currentPTS );
                        insertionError( m_primaryStreamProcessor,RBI_DetailCode_unableToSplice_missed_prefetchWindow );
                        m_primaryStreamProcessor->stopInsertion();
                     }
                  }
                  else
                  {
                     m_detailCode = RBI_DetailCode_invalidSpot;
                     termInsertionSpot();
                  }
               }
            }
         }
         if ( m_insertSrc )
         {
            if ( m_insertSrc->dataReady() )
            {
               m_insertPending= false;

                if ( m_primaryStreamProcessor->startInsertion( m_insertSrc, m_startPTS, m_endPTS ) )
                {
                   m_inserting= true;
                   test_insert= true;
                   INFO("test_insert is set to true %d", __LINE__ );
                }
                else
                {
                   rbm->releaseInsertSource(m_insertSrc);
                   m_insertSrc= 0;
                }
            }
#ifdef ENABLE_HTTP_MISSED_PREFETCH
            else
            {
               // Check if currentPTS is already past startPTS
               long long currentPTS= m_primaryStreamProcessor->getCurrentPTS();
               long long abortPTS= ((m_startPTS + 10000LL*90LL) & MAX_90K);
               if ( PTS_IN_RANGE( currentPTS, m_startPTS, abortPTS ) )
               {
                  long long leadTime= 0;
                  leadTime= (currentPTS - m_startPTS)&MAX_90K;
                  m_timeReady = (m_utcTimeSpot+(leadTime/90LL));
                  m_timeExpired = m_utcTimeSpot;
                  ERROR("RBIContext::updateInsertionState: HTTP response missed prefetch window: spot %d prefetchPTS %lld startPTS %lld currentPTS %lld", m_spotIndex, m_prefetchPTS, m_startPTS, currentPTS );
                  insertionError( m_primaryStreamProcessor,RBI_DetailCode_unableToSplice_missed_prefetchWindow );
                  m_primaryStreamProcessor->stopInsertion();
                  rbm->releaseInsertSource(m_insertSrc);
                  m_insertSrc= 0;
               }
            }
#endif
         }
         else if ( m_dfnCurrent && !m_spotIsReplace )
         {
            if(m_dfnCurrent->isSpotValid() == RBI_Spot_invalid)
            {
               INFO("RBIContext::updateInsertionState: spot %d: fixed: isSpotValid= %d getSpotDetailCode()=%d", m_spotIndex, m_dfnCurrent->isSpotValid(),m_dfnCurrent->getSpotDetailCode());
               if (m_dfnCurrent->getSpotDetailCode() != 0)
               {
                  insertionError( m_primaryStreamProcessor, m_dfnCurrent->getSpotDetailCode());
               }
            }
            else
            {
               m_insertPending= false;
               INFO("RBIContext::updateInsertionState: spot %d: fixed: startPTS %llx endPTS %llx", m_spotIndex, m_startPTS, m_endPTS );
               if ( m_primaryStreamProcessor->startInsertion( 0, m_startPTS, m_endPTS ) )
               {
                  m_inserting= true;
                  test_insert= true;
                  INFO("test_insert is set to true %d", __LINE__ );
               }
            }
         }
      }
   }
   if ((m_detailCode != 0) &&
       (m_detailCode != RBI_DetailCode_assetDurationInadequate) &&
       (m_detailCode != RBI_DetailCode_abnormalTermination) &&
       (m_detailCode != RBI_DetailCode_assetDurationExceeded))
   {
      termInsertionSpot();
   }
   if ( m_prefetchPTSNext != -1LL )
   {
      long long currentPTS= m_primaryStreamProcessor->getCurrentPTS();
      
      if ( PTS_IN_RANGE( currentPTS, m_prefetchPTSNext, m_startPTSNext ) )
      {
         RBIInsertSource *src = 0;
         if(m_dfnNext->isSpotValid())
         {
            src= m_dfnNext->getInsertSource();
         }

         if ( src )
         {
            m_insertSrcNext= src;
            src->setEvents( this );
         }
         else
         {
            ERROR("Unable to create insertSrcNext");
         }
         m_prefetchPTSNext= -1LL;
      }      
   }
}

bool RBIContext::triggerDetected( RBIStreamProcessor *sp, RBITrigger* trigger )
{
   bool retVal = false;
   assert( sp == m_primaryStreamProcessor );
   
   if ( trigger )
   {
      INFO( "RBIContext::triggerDetected: id %X", trigger->getUnifiedId() );
      m_utcSpliceTime = trigger->getUTCSpliceTime();
      
      if ( !m_inserting && !m_insertPending )
      {
         int rc= RBIManager::getInstance()->registerInsertOpportunity( m_sessionIdStr,
                                                                       trigger->getUTCTriggerTime(),
                                                                       trigger->getUTCSpliceTime(),
                                                                       trigger->getSplicePTS() );
         if ( rc == 0 )
         {
            sendInsertionOpportunity(trigger);
     
            m_insertPending= true;
            m_triggerId= trigger->getUnifiedId();
            
            if ( trigger->getSplicePTS() == -1L )
            {
               // If no trigger PTS is available, treat as immediate
               ERROR("Immediate triggers are not supported");
            }
            retVal = true;
         }
         else if (rc == RBIResult_spliceTimeout)
         {
            ERROR("Unable to register insert opportunity: Insertion point time delta exceeded. utcTrigger %lld utcSplice %lld leadTime %lld\n", trigger->getUTCTriggerTime(), trigger->getUTCSpliceTime(), ((trigger->getUTCSpliceTime()) - (trigger->getUTCTriggerTime())));
            insertionStatusUpdate( m_primaryStreamProcessor,RBI_DetailCode_insertionTimeDeltaExceeded );
         }
         else
         {
            ERROR("Unable to register insert opportunity: session %d, trigger %X utcSplice %lld", 
                   m_sessionNumber, trigger->getUnifiedId(), trigger->getUTCSpliceTime() );
         }
      }
   }
   return retVal;
}

void RBIContext::insertionStatusUpdate( RBIStreamProcessor *sp, int detailCode )
{
   TRACE3( "RBIContext::insertionStatusUpdate \n");
   m_detailCode= detailCode;
   if((detailCode != RBI_DetailCode_assetDurationInadequate) && 
      (detailCode != RBI_DetailCode_abnormalTermination) && 
      (detailCode != RBI_DetailCode_assetDurationExceeded))
   {
      sendInsertionStatusEvent( RBI_SpotEvent_insertionStatusUpdate );
      m_insertionStatusUpdateEmitted= true;
   }
}

void RBIContext::insertionStarted( RBIStreamProcessor *sp )
{
   const RBIInsertDefinition *dfnCur = m_dfnSet->getSpot( m_spotIndex );
   INFO( "RBIContext::insertionStarted for spot %d Url %s", m_spotIndex,  dfnCur->getUri());

   sendInsertionStatusEvent( RBI_SpotEvent_insertionStarted );
   m_insertionStartedEmitted= true;
}

void RBIContext::insertionStopped( RBIStreamProcessor *sp )
{
   sp->setInsertionEndTime(getCurrentTimeMicro());
   long long elapsed= (sp->getInsertionEndTime() - sp->getInsertionStartTime())/1000LL;
   INFO("insertionStopped: endTime (%lld) startTime (%lld) elapsed %lld (ms)\n",
	   sp->getInsertionEndTime(), sp->getInsertionStartTime(), elapsed);

   if (m_dfnSet)
   {
      const RBIInsertDefinition *dfnCur = m_dfnSet->getSpot( m_spotIndex );
      INFO( "RBIContext::insertionStopped: m_currentPTS %llx for spot %d Url %s", m_primaryStreamProcessor->getCurrentPTS(), m_spotIndex,  dfnCur->getUri() );
   }
   else
   {
      ERROR( "RBIContext::insertionStopped: m_dfnSet is NULL m_spotIndex = %d\n", m_spotIndex );
   }
   RBIManager *rbm= RBIManager::getInstance();
   if (( m_detailCode == 0 ) ||
       (m_detailCode == RBI_DetailCode_assetDurationInadequate) ||
       (m_detailCode == RBI_DetailCode_abnormalTermination) ||
       (m_detailCode == RBI_DetailCode_assetDurationExceeded) ||
       (m_detailCode == RBI_DetailCode_invalidSpot) )
   {
      if(m_detailCode != RBI_DetailCode_invalidSpot)
         INFO("RBI LSA-SS : Insertion Success for %s spot %d", (m_spotIsReplace == true) ? "Replace" : "Fixed", m_spotIndex);

      if ( m_insertionStartedEmitted )
      {
         m_insertionStartedEmitted= false;
         sendInsertionStatusEvent( RBI_SpotEvent_insertionStopped );         
      }
   }
   else
   {
      sendInsertionStatusEvent( RBI_SpotEvent_insertionFailed );
   }
   if ( m_insertSrc )
   {
      rbm->releaseInsertSource( m_insertSrc );
      m_insertSrc= 0;
   }
   m_inserting= false;
   test_insert= false;
   INFO("test_insert is set to false");
   m_insertPending= false;
   m_detailCode= 0;
   if ( m_dfnSet )
   {
      ++m_spotIndex;
      m_dfnCurrent= 0;
      if ( m_spotIndex < m_spotCount )
      {
         m_insertPending= true;
         updateInsertionState(m_primaryStreamProcessor->getSpliceUpdatedOutPTS());
      }
      else
      {
         rbm->releaseInsertDefinitionSet( m_sessionIdStr );
         rbm->unregisterInsertDefinitionSet( m_sessionIdStr );
         this->setTriggerDetectedForOpportunity(false);
         m_dfnSet= 0;
         INFO( "RBIContext::insertionStopped: All breaks in this spot done. m_spotCount = %d\n", m_spotCount );
         sendInsertionStatusEvent( RBI_SpotEvent_endAll );
      }
   }
}

void RBIContext::insertionError( RBIStreamProcessor *sp, int detailCode )
{
   ERROR( "RBIContext::insertionError: detailCode %X", detailCode );
   m_detailCode= detailCode;
}

void RBIContext::privateData( RBIStreamProcessor *sp, int streamType, unsigned char *packet, int len )
{
   RBI_PrivateData_Data privateData;
   memcpy( privateData.sessionId, m_sessionId, sizeof(m_sessionId) );
   memcpy( privateData.deviceId, m_deviceId, sizeof(m_deviceId) );
   privateData.streamType= streamType;
   memcpy( privateData.packet, packet, len);
   IARM_Bus_BroadcastEvent(RBI_RPC_NAME, RBI_EVENT_PRIVATE_DATA, &privateData, sizeof(privateData));
   if ( (m_privateDataPacketCount % 100) == 0 )
   {
      INFO("RBIContext: private data: session: %s packet count: %d", m_sessionIdStr.c_str(), m_privateDataPacketCount+1 );
   }
   ++m_privateDataPacketCount;
}

void RBIContext::httpTimeoutError( RBIInsertSource *src )
{
   if ( m_inserting || m_insertPending )
   {
      if ( src == m_insertSrc )
      {
         m_detailCode= RBI_DetailCode_httpResponseTimeout;
      }
      else
      {
         m_detailCodeNext= RBI_DetailCode_httpResponseTimeout;
      }
   }
   else
   {
      ERROR( "RBIContext::httpTimeoutError: session: %d, %s unexpected source error", m_sessionNumber, m_sessionIdStr.c_str() );
   }
}

void RBIContext::tsFormatError( RBIInsertSource *src )
{
   if ( m_inserting || m_insertPending )
   {
      if ( src == m_insertSrc )
      {
         m_detailCode= RBI_DetailCode_assetCorrupted;
      }
      else
      {
         m_detailCodeNext= RBI_DetailCode_assetCorrupted;
      }
   }
   else
   {
      ERROR( "RBIContext::tsFormatErrorror: session: %d, %s unexpected source error", m_sessionNumber, m_sessionIdStr.c_str() );
   }
}

void RBIContext::sourceError( RBIInsertSource *src )
{
   if ( m_inserting || m_insertPending )
   {
      if ( src == m_insertSrc )
      {
         m_detailCode= RBI_DetailCode_assetUnreachable;
      }
      else
      {
         m_detailCodeNext= RBI_DetailCode_assetUnreachable;
      }
   }
   else
   {
      ERROR( "RBIContext::sourceError: session: %d, %s unexpected source error", m_sessionNumber, m_sessionIdStr.c_str() );
   }
   INFO( "RBIContext::sourceError: m_detailCode= %d\n",m_detailCode );
}

void RBIContext::termInsertionSpot()
{
   if ( m_inserting )
   {
      INFO( "RBIContext::termInsertion: session: %d, %s ending insertion", m_sessionNumber, m_sessionIdStr.c_str() );
      m_primaryStreamProcessor->stopInsertion();
   }
   else if ( m_insertPending )
   {
      INFO( "RBIContext::termInsertion: session: %d, %s aborting insertion", m_sessionNumber, m_sessionIdStr.c_str() );
      insertionStopped(m_primaryStreamProcessor);
   }
}

void RBIContext::termInsertionAll()
{
   m_spotCount=0;
   termInsertionSpot();
   if ( m_insertSrc )
   {
      RBIManager::getInstance()->releaseInsertSource( m_insertSrc );
      m_insertSrc= 0;
   }
   if ( m_insertSrcNext )
   {
      RBIManager::getInstance()->releaseInsertSource( m_insertSrcNext );
      m_insertSrcNext= 0;
   }
   m_prefetchPTSNext= -1LL;
   m_dfnCurrent= 0;
   m_dfnNext= 0;
   m_dfnSet= 0;
}

void RBIContext::sendTuneInEvent()
{
   RBI_TuneIn_Data tuneIn;
   struct timeval tv;
   struct timezone tz;
   
   RBIManager::getInstance()->setTimeZoneName();

   gettimeofday( &tv, &tz );
   
   memcpy( tuneIn.sessionId, m_sessionId, sizeof(m_sessionId) );
   tuneIn.utcTime= UTCMILLIS(tv);
   memcpy( tuneIn.deviceId, m_deviceId, sizeof(m_deviceId) );
   memset(tuneIn.serviceURI, 0, sizeof(tuneIn.serviceURI));
   strncpy( tuneIn.serviceURI, m_sourceUri.c_str(), sizeof(tuneIn.serviceURI)-1);
   IARM_Bus_BroadcastEvent(RBI_RPC_NAME, RBI_EVENT_TUNEIN, &tuneIn, sizeof(tuneIn));
}

void RBIContext::sendTuneOutEvent()
{
   RBI_TuneOut_Data tuneOut;
   struct timeval tv;
   struct timezone tz;
   
   gettimeofday( &tv, &tz );
   
   memcpy( tuneOut.sessionId, m_sessionId, sizeof(m_sessionId) );
   tuneOut.utcTime= UTCMILLIS(tv);
   memcpy( tuneOut.deviceId, m_deviceId, sizeof(m_deviceId) );
   memset(tuneOut.serviceURI, 0, sizeof(tuneOut.serviceURI));
   strncpy( tuneOut.serviceURI, m_sourceUri.c_str(), sizeof(tuneOut.serviceURI)-1);
   IARM_Bus_BroadcastEvent(RBI_RPC_NAME, RBI_EVENT_TUNEOUT, &tuneOut, sizeof(tuneOut));
   
   RBIManager::getInstance()->unregisterInsertOpportunity( m_sessionIdStr );
   RBIManager::getInstance()->unregisterInsertDefinitionSet( m_sessionIdStr );
   this->setTriggerDetectedForOpportunity(false);
}

void RBIContext::sendInsertionOpportunity( RBITrigger *trigger )
{
   IARM_Result_t result;
   RBI_InsertionOpportunity_Data opportunity;
   struct timeval tv;
   struct timezone tz;
   char dateTime[256];

   RBIManager::getInstance()->setTimeZoneName();
   gettimeofday( &tv, &tz );
   memset(&opportunity, 0, sizeof(opportunity));
   memcpy( opportunity.sessionId, m_sessionId, sizeof(m_sessionId) );
   memcpy( opportunity.deviceId, m_deviceId, sizeof(m_deviceId) );
   opportunity.triggerId= trigger->getUnifiedId();
   memset(opportunity.serviceURI, 0, sizeof(opportunity.serviceURI));
   strncpy( opportunity.serviceURI, m_sourceUri.c_str(), sizeof(opportunity.serviceURI)-1);
   opportunity.utcTimeCurrent= UTCMILLIS(tv);
   opportunity.utcTimeOpportunity= trigger->getUTCSpliceTime();
   RBIManager::getInstance()->getTunerStatus(m_sourceUri.c_str(), opportunity.recActivity, &opportunity.totalReceivers );

   memcpy(&opportunity.poData, trigger->poData(), sizeof(opportunity.poData));
   memset(dateTime, 0, sizeof(dateTime));
   formatTime(opportunity.utcTimeCurrent, dateTime);
   INFO("SCTE-35: sending insertion streamId %s signal id %s poType %d poDuration %d utcTimeCurrent[%llu]",
      opportunity.poData.streamId, opportunity.poData.signalId, opportunity.poData.poType, opportunity.poData.poDuration, opportunity.utcTimeCurrent );

   result= IARM_Bus_BroadcastEvent(RBI_RPC_NAME, RBI_EVENT_INSERTION_OPPORTUNITY, &opportunity, sizeof(opportunity));
   INFO( "RBIContext::sendInsertionOpportunity: emit RBI_EVENT_INSERTION_OPPORTUNITY (%d) result %d", RBI_EVENT_INSERTION_OPPORTUNITY, result );
}

void RBIContext::sendInsertionStatusEvent( int event )
{
   IARM_Result_t result;
   RBI_InsertionStatus_Data status;   
   struct timeval tv;
   struct timezone tz; 
   char dateTime[256];
   std::string detailCodeString = "";

   gettimeofday( &tv, &tz );
   
   memset( &status, 0, sizeof(RBI_InsertionStatus_Data) );
   memcpy( status.sessionId, m_sessionId, sizeof(m_sessionId) );
   memcpy( status.deviceId, m_deviceId, sizeof(m_deviceId) );
   status.triggerId= m_triggerId;
   if ( m_dfnCurrent )
   {
      const char *trackingString= m_dfnCurrent->getTrackingIdString();
      if( trackingString )
      {
         strncpy( status.trackingIdString, trackingString, strlen( trackingString ) );
      }
   }
   switch( event )
   {
      case RBI_SpotEvent_insertionStarted:
      case RBI_SpotEvent_insertionStatusUpdate:
      case RBI_SpotEvent_insertionStopped:
         break;
      case RBI_SpotEvent_insertionFailed:
         INFO("RBI LSA-P: Insertion Failure for spot %d", m_spotIndex);
         break;
      case RBI_SpotEvent_endAll:
         break;
      default:
         event= RBI_SpotEvent_unknown;
         break;
   }
   status.event= (RBI_SpotEvent)event;
   if ( m_dfnCurrent )
   {
      status.action= m_dfnCurrent->isReplace() ? RBI_SpotAction_replace : RBI_SpotAction_fixed;
   }
   status.utcTimeSpot= m_utcTimeSpot;

   long long insertionEndTime = m_primaryStreamProcessor->getInsertionEndTime();
   if ( event == RBI_SpotEvent_insertionStopped || (event == RBI_SpotEvent_endAll))
   {
      status.utcTimeEvent= m_utcTimeSpot + (long long)(UTCMILLIS(tv)-m_utcTimeSpot);
      if ( m_dfnCurrent )
          status.spotDuration = m_dfnCurrent->getDuration();

      status.spotNPT = (insertionEndTime - m_primaryStreamProcessor->getInsertionStartTime())/1000;
   }
   else
   {
      status.utcTimeEvent= UTCMILLIS(tv);
      status.spotNPT = 0;
      insertionEndTime = m_primaryStreamProcessor->getInsertionStartTime();
   }
   memset(dateTime, 0, sizeof(dateTime));
   formatTime(status.utcTimeEvent, dateTime);
   INFO("sendInsertionStatusEvent m_spotIndex[%d] event[%d] utcTimeEvent[%llu] [%s] UTCMILLIS(tv)[%llu] utcTimeSpot[%llu] spotDuration[%d] status.spotNPT= [%d] startTime[%lld] endTime[%lld]\n",
       m_spotIndex, event, status.utcTimeEvent, dateTime, UTCMILLIS(tv), status.utcTimeSpot, status.spotDuration, status.spotNPT,  m_primaryStreamProcessor->getInsertionStartTime(), insertionEndTime);

   status.detailCode= m_detailCode;
   status.scale= 1.0;
   if ((status.action == RBI_SpotAction_replace ) || (status.action == RBI_SpotAction_fixed))
   {   
      const char *str= m_dfnCurrent->getAssetID();
      if ( str )
      {
         int len= strlen(str);
         if ( len > RBI_MAX_ASSETID_LEN )
         {
            len= RBI_MAX_ASSETID_LEN;
         }
         strncpy( status.assetID, str, len );
      }

      str= m_dfnCurrent->getProviderID();
      if ( str )
      {
         int len= strlen(str);
         if ( len > RBI_MAX_PROVIDERID_LEN )
         {
            len= RBI_MAX_PROVIDERID_LEN;
         }
         strncpy( status.providerID, str, len );
      }
      
      str= m_dfnCurrent->getUri();
      if ( str )
      {
         int len= strlen(str);
         if ( len > RBI_MAX_CONTENT_URI_LEN )
         {
            len= RBI_MAX_CONTENT_URI_LEN;
         }
         strncpy( status.uri, str, len );
      }
   }
   switch( m_detailCode )
   {
      case RBI_DetailCode_missingMediaType:
      {
         detailCodeString  = "Error in ContentLocations -mediaType attributes missing.";
         break;
      }

      case RBI_DetailCode_PRSPAudioAssetNotFound:
      {
         detailCodeString  = "Asset of matching audio type cannot be found. Support types of AC-3.";
         break;
      }

      case RBI_DetailCode_matchingAssetNotFound:
      {
         detailCodeString  = "Asset of matching type cannot be found. Supports only MPEG4 type";
         break;
      }

      case RBI_DetailCode_missingContentLocation:
      {
         detailCodeString  = "Asset has no ContentLocation.";
         break;
      }

      case RBI_DetailCode_missingContent:
      {
         detailCodeString  = "Missing PlacementElement Content.";
         break;
      }

      case RBI_DetailCode_assetUnreachable:
      {
         detailCodeString  = "Asset located at ";
         detailCodeString.append(status.uri);
         detailCodeString.append(" could not be accessed");
         detailCodeString = detailCodeString + " reason=" + '\"';

         size_t  bytesReq = b64_encoded_size(strlen(curlErrorCode) + 1);
         uint8_t *base64DataPtr = (uint8_t*)calloc(bytesReq+1, sizeof(uint8_t));
         if(base64DataPtr)
         {
            base64_encode((const  uint8_t*) curlErrorCode, strlen(curlErrorCode), base64DataPtr);
            base64DataPtr[bytesReq] = '\0';
            detailCodeString = detailCodeString + (char*)base64DataPtr;

            free(base64DataPtr);
            base64DataPtr = NULL;
         }
         detailCodeString = detailCodeString + '\"';
         ERROR("RBI LSA-SE : Asset Unreachable for %s spot %d", (m_spotIsReplace == true) ? "Replace" : "Fixed", m_spotIndex);
         break;
      }

      case RBI_DetailCode_patPmtChanged:
      {
         detailCodeString  = "Stream components are not minimally compatible. PAT/PMT update found";
         ERROR("RBI LSA-SE : Stream components are not minimally compatible. PAT/PMT update found for %s spot %d", (m_spotIsReplace == true) ? "Replace" : "Fixed", m_spotIndex);
         break;
      }

      case RBI_DetailCode_assetCorrupted:
      {
         detailCodeString  = "Asset located at ";
         detailCodeString.append(status.uri);
         detailCodeString.append(" is corrupt or of unknown format");
         ERROR("RBI LSA-SE : Asset Corrupted for %s spot %d", (m_spotIsReplace == true) ? "Replace" : "Fixed", m_spotIndex);
         break;
      }

      case RBI_DetailCode_insertionTimeDeltaExceeded:
      {
         char work[256];
         char buffer[256];
         int serviceid;
         char *timeSignalbase64Msg = m_primaryStreamProcessor->getTimeSignalBase64Msg();

         detailCodeString  = "Insertion point time delta exceeded: service_id=";
         if ( sscanf( m_sourceUri.c_str(), "ocap://0x%X", &serviceid ) == 1 )
         {
             sprintf(work,"urn:cibo:sourceid:%d",serviceid);
         }
         detailCodeString = detailCodeString + '\"' + work + '\"';
         detailCodeString.append(" current_timestamp=");
         long long utcTimeCur = UTCMILLIS(tv);
         if (formatTime(utcTimeCur, buffer))
         {
            detailCodeString = detailCodeString + buffer;
         }
         detailCodeString.append(" insertion_timestamp=");
         if (formatTime(m_utcSpliceTime, buffer))
         {
            detailCodeString = detailCodeString + buffer;
         }
         sprintf(buffer," PCR=%llu", m_primaryStreamProcessor->getCurrentPCR());
         detailCodeString.append(buffer);
         detailCodeString.append(" SCTE35=\"");
         if(timeSignalbase64Msg)
         {
            detailCodeString.append(timeSignalbase64Msg);
            free(timeSignalbase64Msg);
            timeSignalbase64Msg = NULL;
         }
         else
         {
            ERROR("timeSignalbase64Msg is NULL");
         }
         detailCodeString.append("\"");
         break;
      }

      case RBI_DetailCode_httpResponseTimeout:
      {
         detailCodeString  = "Timeout on asset fetch.";
         detailCodeString = detailCodeString + " ContentURL=" + '\"';

         size_t  bytesReq = b64_encoded_size(strlen(status.uri) + 1);
         uint8_t *base64DataPtr = (uint8_t*)calloc(bytesReq+1, sizeof(uint8_t));
         if(base64DataPtr)
         {
            base64_encode((const  uint8_t*) status.uri, strlen(status.uri), base64DataPtr);
            base64DataPtr[bytesReq] = '\0';
            detailCodeString = detailCodeString + (char*)base64DataPtr;

            free(base64DataPtr);
            base64DataPtr = NULL;
         }
         detailCodeString = detailCodeString + '\"' + " reason=" + '\"';

         bytesReq = b64_encoded_size(strlen(curlErrorCode) + 1);
         base64DataPtr = (uint8_t*)calloc(bytesReq+1, sizeof(uint8_t));
         if(base64DataPtr)
         {
            base64_encode((const  uint8_t*) curlErrorCode, strlen(curlErrorCode), base64DataPtr);
            base64DataPtr[bytesReq] = '\0';
            detailCodeString = detailCodeString + (char*)base64DataPtr;

            free(base64DataPtr);
            base64DataPtr = NULL;
         }
         detailCodeString = detailCodeString + '\"';
         ERROR("RBI LSA-SE : HTTP Response Timeout for %s spot %d\n", (m_spotIsReplace == true) ? "Replace" : "Fixed", m_spotIndex);
         break;
      }

      case RBI_DetailCode_responseTimeout:
      {
         ERROR("RBI LSA-E : Status Event Response Timeout");
         break;
      }

      case RBI_DetailCode_matchingAudioAssetNotFound:
      {
         detailCodeString  = "Asset of matching audio type cannot be found. Support types of AC-3.";
         ERROR("RBI LSA-E : Status Event Matching Audio Asset Not Found for spot %d", m_spotIndex);
         break;
      }

      case RBI_DetailCode_assetDurationExceeded:
      {
         detailCodeString  = "Asset duration exceeded.";
         ERROR("RBI LSA-E : Status Event Asset duration exceeded for spot %d", m_spotIndex);
         break;
      }

      case RBI_DetailCode_assetDurationInadequate:
      {
         detailCodeString  = "Actual asset duration less than duration in PlacementContent.";
         ERROR("RBI LSA-E : Status Event Actual asset duration less than duration in PlacementContent for spot %d", m_spotIndex);
         break;
      }
      case RBI_DetailCode_unableToSplice_missed_prefetchWindow:
      {
         char timeString[256];
         memset(timeString, 0, sizeof(timeString));
         detailCodeString  = "Placement received too late - ";
         detailCodeString = detailCodeString + "reason= " + '\"' + "RBI_DetailCode_unableToSplice-missed prefetch window" + '\"';
         if (formatTime(m_timeExpired, timeString))
         {
            detailCodeString = detailCodeString + ", expired=" + '\"' + timeString + '\"';
         }
         if (formatTime(m_timeReady,timeString))
         {
            detailCodeString = detailCodeString + ", ready=" + '\"' + timeString + '\"';
         }
         ERROR("RBI LSA-SE : Missed Prefetch Window for %s spot %d", (m_spotIsReplace == true) ? "Replace" : "Fixed", m_spotIndex);
         break;
      }
      case RBI_DetailCode_unableToSplice_missed_spotWindow:
      {
         char timeString[256];
         memset(timeString, 0, sizeof(timeString));
         long long leadTime = m_primaryStreamProcessor->getLeadTime();
         m_timeReady = (m_utcTimeSpot+leadTime);
         m_timeExpired = m_utcTimeSpot;
         ERROR( "RBI m_utcTimeSpot= %lld m_timeReady= %lld m_timeExpired= %lld \n",m_utcTimeSpot,m_timeReady,m_timeExpired );
         detailCodeString  = "Placement received too late - ";
         detailCodeString = detailCodeString + "reason= " + '\"' + "RBI_DetailCode_unableToSplice_missed_spotWindow" + '\"';
         if (formatTime(m_timeExpired, timeString))
         {
            detailCodeString = detailCodeString + ", expired=" + '\"' + timeString + '\"';
         }
         if (formatTime(m_timeReady,timeString))
         {
            detailCodeString = detailCodeString + ", ready=" + '\"' + timeString + '\"';
         }
         ERROR("RBI LSA-SE : Missed Spot Window for %s spot %d", (m_spotIsReplace == true) ? "Replace" : "Fixed", m_spotIndex);
         break;
      }
      case RBI_DetailCode_abnormalTermination:
      {
         std::string reason = "Read error. Server may not be sending data fast enough";

         detailCodeString  = "Abnormal termination of play out.";
         detailCodeString = detailCodeString + " reason=" + '\"';

         size_t  bytesReq = b64_encoded_size(reason.length() + 1);
         uint8_t *base64DataPtr = (uint8_t*)calloc(bytesReq+1, sizeof(uint8_t));
         if(base64DataPtr)
         {
            base64_encode((const  uint8_t*) reason.c_str(), reason.length(), base64DataPtr);
            base64DataPtr[bytesReq] = '\0';
            detailCodeString = detailCodeString + (char*)base64DataPtr;

            free(base64DataPtr);
            base64DataPtr = NULL;
         }
         detailCodeString = detailCodeString + '\"' + " ContentURL=" + '\"';

         bytesReq = b64_encoded_size(strlen(status.uri) + 1);
         base64DataPtr = (uint8_t*)calloc(bytesReq+1, sizeof(uint8_t));
         if(base64DataPtr)
         {
            base64_encode((const  uint8_t*) status.uri, strlen(status.uri), base64DataPtr);
            base64DataPtr[bytesReq] = '\0';
            detailCodeString = detailCodeString + (char*)base64DataPtr;

            free(base64DataPtr);
            base64DataPtr = NULL;
         }
         detailCodeString = detailCodeString + '\"';

         ERROR("RBI LSA-SE : Abnormal termination for %s spot %d", (m_spotIsReplace == true) ? "Replace" : "Fixed", m_spotIndex);
         break;
      }
    }
    if(detailCodeString.size() > 0)
    {
       if(detailCodeString.size() < RBI_MAX_STATUS_DETAILNOTE_LEN)
       {
          strncpy( status.detailNoteString, detailCodeString.c_str(), detailCodeString.size());
          status.detailNoteString[detailCodeString.size()] = '\0';
       }
       else
       {
          strncpy( status.detailNoteString, detailCodeString.c_str(), RBI_MAX_STATUS_DETAILNOTE_LEN);
          status.detailNoteString[RBI_MAX_STATUS_DETAILNOTE_LEN] = '\0';

          ERROR("RBIContext::sendInsertionStatus: detailCodeString.size() %d is greater than RBI_MAX_STATUS_DETAILNOTE_LEN(%d)", detailCodeString.size(), RBI_MAX_STATUS_DETAILNOTE_LEN);
       }
   }
   result= IARM_Bus_BroadcastEvent(RBI_RPC_NAME, RBI_EVENT_INSERTION_STATUS, &status, sizeof(status));
   INFO( "RBIContext::sendInsertionStatus: emit RBI_EVENT_INSERTION_STATUS (%d) result %d", RBI_EVENT_INSERTION_STATUS, result );
   m_detailCode = 0;
   m_insertionStatusUpdateEmitted = false;
}

RBIStreamProcessor::RBIStreamProcessor()
  : m_havePAT(false), m_versionPAT(0), m_program(-1), m_pmtPid(-1), m_havePMT(false), m_versionPMT(0), 
    m_pcrPid(-1), m_videoPid(-1), m_audioPid(-1), m_triggerEnable(false), m_privateDataEnable(false), m_isH264(false),
    m_inSync(false), m_packetSize(188), m_ttsSize(0), m_remainderOffset(0), m_remainderSize(0),
    m_frameDetectEnable(false), m_streamOffset(0LL),
    m_frameStartOffset(0LL), m_bitRate(0), m_avgBitRate(0),
    m_ttsInsert(0xFFFFFFFF), m_frameWidth(-1), m_frameHeight(-1), m_isInterlaced(false), m_isInterlacedKnown(false),
    m_frameType(0), m_scanForFrameType(false), m_scanHaveRemainder(false), m_prescanForFrameType(false), m_prescanHaveRemainder(false),
    m_emulationPreventionCapacity(0), m_emulationPreventionOffset(0), m_emulationPrevention(0),
    m_haveBaseTime(false), m_haveSegmentBasePCRAfterTransitionToAd(false), PCRBaseTime(false), m_baseTime(-1LL), m_basePCR(-1LL), m_segmentBaseTime(-1LL), m_segmentBasePCR(-1LL),
    m_currentPCR(-1LL), m_prevCurrentPCR(-1LL), m_currentPTS(-1LL), m_leadTime(-1LL), m_currentAudioPTS(-1LL), m_currentMappedPTS(-1LL),
    m_currentMappedPCR(-1LL), m_currentInsertPCR(-1LL),
    m_sample1PCR(-1LL), m_sample1PCROffset(0LL), m_sample2PCR(-1LL), m_sample2PCROffset(0LL),
    m_trackContinuityCountersEnable(true), m_GOPSize(-1), m_frameInGOPCount(-1), m_startOfSequence(false), m_startOfSequencePrev(false),
    m_mapStartPending(false), m_mapStartArmed(false), m_mapping(false), m_mapEndPending(false), m_mapEndPendingPrev(false), m_mapEnding(false), m_mapEndingNeedVideoEOF(false),
    m_backToBack(false), m_backToBackPrev(false), m_randomAccess(false), m_randomAccessPrev(false), m_mapEditGOP(false), m_spliceOffset(0), m_mapSpliceOutPTS(-1LL),
    m_mapSpliceInPTS(-1LL), m_mapSpliceOutAbortPTS(-1LL), m_mapSpliceInAbortPTS(-1LL), m_mapSpliceInAbort2PTS(-1LL), 
    m_mapStartPTS(-1LL), m_mapStartPCR(-1LL), m_insertStartTime(-1LL), m_insertEndTime(-1LL),
    m_lastInsertBufferTime(-1LL), m_nextInsertBufferTime(-1LL), m_lastInsertGetDataDuration(0),
    m_insertByteCount(0), m_insertByteCountError(0), m_minInsertBitRateOut(0), m_maxInsertBitRateOut(0), m_map(0), m_insertSrc(0),
    m_videoComponentCount(0), m_videoComponents(0),
    m_audioComponentCount(0), m_audioComponents(0),
    m_dataComponentCount(0), m_dataComponents(0),
    m_events(0), m_filterAudio(false), m_filterAudioStopPTS(0LL), m_filterAudioStop2PTS(0LL),
    m_audioReplicationEnabled(false), m_captureEndCount(0), m_outFile(0),
    m_audioInsertCount(0), m_lastBufferInsertSize(1), m_replicatingAudio(false), m_replicateAudioSrcPid(-1),
    m_replicateAudioStreamType(0), m_replicateAudioIndex(0), m_TimeSignalbase64Msg(NULL), m_splicePTS_offset(1500),
    m_adVideoFrameCount(0),
    m_netReplaceableVideoFramesCnt(0),
    m_lastAdVideoFramePTS(-1LL),
    m_maxAdVideoFrameCountReached(false),
    m_maxAdAudioFrameCountReached(false),
    m_lastAdVideoFrameDetected(false),
    m_lastAdPCR(-1), 
        m_lastPCRTTSValue(0),
        m_lastMappedPCRTTSValue(-1),
        m_firstAdPacketAfterTransitionTTSValue(0), 
        m_adVideoPktsDelayed(false),
        m_adVideoPktsDelayedPCRBased(false),
        m_adVideoPktsTooFast(false),
        m_adAudioPktsDelayed(false), 
        m_adPktsAccelerationEnable(false), 
        m_skipNullPktsInAd(false), 
        m_transitioningToAd(false), 
        m_transitioningToNetwork(false),
        m_ac3FrameByteCnt(0),
        m_adAccelerationState(false),
        m_removeNull_PSIPackets(false),
        m_determineNetAC3FrameSizeDuration(true),
        m_determineAdAC3FrameSizeDuration(true),
        m_spliceAudioPTS_offset(2880),
        m_currentAdAudioPTS(0), 
        m_mapSpliceUpdatedOutPTS(-1LL), m_b2bPrevAdAudioData(NULL), m_b2bPrevAdAudioDataCopy(NULL),
        m_b2bSavedAudioPacketsCnt(0),
        m_b2bBackedAdAudioPacketsCnt(0),
        m_prevAdBaseTime(-1LL),
        m_prevAdSegmentBaseTime(-1LL)
{
   memset( m_triggerPids, 0x00, sizeof(m_triggerPids) );
   memset( m_privateDataPids, 0x00, sizeof(m_privateDataPids) );
   memset( m_privateDataTypes, 0x00, sizeof(m_privateDataTypes) );
   memset( m_continuityCounters, 0x00, sizeof(m_continuityCounters) );
   memset( &m_privateDataMonitor, 0x00, sizeof(m_privateDataMonitor) );
   memset( m_SPS, 0, 32 * sizeof(H264SPS) );
   memset( m_tableAggregator, 0x00, sizeof(m_tableAggregator) );
   m_tableAggregator[TABLE_AGGREGATOR_INDEX_PMT].m_packets = new unsigned char[MAX_PMT_PACKET_BUFFER_SIZE];
   m_tableAggregator[TABLE_AGGREGATOR_INDEX_SCTE35].m_packets = new unsigned char[MAX_SCTE35_PACKET_BUFFER_SIZE];
   memset( m_replicateAudioPacket, 0x00, sizeof(m_replicateAudioPacket) );

   m_H264Enabled= RBIManager::getInstance()->getH264Enabled();
   m_captureEnabled= RBIManager::getInstance()->getCaptureEnabled();
   m_audioReplicationEnabled= RBIManager::getInstance()->getAudioReplicationEnabled();
   m_spliceOffset= RBIManager::getInstance()->getSpliceOffset();
}

RBIStreamProcessor::~RBIStreamProcessor()
{
   termComponentInfo();
   delete [] m_tableAggregator[TABLE_AGGREGATOR_INDEX_PMT].m_packets;
   if( m_tableAggregator[TABLE_AGGREGATOR_INDEX_PMT].m_section)
   {
      delete [] m_tableAggregator[TABLE_AGGREGATOR_INDEX_PMT].m_section;
   }

   delete [] m_tableAggregator[TABLE_AGGREGATOR_INDEX_SCTE35].m_packets;
   if( m_tableAggregator[TABLE_AGGREGATOR_INDEX_SCTE35].m_section)
   {
      delete [] m_tableAggregator[TABLE_AGGREGATOR_INDEX_SCTE35].m_section;
   }
}
      
void RBIStreamProcessor::markNextGOPBroken()
{
   m_mapEditGOP= true;
}

void RBIStreamProcessor::useSI(RBIStreamProcessor *other)
{
   m_havePAT= other->m_havePAT;
   m_versionPAT= other->m_versionPAT;
   m_program= other->m_program;
   m_pmtPid= other->m_pmtPid;
   m_havePMT= other->m_havePMT;
   m_versionPMT= other->m_versionPMT;
   m_pcrPid= other->m_pcrPid;
   m_videoPid= other->m_videoPid;
   m_audioPid= other->m_audioPid;
   m_inSync= true;
}

bool RBIStreamProcessor::startInsertion( RBIInsertSource *insert, long long startPTS, long long endPTS )
{
   bool result= true;
   RBIPidMap *map= 0;
   
   if ( insert )
   {
      int i = 0, j = 0, k = 0, l = 0;

      // Starting a "replace" spot
      
      RBIStreamProcessor &processorInsert= insert->getStreamProcessor();

      if ( 
           ((processorInsert.m_videoComponentCount > 0) && (m_videoComponentCount == 0)) ||
           ((m_videoComponentCount > 0) && (processorInsert.m_videoComponentCount == 0)) ||
           ((processorInsert.m_audioComponentCount > 0) && (m_audioComponentCount == 0)) ||
           ((m_audioComponentCount > 0) && (processorInsert.m_audioComponentCount == 0)) 
         )
      {
         ERROR( "RBIStreamProcessor::startInsertion: stream components are not minimally compatible" );
         result= false;
      }
      else
      {
         if ( m_havePAT && m_havePMT && 
              processorInsert.m_havePAT && processorInsert.m_havePMT )
         {   
            map= new RBIPidMap();
            if ( map )
            {
                map->m_pmtPid= processorInsert.m_pmtPid;
                memcpy( map->m_replacementPAT, m_pat, m_packetSize );
                memcpy( map->m_replacementPMT, m_tableAggregator[TABLE_AGGREGATOR_INDEX_PMT].m_packets, 
                        m_tableAggregator[TABLE_AGGREGATOR_INDEX_PMT].m_packetWriteOffset );
                map->m_psiInsertionIncomplete = false;
                map->m_replacementPMTLength = m_tableAggregator[TABLE_AGGREGATOR_INDEX_PMT].m_packetWriteOffset;

                memset( map->m_pidMap, 0, sizeof(map->m_pidMap) );
                memset( map->m_needPayloadStart, 0, sizeof(map->m_needPayloadStart) );
                memset( map->m_pidIsMapped, 0, sizeof(map->m_pidIsMapped) );

                // Map video based on PMT order
                int videoMappedCount= 0;
                for( i= 0; i < processorInsert.m_videoComponentCount; ++i )
                {
                    if ( 
                            (processorInsert.m_videoComponents[i].streamType == m_videoComponents[i].streamType) ||
                            ( ((m_videoComponents[i].streamType == 0x02) ||
                               (m_videoComponents[i].streamType == 0x80) ) &&
                              ((processorInsert.m_videoComponents[i].streamType == 0x02) ||
                               (processorInsert.m_videoComponents[i].streamType == 0x80)) ) 
                       )
                    {
                        ++videoMappedCount;
                        TRACE2( "map video pid %x to %x", processorInsert.m_videoComponents[i].pid, m_videoComponents[i].pid );
                        map->m_pidMap[processorInsert.m_videoComponents[i].pid]= m_videoComponents[i].pid;
                    }
                    else
                    {
                        ERROR( "RBIStreamProcessor::startInsertion: video component %d do not have the same stream types (primary %d vs insert %d)",
                                m_videoComponents[i].streamType, processorInsert.m_videoComponents[i].streamType );
                        result= false;
                        goto exit;
                    }
                }

                // First map audio by language code
                int audioMappedCount= 0;
                char undeterminedAudioLang[] = "und";
                for( i= 0; i < processorInsert.m_audioComponentCount; ++i )
                {
                    if ( processorInsert.m_audioComponents[i].associatedLanguage )
                    {
                        for( j= 0; j < m_audioComponentCount; ++j )
                        {
                            if( map->m_pidIsMapped[m_audioComponents[j].pid] )
                            {
                                continue;
                            }
                            if (
                                    /* stream is not mapped yet AND */
                                    ( map->m_pidMap[processorInsert.m_audioComponents[i].pid] == 0) &&
                                    /* if stream type encoding is the same */
                                    ( m_audioComponents[j].streamType == processorInsert.m_audioComponents[i].streamType) &&
                                    /* AND inserted content language is defined */
                                    ( m_audioComponents[j].associatedLanguage &&
                                      /* AND associated languages of broadcast and inserted content matches OR */
                                      ( ( strncmp( processorInsert.m_audioComponents[i].associatedLanguage,
                                                   m_audioComponents[j].associatedLanguage, 3 ) == 0 ) ||
                                        /* broadcast language is labeled undefined OR */
                                        ( strncmp( processorInsert.m_audioComponents[i].associatedLanguage, undeterminedAudioLang, 3 ) == 0 ) ||
                                        /* inserted content language is labeled undefined */
                                        ( strncmp( m_audioComponents[j].associatedLanguage, undeterminedAudioLang, 3 ) == 0 ) ) )
                               )
                            {
                                ++audioMappedCount;
                                INFO( "map audio pid %x[%s] to %x[%s] based on lang",
                                  processorInsert.m_audioComponents[i].pid, processorInsert.m_audioComponents[i].associatedLanguage,
                                  m_audioComponents[j].pid, m_audioComponents[j].associatedLanguage );
                                map->m_pidMap[processorInsert.m_audioComponents[i].pid]= m_audioComponents[j].pid;
                                map->m_needPayloadStart[processorInsert.m_audioComponents[i].pid]= true;
                                map->m_pidIsMapped[m_audioComponents[j].pid]= true;
                                break;
                            }
                            else if
                                (
                                 /* stream is not mapped yet AND */
                                 (map->m_pidMap[processorInsert.m_audioComponents[i].pid] == 0) &&
                                 /* if stream type encoding is the same */
                                 ( m_audioComponents[j].streamType == processorInsert.m_audioComponents[i].streamType) &&
                                 /* AND if the inserted content language is not defined OR */
                                 ( ( m_audioComponents[j].associatedLanguage == 0 ) ||
                                   /* broadcast language is not equal to the inserted content language */
                                   ( processorInsert.m_audioComponents[i].associatedLanguage !=
                                     m_audioComponents[j].associatedLanguage ) )
                                )
                                {
                                    ++audioMappedCount;
                                    INFO( "map audio pid %x[%s] to %x[%s] based on lang",
                                      processorInsert.m_audioComponents[i].pid, processorInsert.m_audioComponents[i].associatedLanguage,
                                      m_audioComponents[j].pid, m_audioComponents[j].associatedLanguage );
                                    map->m_pidMap[processorInsert.m_audioComponents[i].pid]= m_audioComponents[j].pid;
                                    map->m_needPayloadStart[processorInsert.m_audioComponents[i].pid]= true;
                                    map->m_pidIsMapped[m_audioComponents[j].pid]= true;
                                    break;
                                }
                        }
                    }
                }

                if ( audioMappedCount < m_audioComponentCount && audioMappedCount < MAX_AUDIO_PID_COUNT )
                {
                    if( m_audioReplicationEnabled )
                    {
                        // Replicate default audio stream in Ad to fill any unmapped audio pids in the linear content 
                        for( j= 0; j < m_audioComponentCount; ++j )
                        {
                            for( i= 0; i < processorInsert.m_audioComponentCount; ++i )
                            {
                                if ( map->m_pidMap[processorInsert.m_audioComponents[i].pid] == m_audioComponents[j].pid )
                                {
                                    break;
                                }
                            }
                            if ( i == processorInsert.m_audioComponentCount )
                            {
                                if ( m_replicateAudioSrcPid == -1 )
                                {
                                    INFO("linear audio pid %x type %d has no mapping",
                                            m_audioComponents[j].pid, m_audioComponents[j].streamType );
                                    for( i= 0; i < processorInsert.m_audioComponentCount; ++i )
                                    {
                                        if ( m_audioComponents[j].streamType == processorInsert.m_audioComponents[i].streamType )
                                        {
                                            m_replicateAudioSrcPid= processorInsert.m_audioComponents[i].pid;
                                            m_replicateAudioStreamType= processorInsert.m_audioComponents[i].streamType;
                                            INFO("choosing Ad audio pid %x type %d as stream for audio replication",
                                                    m_replicateAudioSrcPid, m_replicateAudioStreamType );
                                            break;
                                        }
                                    }
                                }
                                if ( (m_replicateAudioSrcPid != -1) &&
                                        (m_audioComponents[j].streamType == m_replicateAudioStreamType) )
                                {
                                    if ( audioMappedCount < MAX_AUDIO_PID_COUNT) 
                                    {
                                        ++audioMappedCount;
                                        INFO( "replicating audio pid %x onto pid %x[%s]", m_replicateAudioSrcPid, m_audioComponents[j].pid, m_audioComponents[j].associatedLanguage);
                                        m_replicateAudioTargetPids.push_back( m_audioComponents[j].pid );
                                    }
                                    else
                                    {
                                        INFO( "Audio PID replicate failed: mapping PID %x exceeds max audio PID count [%d]", m_audioComponents[j].pid, MAX_AUDIO_PID_COUNT );
                                    }
                                }
                                else
                                {
                                    INFO( "Audio PID replicate failed: Source PID:[%x] StreamType:[%x] Replicate StreamType:[%x]", m_replicateAudioSrcPid, m_audioComponents[j].streamType, m_replicateAudioStreamType );
                                }
                            }
                        }
                    }
                    else
                    {
                        // Map any remaining audio by PMT order
                        if ( m_audioComponentCount-audioMappedCount < processorInsert.m_audioComponentCount-audioMappedCount )
                        {
                            k= 0;
                            for( i= 0; i < processorInsert.m_audioComponentCount; ++i )
                            {
                                if ( map->m_pidMap[processorInsert.m_audioComponents[i].pid] == 0 )
                                {
                                    for( ; k < m_audioComponentCount; ++k )
                                    {
                                        int pid= m_audioComponents[k].pid;
                                        for( l= 0; l < processorInsert.m_audioComponentCount; ++l )
                                        {
                                            if ( map->m_pidMap[processorInsert.m_audioComponentCount] == pid )
                                            {
                                                break;
                                            }
                                        }
                                        if (
                                                (l >= processorInsert.m_audioComponentCount) &&
                                                (processorInsert.m_audioComponents[l].streamType == m_audioComponents[k].streamType)
                                           )
                                        {
                                            ++audioMappedCount;
                                            INFO( "map audio pid %x[%s] to %x[%s] based on order",
                                              processorInsert.m_audioComponents[i].pid, processorInsert.m_audioComponents[i].associatedLanguage,
                                              m_audioComponents[j].pid, m_audioComponents[j].associatedLanguage );
                                            map->m_pidMap[processorInsert.m_audioComponents[i].pid]= pid;
                                            map->m_needPayloadStart[processorInsert.m_audioComponents[i].pid]= true;
                                            map->m_pidIsMapped[m_audioComponents[j].pid]= true;
                                            break;
                                        }
                                    }
                                }
                                if ( audioMappedCount >= m_audioComponentCount )
                                {
                                    break;
                                }
                            }
                        }
                    }
                } 
                if ( 
                        (videoMappedCount < m_videoComponentCount) ||
                        (audioMappedCount < m_audioComponentCount)
                   )
                {
                    WARNING( "RBIStreamProcessor::startInsertion: unable to map all A/V streams (video %d of %d mapped, audio %d of %d mapped)",
                            videoMappedCount, m_videoComponentCount, audioMappedCount, m_audioComponentCount );
                }
                if ( audioMappedCount == 0 )
                {
                    if ( m_events )
                    {
                        m_events->insertionStatusUpdate( this, RBI_DetailCode_matchingAudioAssetNotFound );
                    }
                }

                if ( result )
                {
                    if ( m_map )
                    {
                        delete m_map;
                    }
                    m_map= map;
                    m_insertSrc= insert;
                    m_frameDetectEnable= true;
                    m_mapStartPending= true;
                    m_mapStartArmed= false;
                    m_mapEndPending= false;
                    m_mapEnding= false;
                    m_mapEndingNeedVideoEOF= false;
                    m_randomAccess= false;
                    m_mapEditGOP= false;
                    m_mapping= false;
                    m_haveBaseTime= false;
                    m_mapSpliceOutPTS= ((startPTS+m_spliceOffset*90LL)&MAX_90K);
                    m_mapSpliceInPTS= endPTS;
                    m_mapSpliceOutAbortPTS= ((startPTS + 1000LL*90LL)&MAX_90K);
                    m_mapSpliceInAbortPTS= ((endPTS + 1000LL*90LL)&MAX_90K);
                    m_mapSpliceInAbort2PTS= ((endPTS + 1500LL*90LL)&MAX_90K);
                    m_insertByteCount= 0;
                    m_insertByteCountError= 0;
                    m_minInsertBitRateOut= 0;
                    m_maxInsertBitRateOut= 0;
                    m_insertStartTime= getCurrentTimeMicro();
                    DEBUG("Setting insert start time %lld " , m_insertStartTime);
                    m_lastInsertBufferTime= -1LL;
                    m_mapStartPTS= -1LL;
                    m_sample1PCR= -1LL;
                    m_sample1PCROffset= 0LL;
                    m_sample2PCR= -1LL;
                    m_sample2PCROffset= 0LL;
                    m_adVideoFrameCount = 0;
                    m_mapSpliceUpdatedOutPTS = -1;

                    m_netReplaceableVideoFramesCnt = round((double)((endPTS - startPTS)*VIDEO_FRAME_RATE/90000));
                    INFO("Number of network video frames available for replacement %d", m_netReplaceableVideoFramesCnt);

                    m_transitioningToAd = false;
                    m_transitioningToNetwork = false;

                    m_lastAdVideoFramePTS = -1LL;
                    m_maxAdVideoFrameCountReached = false;
                    m_maxAdAudioFrameCountReached = false;
                    m_lastAdVideoFrameDetected = false;

                    m_haveSegmentBasePCRAfterTransitionToAd = false;
                    m_currentAdAudioPTS = 0;
                    m_lastAdPCR = -1;
                    m_removeNull_PSIPackets = false;
                    m_determineNetAC3FrameSizeDuration = true;
                    m_determineAdAC3FrameSizeDuration= true;

                    if( !m_backToBackPrev )
                    {
                        m_lastMappedPCRTTSValue = -1;
                        m_mapStartPCR= -1LL;
                    }
                    INFO("replace: m_currentPTS %llx m_mapSpliceOutPTS %llx m_mapSpliceOutAbortPTS %llx m_mapSpliceInAbortPTS %llx m_mapSpliceInAbort2PTS %llx m_mapSpliceInPTS %llx backToBack: %d",
                            m_currentPTS, m_mapSpliceOutPTS, m_mapSpliceOutAbortPTS, m_mapSpliceInAbortPTS, m_mapSpliceInAbort2PTS, m_mapSpliceInPTS, m_backToBack );
                }
            }
         }
         else
         {
            ERROR( "RBIStreamProcessor::startInsertion: streams not ready" );
            result= false;
         }   
      }   
   }
   else
   {
      // Starting a "fixed" spot

      if ( m_map )
      {
         delete m_map;
      }
      m_map= 0;
      m_insertSrc= 0;
      m_frameDetectEnable= true;
      m_mapStartPending= true;
      m_mapStartArmed= false;
      m_mapEndPending= false;
      m_mapEnding= false;
      m_mapEndingNeedVideoEOF= false;
      m_randomAccess= false;
      m_mapEditGOP= false;
      m_mapping= false;
      m_haveBaseTime= false;
      m_mapSpliceOutPTS= startPTS;
      m_mapSpliceInPTS= endPTS;
      m_mapSpliceOutAbortPTS= ((startPTS + 1LL*90000LL)&MAX_90K);
      m_mapSpliceInAbortPTS= ((endPTS + 1000LL*90LL)&MAX_90K);
      m_mapSpliceInAbort2PTS= ((endPTS + 1500LL*90LL)&MAX_90K);
      m_insertByteCount= 0;
      m_insertByteCountError= 0;
      m_minInsertBitRateOut= 0;
      m_maxInsertBitRateOut= 0;
      m_insertStartTime= getCurrentTimeMicro();
      DEBUG("Setting insert start time Fixed %lld " , m_insertStartTime);
      m_lastInsertBufferTime= -1LL;
      m_mapStartPTS= -1LL;
      m_mapStartPCR= -1LL;
      m_sample1PCR= -1LL;
      m_sample1PCROffset= 0LL;
      m_sample2PCR= -1LL;
      m_sample2PCROffset= 0LL;

        m_netReplaceableVideoFramesCnt = round((double)((endPTS - startPTS)*VIDEO_FRAME_RATE/90000));
        INFO("Number of network video frames in fixed ad spot %d", m_netReplaceableVideoFramesCnt);

        m_transitioningToAd = false;
        m_transitioningToNetwork = false;      
  
        m_currentAdAudioPTS = 0;
        m_lastMappedPCRTTSValue = -1;
        m_lastAdPCR = -1;
        m_determineNetAC3FrameSizeDuration = true;
        m_determineAdAC3FrameSizeDuration= true;
        m_mapSpliceUpdatedOutPTS = -1;

      INFO("fixed: m_currentPTS %llx m_mapSpliceOutPTS %llx m_mapSpliceOutAbortPTS %llx m_mapSpliceInAbortPTS %llx m_mapSpliceInAbort2PTS %llx m_mapSpliceInPTS %llx",
            m_currentPTS, m_mapSpliceOutPTS, m_mapSpliceOutAbortPTS, m_mapSpliceInAbortPTS, m_mapSpliceInAbort2PTS, m_mapSpliceInPTS );
   }
   
exit:

   if ( !result )
   {
      if ( map )
      {
         delete map;
      }
   }
   
   return result;
}

void RBIStreamProcessor::stopInsertion()
{
    // DELIA-38430: Start.
    // Added the debug print to find the frequency of the issue DELIA-38430 reported in the field.
    long long diff = 0;
    long long playedOutPTS = (((m_currentPTS-m_mapStartPTS)&MAX_90K)/90LL);
    long long spliceOutPTSDelta = (((m_mapSpliceInPTS-m_mapSpliceOutPTS)&MAX_90K)/90LL);

    if ( playedOutPTS > spliceOutPTSDelta ) {
        diff = (playedOutPTS - spliceOutPTSDelta);
    } else if (spliceOutPTSDelta > playedOutPTS) {
        diff = (spliceOutPTSDelta - playedOutPTS);
    } else if (playedOutPTS == spliceOutPTSDelta) {
        diff = 0;
    }

    // 1500 has no logic here, its just a number and will hit here only if there is some error case.
    if(diff > 1500)
    {
        ERROR("stopInsertion delta between played out(%lld) and splice out(%lld) is (%lld)", playedOutPTS, spliceOutPTSDelta, diff);
    }
    // DELIA-38430 : End.

   INFO("stopInsertion: (m_currentPTS-m_mapStartPTS) (%lld) (m_mapSpliceInPTS-m_mapSpliceOutPTS)= (%lld) diff (%lld)\n", (((m_currentPTS-m_mapStartPTS)&MAX_90K)/90LL), (((m_mapSpliceInPTS-m_mapSpliceOutPTS)&MAX_90K)/90LL), diff);

   m_mapping= false;
   m_mapStartPending= false;
   m_mapStartArmed= false;
   m_mapEndPending= false;
   m_randomAccess= false;
   m_mapEditGOP= false;
   m_frameDetectEnable= false;
   m_haveBaseTime= false;   
   m_mapStartPTS= -1LL;
   m_mapStartPCR= -1LL;
   m_sample1PCR= -1LL;
   m_sample1PCROffset= 0LL;
   m_sample2PCR= -1LL;
   m_sample2PCROffset= 0LL;
   m_currentAudioPTS= -1LL;
   INFO("m_currentAudioPTS %llx", m_currentAudioPTS);
   m_backToBackPrev= m_backToBack;
   m_replicatingAudio= false;
   m_replicateAudioSrcPid= -1;
   m_replicateAudioTargetPids.clear();
   m_adVideoFrameCount = 0;
   m_transitioningToAd = false;
   m_transitioningToNetwork = false;   
   m_adVideoPktsDelayed = false; 
   m_adVideoPktsTooFast = false;
   m_adAudioPktsDelayed = false; 
   m_adPktsAccelerationEnable = false;
   m_adAccelerationState = false;  
   m_currentAdAudioPTS = 0;
   m_lastAdPCR = -1;
   m_removeNull_PSIPackets = false;

   m_haveSegmentBasePCRAfterTransitionToAd = false;
   m_determineNetAC3FrameSizeDuration = true;
   m_determineAdAC3FrameSizeDuration= true;
   
   if ( !m_backToBack )
   {
      m_ttsInsert= 0xFFFFFFFF;
   }
   if ( m_map )
   {
      delete m_map;
      m_map= 0;
   }
   if ( m_events )
   {
      m_events->insertionStopped( this );
   }
}

int RBIStreamProcessor::getInsertionDataSize()
{
   int size= 0;
   long long lastBufferTime, now;
   long long interBufferTime = 0, bytes;
   int insertBitRate;
   int packetCount;
   int nominalInterval;

   if ( m_insertSrc && m_mapping )
   {
      now= getCurrentTimeMicro();

      insertBitRate= m_insertSrc->bitRate()*(m_packetSize/188.0);
      if( m_audioReplicationEnabled && (m_insertByteCount > 0) && getLastBufferInsertSize() > 0 )
      {
         insertBitRate*= (((float)(getLastAudioInsertCount() * m_packetSize) + getLastBufferInsertSize() )/((float)getLastBufferInsertSize() ));
      }

      // Use a nominal interval of 70 ms
      nominalInterval=70000;  

      lastBufferTime= m_lastInsertBufferTime;
      if ( (lastBufferTime == -1LL) || (m_insertByteCount == 0) )
      {
        // Default to a multiple of packet size that is proportional
        // to the source bitrate.
        packetCount= (MAX_PACKETS*192LL*insertBitRate/15000000LL)/m_packetSize;

        if ( packetCount > MAX_PACKETS )
        {
           packetCount= MAX_PACKETS;
           INFO("Inserting MAX_PACKTS..");
        }
        size= packetCount*m_packetSize;
      }
      else
      {
         // Determine the amount of data that the current inter-buffer
         // interval represents given the insert content bitrate.  Make
         // sure the interval is at least nominalInterval us.
         interBufferTime= now-lastBufferTime;
         if ( interBufferTime < nominalInterval )
         {
            int delay= nominalInterval-interBufferTime-m_lastInsertGetDataDuration;
            if ( delay > 0 )
            {
               return 0;
            }
            now= getCurrentTimeMicro();
            interBufferTime= (now-lastBufferTime);
         }
         bytes= (insertBitRate * (((interBufferTime)/1000000.0))/8);

         packetCount= (bytes+m_insertByteCountError+(m_packetSize/2))/m_packetSize;
         
        if (m_adVideoPktsDelayed || m_adAudioPktsDelayed || m_adVideoPktsDelayedPCRBased)
        {
            packetCount = MAX_PACKETS;
            bytes = packetCount * m_packetSize;     
            DEBUG("In getInsertionDataSize Ad packets delayed, sending max packets ");
        } 
        else if (m_adVideoPktsTooFast)
        {
            packetCount /= 2;
        }

        if ( packetCount > MAX_PACKETS )
        {
           packetCount= MAX_PACKETS;
           INFO("Inserting MAX_PACKTS...");
        }
         size= packetCount*m_packetSize;
         m_insertByteCountError += (bytes-size);
         TRACE2("m_insertByteCountError %d bytes %lld size %d", m_insertByteCountError, bytes, size );
      }

      m_nextInsertBufferTime= now;

      if ( (m_lastInsertBufferTime == -1LL) || (m_insertByteCount == 0) )
      {
         DEBUG("startTime %lld byteTotal %d srcBR %d primaryBitRate %d first buffer size %d tts %X", 
               m_insertStartTime, m_insertByteCount, insertBitRate, m_bitRate, size, m_ttsInsert );
      }
      else
      {
         long long elapsed= now-m_insertStartTime;
         int meanOutBitRate= (m_insertByteCount*8*1000000LL)/elapsed;
         int outBitRate= (size*8*1000000LL)/interBufferTime;
         if ( (outBitRate < m_minInsertBitRateOut) || (m_minInsertBitRateOut == 0) )
         {
            m_minInsertBitRateOut= outBitRate;
         }
         if ( outBitRate > m_maxInsertBitRateOut )
         {
            m_maxInsertBitRateOut= outBitRate;
         }

         int targetBitRate= (int)(insertBitRate);
         bool emitLog= (((m_lastInsertBufferTime%2000000) < 1000000) && ((m_nextInsertBufferTime%2000000) > 1000000));
         bool alarm= (outBitRate < 0.9*targetBitRate);
         interBufferTime += m_lastInsertGetDataDuration;
         if ( alarm ) emitLog= true;
         if ( emitLog )
         {
            DEBUG("elapsed %8lld (us) bytes %9d targBR %8d srcBR %d primaryBR %d tts %X %s", 
                  elapsed, m_insertByteCount, targetBitRate, insertBitRate, m_bitRate, m_ttsInsert, (alarm ? "<<<<<<" : "") ); 
            DEBUG("interval %7lld (us) size %10d outBR %9d (%d-%d) avgOutBR %d", 
                  interBufferTime, size, outBitRate, m_minInsertBitRateOut, m_maxInsertBitRateOut, meanOutBitRate);
         }
         else
         {
            DEBUG("elapsed %8lld (us) bytes %9d targBR %8d srcBR %d primaryBR %d tts %X %s", 
                  elapsed, m_insertByteCount, targetBitRate, insertBitRate, m_bitRate, m_ttsInsert, (alarm ? "<<<<<<" : "") ); 
            DEBUG("interval %7lld (us) size %10d outBR %9d (%d-%d) avgOutBR %d", 
                   interBufferTime, size, outBitRate, m_minInsertBitRateOut, m_maxInsertBitRateOut, meanOutBitRate);
         }
      }
   }
   setLastBufferInsertSize( size );
    
   return size;
}

int RBIStreamProcessor::getInsertionData( unsigned char *packets, int size )
{
   int insertedSize= 0;
   unsigned char *packet, *bufferEnd;
   int insertBitRate;
   unsigned int ttsInc = 0;
   int mapPid;
   unsigned char byte3;
   long long start, end;
   int audioInsertCount= 0;
   bool pktSkipped = false; 

   start= getCurrentTimeMicro();

    if ( (m_maxAdVideoFrameCountReached || m_mapEndPending) && m_backToBack )
    {
        m_b2bSavedAudioPacketsCnt = 0;
        m_b2bBackedAdAudioPacketsCnt = 0;
        if( !m_maxAdAudioFrameCountReached )
        {
            unsigned char *adAudioPackets = NULL, *audioDataBufferEnd = NULL, *adPacket = NULL;
            int maxNumOfAudPacketsToBeSaved = 300;

            m_b2bPrevAdAudioData = (unsigned char*)calloc(maxNumOfAudPacketsToBeSaved*188, sizeof(unsigned char) );
            adAudioPackets = m_b2bPrevAdAudioData;
            m_b2bPrevAdAudioDataCopy = m_b2bPrevAdAudioData;
            audioDataBufferEnd = m_b2bPrevAdAudioData + maxNumOfAudPacketsToBeSaved*188*sizeof(unsigned char);
            adPacket = (unsigned char*)malloc(188*sizeof(unsigned char));

            
            if (adAudioPackets)
            {                
                packet= packets+m_ttsSize;
                bufferEnd= packets+size;

                while( adAudioPackets < audioDataBufferEnd )
                {
                    int lenDidRead = m_insertSrc->read( adPacket, 188 );
                    if (lenDidRead == 188)
                    {
                        int insertPid= (((adPacket[1] << 8) | adPacket[2]) & 0x1FFF);
                        int pid = m_map->m_pidMap[insertPid];
                        if(pid == m_audioPid )
                        {
                            if(m_determineAdAC3FrameSizeDuration){
                              int payloadOffset = 4;
                              int adaptation = ((adPacket[3] & 0x30) >> 4);

                              if (adaptation & 0x02) {
                                  payloadOffset += (1 + adPacket[4]);
                              }
                              int payloadStart = (adPacket[1] & 0x40);
                              if ((adaptation & 0x01) && payloadStart && ((adPacket[payloadOffset] == 0x00) && (adPacket[payloadOffset + 1] == 0x00) && (adPacket[payloadOffset + 2] == 0x01))) {
                                  payloadOffset += (9 + adPacket[payloadOffset + 8]);
                                  getAC3FrameSizeDuration( adPacket, payloadOffset, &adAC3FrameInfo );
                                  if( adAC3FrameInfo.duration > 0 && adAC3FrameInfo.size > 0)
                                  {
                                      m_determineAdAC3FrameSizeDuration = false;
                                  }
                                }
                            }
                            if( !m_determineAdAC3FrameSizeDuration ){
                                    long long audioPTS = getAC3FramePTS( adPacket, &adAC3FrameInfo, m_currentAdAudioPTS);
                                if (audioPTS != -1)
                                {
                                    m_currentAdAudioPTS = audioPTS;
                                    if (m_currentAdAudioPTS >= m_lastAdVideoFramePTS)
                                    {
                                        DEBUG("Maximum Ad Audio frames reached, Net Video frames to be replaced %d Ad Video frame count %d Last Ad Video Frame PTS %llx Last Ad Audio Frame PTS %llx", m_netReplaceableVideoFramesCnt, m_adVideoFrameCount, m_lastAdVideoFramePTS, m_currentAdAudioPTS); 
                                        m_maxAdAudioFrameCountReached = true;
                                        for( int i = 188 - adAC3FrameInfo.byteCnt; i < 188; i++ )
                                        {
                                            adPacket[i] = 0xFF;
                                        }                                
                                    }
                                }
                            }
                            
                            adPacket[1]= ((adPacket[1] & 0xE0) | ((pid>>8) & 0x1F));
                            adPacket[2]= (pid & 0xFF);
                            memcpy(adAudioPackets, adPacket, 188);
                            adAudioPackets += 188;
                            maxNumOfAudPacketsToBeSaved--;
                            m_b2bSavedAudioPacketsCnt++;
                            
                            if( m_maxAdAudioFrameCountReached )
                            {
                                break;
                            }
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }
            DEBUG("B2B Ad save remaining ad audio data for insertion during the next LSA ad, number of audio packets read until splice point %d remaining space in buffer %d ", m_b2bSavedAudioPacketsCnt, maxNumOfAudPacketsToBeSaved);
        }
        DEBUG("B2B Ad stopping insertion at PTS %llx", m_currentPTS);
        stopInsertion();
    }

   
   if ( m_insertSrc && m_mapping )
   { 
      packet= packets+m_ttsSize;

      bufferEnd= packets+size;

      if ( m_ttsSize )
      {
         insertBitRate= m_insertSrc->bitRate()*(m_packetSize/188.0);
        ttsInc= (8*192*27000000LL/(insertBitRate));
         DEBUG("RBIStreamProcessor::getInsertionData: insertBitRate %d ttsInc %d", insertBitRate, ttsInc);
      }

      m_lastInsertBufferTime= m_nextInsertBufferTime;

      int backedupAudFramePktCnt = 0;

      while( packet < bufferEnd )
      {
                
        if( m_ttsSize ) 
        {
            if( m_adVideoPktsDelayed || m_adAudioPktsDelayed || m_adVideoPktsDelayedPCRBased )
            {
                ttsInc = 8*192*27/40;  
            }
            else if( m_adVideoPktsTooFast )
            {
                ttsInc= (8*192*2*27000000LL/(insertBitRate));
            }
            else
            {   
                ttsInc= (8*192*27000000LL/(insertBitRate));
            } 
        }
        
            if ( m_audioReplicationEnabled && m_replicatingAudio )
            {
               memcpy( packet, m_replicateAudioPacket, m_packetSize-m_ttsSize );
               mapPid= m_replicateAudioTargetPids[m_replicateAudioIndex];
               packet[1]= ((packet[1] & 0xE0) | ((mapPid>>8) & 0x1F));
               packet[2]= (mapPid & 0xFF);
               m_ttsInsert-= ttsInc;
               ++m_replicateAudioIndex;
               ++audioInsertCount;
               if ( m_replicateAudioIndex >= m_replicateAudioTargetPids.size() )
               {
                  m_replicatingAudio= false;
               }
            } 
            else if(backedupAudFramePktCnt > 0 && m_backToBackPrev && (m_b2bSavedAudioPacketsCnt > 0 || m_b2bBackedAdAudioPacketsCnt > 0) )
            {
                mapPid = (((m_b2bPrevAdAudioData[1] << 8) | m_b2bPrevAdAudioData[2]) & 0x1FFF);                    
                memcpy (packet, m_b2bPrevAdAudioData, 188);
                if( m_b2bSavedAudioPacketsCnt > 0 )
                {
                    m_b2bSavedAudioPacketsCnt--;
                    if( m_b2bSavedAudioPacketsCnt == 0 )
                    {
                        m_b2bPrevAdAudioData = m_b2bPrevAdAudioDataCopy;
                    }
                    else
                    {
                        m_b2bPrevAdAudioData += 188;
                    }
                }    
                else if ( m_b2bBackedAdAudioPacketsCnt > 0 )
                {
                    m_b2bPrevAdAudioData += 188;
                    
                    if( (m_b2bPrevAdAudioData - m_b2bPrevAdAudioDataCopy) == 188*m_b2bBackedAdAudioPacketsCnt )
                    {
                        m_b2bBackedAdAudioPacketsCnt = 0;                        
                    }
                    
                }    

                backedupAudFramePktCnt--;
                
                if ( m_b2bSavedAudioPacketsCnt  == 0 && m_b2bBackedAdAudioPacketsCnt == 0)
                {
                    if( m_b2bPrevAdAudioDataCopy )
                    {
                        free(m_b2bPrevAdAudioDataCopy);                                                                        
                    }
                } 
                // update continuity counters and timestamps
                byte3= packet[3];
                if ( byte3 & 0x10 )
                {
                    ++m_continuityCounters[mapPid];
                }
                packet[3]= ((byte3 & 0xF0)|(m_continuityCounters[mapPid] & 0x0F));
                reTimestamp( packet, m_packetSize );

                if ( m_audioReplicationEnabled && m_replicateAudioSrcPid != -1)
                {
                    m_replicatingAudio= true;
                    m_replicateAudioIndex= 0;
                    memcpy( m_replicateAudioPacket, packet, m_packetSize-m_ttsSize );
                }
            }
            else
            {
                if( m_map->m_psiInsertionIncomplete )
                {
                    /* There's some PSI data waiting to be inserted. In the current implementation, this has to be the PMT. */
                    mapPid = m_pmtPid;
                    memcpy( packet, &(m_map->m_replacementPMT[m_map->m_replacementPMTReadOffset + m_ttsSize]), m_packetSize-m_ttsSize );
                    m_map->m_replacementPMTReadOffset += m_packetSize;
                    if( m_map->m_replacementPMTReadOffset >= m_map->m_replacementPMTLength )
                    {
                        m_map->m_psiInsertionIncomplete = false;
                    }
                    /* Finished PMT handling */

                    byte3= packet[3];
                    packet[3]= ((byte3 & 0xF0)|(++m_continuityCounters[mapPid] & 0x0F));
                    reTimestamp( packet, m_packetSize );
                }
                else
                {
                    int lenDidRead = m_insertSrc->read( packet, m_packetSize-m_ttsSize );
                    bool audioPacketFromPrevLSAAd = false;
                    bool replicateAudioPacketFromPrevLSAAd = false;

                    if ( lenDidRead == (m_packetSize-m_ttsSize) )
                    {
                        int insertPid= (((packet[1] << 8) | packet[2]) & 0x1FFF);

                        if( insertPid == 0x1FFF && m_backToBackPrev && m_b2bSavedAudioPacketsCnt > 0 )
                        {
                            if( m_b2bPrevAdAudioData )
                            {
                                if(m_b2bPrevAdAudioData[0] == 0x47)
                                {
                                    insertPid= (((m_b2bPrevAdAudioData[1] << 8) | m_b2bPrevAdAudioData[2]) & 0x1FFF);
                                    memcpy (packet, m_b2bPrevAdAudioData, 188); 
                                    m_b2bSavedAudioPacketsCnt--;
                                    if( m_b2bSavedAudioPacketsCnt == 0 && m_b2bBackedAdAudioPacketsCnt > 0)
                                    {
                                        DEBUG("B2B Ad in replacing null packets resetting Ad buffer pointer to audio packets for current backed up ad audio packets m_b2bBackedAdAudioPacketsCnt %d ", m_b2bBackedAdAudioPacketsCnt);
                                        m_b2bPrevAdAudioData = m_b2bPrevAdAudioDataCopy;
                                    }
                                    else if ( m_b2bSavedAudioPacketsCnt > 0 )
                                    {
                                        m_b2bPrevAdAudioData += 188;
                                    }
                                    audioPacketFromPrevLSAAd = true;
                                    if ( m_audioReplicationEnabled && m_replicateAudioSrcPid != -1)
                                    {
                                        replicateAudioPacketFromPrevLSAAd = true;
                                    }
                                }
                                if ( m_b2bSavedAudioPacketsCnt  == 0 && m_b2bBackedAdAudioPacketsCnt == 0 )
                                {
                                    DEBUG("B2B Ad remaining audio packets processed free memory ");
                                    if( m_b2bPrevAdAudioDataCopy )
                                    {
                                        free(m_b2bPrevAdAudioDataCopy);                                                                        
                                    }
                                }
                            }
                        }
                        if (insertPid == 0x1FFF && m_skipNullPktsInAd)
                        {   
                            pktSkipped = true;
                        }
                        else
                        {
                            if ( insertPid == 0 )
                            {
                                mapPid= 0;
                                memcpy( packet, m_map->m_replacementPAT+m_ttsSize, m_packetSize-m_ttsSize );
                            }
                            else if ( insertPid == m_map->m_pmtPid )
                            {

                                mapPid= m_pmtPid;
                                memcpy( packet, m_map->m_replacementPMT+m_ttsSize, m_packetSize-m_ttsSize );
                                m_map->m_replacementPMTReadOffset = m_packetSize;
                                if( m_map->m_replacementPMTReadOffset != m_map->m_replacementPMTLength )
                                {
                                    m_map->m_psiInsertionIncomplete = true;
                                }
                                //TODO: what if the ad PMT has multiple packets in them?
                            }
                            else if ( insertPid == 0x1FFF || audioPacketFromPrevLSAAd )
                            {
                                mapPid= insertPid;
                            }
                            else
                            {
                                mapPid= m_map->m_pidMap[insertPid];
                                if ( mapPid )
                                {
                                    // Trim Ad video and audio frames if necessary                             
                                    int payloadStart= (packet[1] & 0x40);
                                    if ( mapPid == m_videoPid )
                                    {
                                        if ( !m_maxAdVideoFrameCountReached && payloadStart )
                                        {
                                            ++m_adVideoFrameCount;

                                            int lastAdVideoFrameNumber = m_netReplaceableVideoFramesCnt;
                                            // Locate Ad frame to exit    
                                            if( m_adVideoFrameCount > m_netReplaceableVideoFramesCnt - 30 &&  m_insertSrc->getAdVideoExitFrameInfo().size() > 0 )
                                            {     
                                                int exitFrameInfoSize = m_insertSrc->getAdVideoExitFrameInfo().size();
                                                for ( int index = (exitFrameInfoSize-1); index >= 0; --index )
                                                {
                                                    int exitFrameNumber = m_insertSrc->getAdVideoExitFrameInfo().at(index);
                                                    if(exitFrameNumber <= m_netReplaceableVideoFramesCnt)
                                                    {
                                                        DEBUG("Found Ad frame to exit, frame number: %d ", exitFrameNumber  );
                                                        if ( index == (exitFrameInfoSize-1) )
                                                        {
                                                            m_lastAdVideoFrameDetected = true;
                                                        }
                                                        lastAdVideoFrameNumber = exitFrameNumber;
                                                        break;
                                                    }
                                                }
                                            }

                                            if( (m_adVideoFrameCount >  lastAdVideoFrameNumber) || ( (m_adVideoFrameCount == lastAdVideoFrameNumber) && m_lastAdVideoFrameDetected ) )
                                            {                                                    
                                                int payload = 4;
                                                int adaptation= ((packet[3] & 0x30)>>4);
                                                if ( adaptation & 0x02 )
                                                {
                                                    payload += (1+packet[4]);
                                                }
                                                // Determine last Ad video frame PTS
                                                if ( (packet[payload] == 0x00) && (packet[payload+1] == 0x00) && (packet[payload+2] == 0x01) )
                                                {
                                                    int tsbase= payload+9;

                                                    if ( packet[payload+7] & 0x80 )
                                                    {     
                                                        if (m_lastAdVideoFrameDetected)
                                                        {
                                                            m_lastAdVideoFramePTS = readTimeStamp( &packet[tsbase] ) + (long long)(90000/VIDEO_FRAME_RATE);
                                                        }
                                                        else
                                                        {
                                                            m_lastAdVideoFramePTS = readTimeStamp( &packet[tsbase] );                                                         
                                                        }
                                                        DEBUG("Maximum Ad Video frames reached, Net Video frames to be replaced %d Ad Video frame count %d Last Ad Video Frame PTS %llx, m_lastAdVideoFrameDetected %d ", m_netReplaceableVideoFramesCnt, m_adVideoFrameCount, m_lastAdVideoFramePTS, m_lastAdVideoFrameDetected);                                                             
                                                        m_maxAdVideoFrameCountReached = true;
                                                        m_adPktsAccelerationEnable = false;
                                                        m_adVideoPktsDelayed = false;
                                                        m_adVideoPktsDelayedPCRBased = false;
                                                        m_adAudioPktsDelayed = false;                                                        
                                                        m_adVideoPktsTooFast = false;
                                                    }
                                                    --m_adVideoFrameCount;
                                                }    
                                            } 
                                        }

                                        if ( m_maxAdVideoFrameCountReached && !m_lastAdVideoFrameDetected)
                                        {
                                           mapPid= 0x1FFF;
                                        }                                            
                                    }                                       
                                    else if( mapPid == m_audioPid )
                                    {
                                        if( m_maxAdVideoFrameCountReached )
                                        {
                                            if ( m_maxAdAudioFrameCountReached )
                                            {
                                                mapPid= 0x1FFF;
                                            }
                                            else
                                            {
                                                if(m_determineAdAC3FrameSizeDuration){
                                                  int payloadOffset = 4;
                                                  int adaptation = ((packet[3] & 0x30) >> 4);

                                                  if (adaptation & 0x02) {
                                                      payloadOffset += (1 + packet[4]);
                                                  }
                                                  payloadStart = (packet[1] & 0x40);
                                                  if ((adaptation & 0x01) && payloadStart && ((packet[payloadOffset] == 0x00) && (packet[payloadOffset + 1] == 0x00) && (packet[payloadOffset + 2] == 0x01))) {
                                                      payloadOffset += (9 + packet[payloadOffset + 8]);
                                                      getAC3FrameSizeDuration( packet, payloadOffset, &adAC3FrameInfo );
                                                      if( adAC3FrameInfo.duration > 0 && adAC3FrameInfo.size > 0)
                                                      {
                                                          m_determineAdAC3FrameSizeDuration = false;
                                                      }
                                                    }
                                                }
                                                if( !m_determineAdAC3FrameSizeDuration ){
                                                        long long audioPTS = getAC3FramePTS( packet, &adAC3FrameInfo, m_currentAdAudioPTS);
                                                    if (audioPTS != -1)
                                                    {
                                                        m_currentAdAudioPTS = audioPTS;
                                                        if (m_currentAdAudioPTS >= m_lastAdVideoFramePTS)
                                                        {
                                                            DEBUG("Maximum Ad Audio frames reached, Net Video frames to be replaced %d Ad Video frame count %d Last Ad Video Frame PTS %llx Last Ad Audio Frame PTS %llx", m_netReplaceableVideoFramesCnt, m_adVideoFrameCount, m_lastAdVideoFramePTS, m_currentAdAudioPTS); 
                                                            m_maxAdAudioFrameCountReached = true;
                                                            for( int i = 188 - adAC3FrameInfo.byteCnt; i < 188; i++ )
                                                            {
                                                                packet[i] = 0xFF;
                                                            }                                
                                                        }
                                                    }
                                                }                                              
                                            }
                                        }
                                        else if ( m_backToBackPrev && (m_b2bSavedAudioPacketsCnt > 0 || m_b2bBackedAdAudioPacketsCnt > 0) && !audioPacketFromPrevLSAAd )
                                        {   
                                            m_b2bBackedAdAudioPacketsCnt++;
                                            unsigned char *adAudioPackets = m_b2bPrevAdAudioDataCopy + (m_b2bBackedAdAudioPacketsCnt -1)*188;   
                                            packet[1]= ((packet[1] & 0xE0) | ((mapPid>>8) & 0x1F));
                                            packet[2]= (mapPid & 0xFF);
                                            memcpy(adAudioPackets, packet, 188);
                                            backedupAudFramePktCnt = (adAC3FrameInfo.size/184);
                                            mapPid = 0x1FFF;
                                        }
                                    }
                                    if ( m_map->m_needPayloadStart[insertPid] )
                                    {
                                        int payloadStart= (packet[1] & 0x40);
                                        if ( payloadStart )
                                        {
                                            m_map->m_needPayloadStart[insertPid]= false;

                                            // Force a discontinuity
                                            ++m_continuityCounters[mapPid];
                                        }
                                        else
                                        {
                                            // map to null packet
                                            mapPid= 0x1FFF;
                                        }
                                    }
                                }
                                else
                                {
                                    // map to null packet
                                    mapPid= 0x1FFF;
                                }

                                packet[1]= ((packet[1] & 0xE0) | ((mapPid>>8) & 0x1F));
                                packet[2]= (mapPid & 0xFF);
                            }

                            // update continuity counters and timestamps
                            if ( mapPid != 0x1FFF )
                            {
                                byte3= packet[3];
                                if ( byte3 & 0x10 )
                                {
                                    ++m_continuityCounters[mapPid];
                                }
                                packet[3]= ((byte3 & 0xF0)|(m_continuityCounters[mapPid] & 0x0F));
                                reTimestamp( packet, m_packetSize );
                            }

                            if ( (!m_maxAdAudioFrameCountReached && m_audioReplicationEnabled && (insertPid == m_replicateAudioSrcPid) && (mapPid != 0x1FFF)) || (replicateAudioPacketFromPrevLSAAd) )
                            {
                               m_replicatingAudio= true;
                               m_replicateAudioIndex= 0;
                               memcpy( m_replicateAudioPacket, packet, m_packetSize-m_ttsSize );
                            }
                        }    
                    }
                    else
                    {
                       if ( !m_mapEndPending )
                       {
                           if ( m_insertSrc->getTotalRetryCount() >= MAX_TOTAL_READ_RETRY_COUNT )
                           {
                              m_events->insertionStatusUpdate( this, RBI_DetailCode_abnormalTermination );
                              m_mapSpliceInPTS= m_currentPTS;
                           }
                           else
                           {
                              long long now= getCurrentTimeMicro();
                              long long elapsed= (now-m_insertStartTime)/1000;
                              long long media= ((m_mapSpliceInPTS-m_mapSpliceOutPTS)&MAX_90K)/90LL;
                              if( elapsed < (media-RBI_DEFAULT_ASSET_DURATION_OFFSET) && !m_maxAdAudioFrameCountReached && !m_maxAdVideoFrameCountReached)
                              {
                                 INFO("hit end of insertion content: m_currentPTS %llx now(%lld) m_insertStartTime(%lld) elapsed: %lld (us) media: %lld (us)", 
                                       m_currentPTS, (now/1000), (m_insertStartTime/1000), elapsed, media);
                                 m_events->insertionStatusUpdate( this, RBI_DetailCode_assetDurationInadequate );
                                 m_mapSpliceInPTS= m_currentPTS;
                              }
                           }
                       }
                       m_mapEndPending= true;
                       INFO("m_mapEndPending is set to TRUE. hit end of insertion.");

                       // Convert to null packet
                       mapPid= 0x1FFF;
                       packet[0]= 0x47;
                       packet[1]= ((packet[1] & 0xE0) | ((mapPid>>8) & 0x1F));
                       packet[2]= (mapPid & 0xFF);
                    }
                }
            }

         long long start= (m_mapSpliceOutPTS-500*90LL)&MAX_90K;
         if ( !PTS_IN_RANGE( m_currentPTS, start, m_mapSpliceInPTS ) && m_insertSrc->getTotalRetryCount() < MAX_TOTAL_READ_RETRY_COUNT && !m_backToBackPrev )
         {
            int avail= m_insertSrc->avail();
            int bufferSize = RBIManager::getInstance()->getBufferSize();
            if(avail > bufferSize)
            {
               INFO("insertion content duration reached: m_currentPTS %llx. avail[%d] bufferSize[%d]", m_currentPTS, avail, bufferSize);
               m_events->insertionStatusUpdate( this, RBI_DetailCode_assetDurationExceeded );
               m_mapEndPending= true;
               INFO("m_mapEndPending is set to TRUE. insertion content duration reached.");
            }
         }

        if( !pktSkipped )
        {
            if ( m_ttsSize )
            {
               packet[-4]= ((m_ttsInsert>>24)&0xFF);
               packet[-3]= ((m_ttsInsert>>16)&0xFF);
               packet[-2]= ((m_ttsInsert>>8)&0xFF);
               packet[-1]= ((m_ttsInsert)&0xFF);

               if (packet[0] == 0x48) {
                   m_lastMappedPCRTTSValue = m_ttsInsert;
                   packet[0] = 0x47;
               }

               m_ttsInsert += ttsInc;
            }
            packet += m_packetSize;
        }
        else
        {
            pktSkipped = false;
        }

        if ( m_mapEndPending )
        {
           break;
        }
      }

      insertedSize= ((packet-m_ttsSize)-packets);

      if ( m_captureEnabled )
      {
         if ( m_outFile && (insertedSize > 0) )
         {
            fwrite( packets, 1, insertedSize, m_outFile );
         }
      }
      DEBUG("RBIStreamProcessor::getInsertionData: return %d bytes", insertedSize );
   }

   m_insertByteCount += insertedSize;

   end= getCurrentTimeMicro();
   m_lastInsertGetDataDuration= end-start;
   setAudioInsertCount( audioInsertCount );

   if ( m_mapEnding && !m_mapEndingNeedVideoEOF )
   {
      INFO("map ending: done inserting Ad data: total %d bytes", m_insertByteCount);
      m_mapEnding= false;
   }

   return insertedSize;
}


bool RBIStreamProcessor::isNextPacketAudio() {
    bool nextPacketAudio = false;

    int insertPid = m_insertSrc->getNextAdPacketPid();

    int mapPid = m_map->m_pidMap[insertPid];
    if (mapPid == m_audioPid) {
        nextPacketAudio = true;
    }

    return nextPacketAudio;
}

int RBIStreamProcessor::getInsertionDataDuringTransitionToAd(unsigned char *networkPacket) {
    int insertedSize = 0;
    unsigned char *packet;
    int mapPid = 0x1FFF;
    unsigned char byte3;
    long long start, end;
    bool findAdVideoPacket = true;

    start = getCurrentTimeMicro();

    if (m_insertSrc) {
        packet = networkPacket;
        while(findAdVideoPacket)
        {
            if (!isNextPacketAudio()) 
            {
                int lenDidRead = m_insertSrc->read(packet, m_packetSize - m_ttsSize);
                if (lenDidRead == (m_packetSize - m_ttsSize)) 
                {
                    int insertPid = (((packet[1] << 8) | packet[2]) & 0x1FFF);
                    mapPid = m_map->m_pidMap[insertPid];
                    if (mapPid && mapPid == m_videoPid) {
                        int payloadStart= (packet[1] & 0x40);
                        if ( payloadStart )
                        {
                            ++m_adVideoFrameCount;
                        }                                                                                                   
                        if (m_map->m_needPayloadStart[insertPid]) {                                    
                            int payloadStart = (packet[1] & 0x40);
                            if (payloadStart) {
                                m_map->m_needPayloadStart[insertPid] = false;

                                // Force a discontinuity
                                ++m_continuityCounters[mapPid];
                            } else {
                                // map to null packet
                                mapPid = 0x1FFF;
                            }
                        }
                    } else {
                        mapPid = 0x1FFF;
                    }

                    packet[1] = ((packet[1] & 0xE0) | ((mapPid >> 8) & 0x1F));
                    packet[2] = (mapPid & 0xFF);

                    // update continuity counters
                    if (mapPid != 0x1FFF) {
                        byte3 = packet[3];
                        if (byte3 & 0x10) {
                            ++m_continuityCounters[mapPid];
                        }
                        packet[3] = ((byte3 & 0xF0) | (m_continuityCounters[mapPid] & 0x0F));
                        reTimestampDuringTransitionToAd(packet, m_packetSize);
                        findAdVideoPacket = false;
                    }
                    else
                    {
                        if(!m_removeNull_PSIPackets)
                        {
                            findAdVideoPacket = false;
                        }
                    }    
                }
                else {
                    if (!m_mapEndPending) {
                            if ( m_insertSrc->getTotalRetryCount() >= MAX_TOTAL_READ_RETRY_COUNT )
                            {
                                m_events->insertionStatusUpdate( this, RBI_DetailCode_abnormalTermination );
                                m_mapSpliceInPTS= m_currentPTS;
                            }
                            else
                            {
                                long long now = getCurrentTimeMicro();
                                long long elapsed = now - m_insertStartTime;
                                long long media= ((m_mapSpliceInPTS-m_mapSpliceOutPTS)&MAX_90K)/90LL;
                                if( elapsed < (media-RBI_DEFAULT_ASSET_DURATION_OFFSET))
                                {
                                    INFO("In getInsertionDataDuringTransitionToAd, hit end of insertion content: m_currentPTS %llx now(%lld) m_insertStartTime(%lld) elapsed: %lld (us) media: %lld (us)", 
                                          m_currentPTS, (now/1000), (m_insertStartTime/1000), elapsed, media);
                                    m_events->insertionStatusUpdate( this, RBI_DetailCode_assetDurationInadequate );
                                    m_mapSpliceInPTS= m_currentPTS;
                                }                        
                            }                        
                      }
                    m_mapEndPending = true;
                    INFO("In getInsertionDataDuringTransitionToAd, m_mapEndPending is set to TRUE. hit end of insertion.");

                    // Convert to null packet
                    mapPid = 0x1FFF;
                    packet[0] = 0x47;
                    packet[1] = ((packet[1] & 0xE0) | ((mapPid >> 8) & 0x1F));
                    packet[2] = (mapPid & 0xFF);

                    break;
                }
            }    
            else 
            {
                insertedSize = -1;
                break;
            }
        }
    }

    long long inPtsStart= (m_mapSpliceOutPTS-500*90LL)&MAX_90K;
    if ( !PTS_IN_RANGE( m_currentPTS, inPtsStart, m_mapSpliceInPTS ) && m_insertSrc->getTotalRetryCount() < MAX_TOTAL_READ_RETRY_COUNT)
    {
       int avail= m_insertSrc->avail();
       int bufferSize = RBIManager::getInstance()->getBufferSize();
       if(avail > bufferSize)
       {
          INFO("In getInsertionDataDuringTransitionToAd, insertion content duration reached: m_currentPTS %llx. avail[%d] bufferSize[%d]", m_currentPTS, avail, bufferSize);
          m_events->insertionStatusUpdate( this, RBI_DetailCode_assetDurationExceeded );
          m_mapEndPending= true;
          INFO("In getInsertionDataDuringTransitionToAd, m_mapEndPending is set to TRUE. insertion content duration reached.");
       }
    }
         
    packet += m_packetSize;

    if (insertedSize != -1)
    {
        insertedSize = ((packet - m_ttsSize) - networkPacket);
        m_insertByteCount += insertedSize;
    }
    
    end = getCurrentTimeMicro();
    m_lastInsertGetDataDuration = end - start;
   
   if ( m_mapEnding && !m_mapEndingNeedVideoEOF )
   {
      INFO("In getInsertionDataDuringTransitionToAd, map ending: done inserting Ad data: total %d bytes", m_insertByteCount);
      m_mapEnding= false;
    }
    
    return insertedSize;
}

void RBIStreamProcessor::reTimestampDuringTransitionToAd(unsigned char *packet, int length) {
    long long timeOffset, adjustedPCR;

    if (!m_haveBaseTime) {
        m_haveBaseTime = true;
        m_baseTime = m_insertSrc->startPTS();
        m_basePCR = m_insertSrc->startPCR();        
        m_segmentBaseTime = (m_mapStartPTS != -1LL) ? m_mapStartPTS : m_currentPTS;
        m_segmentBasePCR = (m_mapStartPCR != -1LL) ? m_mapStartPCR : m_currentPCR;
        m_currentMappedPCR = m_segmentBasePCR;
        DEBUG("In reTimestampDuringTransitionToAd m_baseTime %llx m_basePCR %llx m_segmentBaseTime %llx m_mapStartPTS %llx m_currentPTS %llx m_mapStartPCR %llx m_currentPCR %llx ", m_baseTime, m_basePCR, m_segmentBaseTime, m_mapStartPTS, m_currentPTS, m_mapStartPCR, m_currentPCR);
    }

    for (int i = 0; i < length; i += m_packetSize) {
        int payloadStart = (packet[1] & 0x40);
        int adaptation = ((packet[3] & 0x30) >> 4);
        int payload = 4;
        int pid;

        pid= (((packet[1] << 8) | packet[2]) & 0x1FFF);

        // Update PCR values
        if (adaptation & 0x02) {
            int adaptationFieldLength = packet[4];
            int adaptationFlags = packet[5];
            if ( (adaptationFieldLength != 0) &&  (adaptationFlags & 0x10) ) {
                    m_lastAdPCR = readPCR(&packet[6]);                       
                     unsigned int tts = ((packet[-4] << 24) | (packet[-3] << 16) | (packet[-2] << 8) | packet[-1]);
                    if (m_currentPCR != -1) {                        
                        m_currentMappedPCR = (m_currentPCR + ((tts - m_lastPCRTTSValue) / 300)) & MAX_90K;
                    } else {
                        m_currentInsertPCR = m_lastAdPCR;
                        timeOffset = m_lastAdPCR - m_basePCR;
                        adjustedPCR = (((long long) (timeOffset) + m_segmentBasePCR)&0x1FFFFFFFFLL);
                        if (adjustedPCR < m_mapStartPCR) {
                            m_mapStartPCR = (m_mapStartPCR + 10)&0x1FFFFFFFFLL;
                            DEBUG("RBIStreamProcessor::reTimestampDuringTransitionToAd: rateAdjustedPCR clamped from %llx to %llx", adjustedPCR, m_mapStartPCR);
                            adjustedPCR = m_mapStartPCR;
                        }
                        m_currentMappedPCR = adjustedPCR;
                    }                  
                    writePCR(&packet[6], m_currentMappedPCR);
                    m_lastMappedPCRTTSValue = tts;
                }
                payload += (1 + packet[4]);
            }

            // Update PTS/DTS values
            if (payloadStart) {
                if ((packet[payload] == 0x00) && (packet[payload + 1] == 0x00) && (packet[payload + 2] == 0x01)) {
                    int tsbase = payload + 9;
                    long long timeOffset;
                    long long PTS = 0, DTS = 0;
                    long long adjustedPTS = 0, adjustedDTS = 0;

                    if (packet[payload + 7] & 0x80) {
                        PTS = readTimeStamp(&packet[tsbase]);
                        timeOffset = PTS - m_baseTime;
                        adjustedPTS = (((long long) (timeOffset) + m_segmentBaseTime)&0x1FFFFFFFFLL);
                        writeTimeStamp(&packet[tsbase], packet[tsbase] >> 4, adjustedPTS);
                        tsbase += 5;
                    }
                    if (packet[payload + 7] & 0x40) {
                        DTS = readTimeStamp(&packet[tsbase]);
                        timeOffset = DTS - m_baseTime;
                        adjustedDTS = (((long long) (timeOffset) + m_segmentBaseTime)&0x1FFFFFFFFLL);
                        writeTimeStamp(&packet[tsbase], packet[tsbase] >> 4, adjustedDTS);
                        tsbase += 5;
                    }
                                       
                    if(adjustedDTS == 0)
                    {
                        adjustedDTS = adjustedPTS;
                        DTS = PTS;
                    }

                    if( m_lastAdPCR != -1)
                    {                                            
                        double requiredStartupDelay = (DTS - m_lastAdPCR)/90000.0;
                    
                        double currentStartupDelay = (adjustedDTS - m_currentMappedPCR)/90000.0;
                        if( currentStartupDelay < requiredStartupDelay ) 
                        { 
                            DEBUG("RBIStreamProcessor::reTimestampDuringTransitionToAd: replacing Null and PSI packets currentStartupDelay %f requiredStartupDelay %f", currentStartupDelay, requiredStartupDelay);                            
                            m_removeNull_PSIPackets = true;
                        }
                    }
                        
                    if (packet[payload + 7] & 0x02) {
                        // CRC flag is set.  Following the PES header will be the CRC for the previous PES packet
                        WARNING("Warning: PES packet has CRC flag set");
                    }                    
                }
            }
 
        packet += m_packetSize;
    }
}

int RBIStreamProcessor::getInsertionAudioDataDuringTransitionBackToNetwork(unsigned char *packets) {
    int insertedSize = 188;
    unsigned char *packet;
    int mapPid;
    unsigned char byte3;

    
    if (m_insertSrc) {
        packet = packets;

        bool audioPacketFound = false;
        bool endOfAdStream = false;

        if ( m_maxAdAudioFrameCountReached )
        {
            insertedSize = -1;                                        
        }

        if ( m_audioReplicationEnabled && m_replicatingAudio )
        {
           memcpy( packet, m_replicateAudioPacket, m_packetSize-m_ttsSize );
           mapPid= m_replicateAudioTargetPids[m_replicateAudioIndex];
           packet[1]= ((packet[1] & 0xE0) | ((mapPid>>8) & 0x1F));
           packet[2]= (mapPid & 0xFF);
           ++m_replicateAudioIndex;
           if ( m_replicateAudioIndex >= m_replicateAudioTargetPids.size() )
           {
              m_replicatingAudio= false;
           }
        } 
        else
        {
            while (!audioPacketFound && !endOfAdStream &&  !m_maxAdAudioFrameCountReached && (insertedSize != -2) ) {
                int lenDidRead = m_insertSrc->read(packet, m_packetSize - m_ttsSize);
                if (lenDidRead == (m_packetSize - m_ttsSize)) {
                    int insertPid = (((packet[1] << 8) | packet[2]) & 0x1FFF);
                    mapPid = m_map->m_pidMap[insertPid];

                    if (mapPid && mapPid == m_audioPid) {
                        if(m_determineAdAC3FrameSizeDuration){
                            int payloadOffset = 4;
                            int adaptation = ((packet[3] & 0x30) >> 4);                            

                            if (adaptation & 0x02) {
                                payloadOffset += (1 + packet[4]);
                            }
                            int payloadStart = (packet[1] & 0x40);
                            if ((adaptation & 0x01) && payloadStart && ((packet[payloadOffset] == 0x00) && (packet[payloadOffset + 1] == 0x00) && (packet[payloadOffset + 2] == 0x01))) {
                                payloadOffset += (9 + packet[payloadOffset + 8]);
                                getAC3FrameSizeDuration( packet, payloadOffset, &adAC3FrameInfo );
                                if( adAC3FrameInfo.duration != -1 && adAC3FrameInfo.size != -1)
                                {
                                    m_determineAdAC3FrameSizeDuration = false;
                                }
                            }
                        }

                        if( !m_determineAdAC3FrameSizeDuration ){
                                long long audioPTS = getAC3FramePTS( packet, &adAC3FrameInfo, m_currentAdAudioPTS);
                            if (audioPTS != -1)
                            {
                                m_currentAdAudioPTS = audioPTS;

                                if (m_currentAdAudioPTS >= m_lastAdVideoFramePTS)
                                {
                                    DEBUG("Maximum Ad Audio frames reached Last Ad Video Frame PTS %llx Last Ad Audio Frame PTS %llx", m_lastAdVideoFramePTS, m_currentAdAudioPTS); 
                                    m_maxAdAudioFrameCountReached = true;
                                    for( int i = 188 - adAC3FrameInfo.byteCnt; i < 188; i++ )
                                    {
                                        packet[i] = 0xFF;
                                    }                                
                                }
                            }
                                }

                        packet[1] = ((packet[1] & 0xE0) | ((mapPid >> 8) & 0x1F));
                        packet[2] = (mapPid & 0xFF);

                        if ( mapPid != 0x1FFF )
                        {
                            byte3 = packet[3];
                            if (byte3 & 0x10) {
                                ++m_continuityCounters[mapPid];
                            }
                            packet[3] = ((byte3 & 0xF0) | (m_continuityCounters[mapPid] & 0x0F));
                            reTimestampDuringTransitionBack(packet, m_packetSize - m_ttsSize, 0, 0);
                        }

                        audioPacketFound = true;

                        if ( !m_maxAdAudioFrameCountReached && m_audioReplicationEnabled && m_replicateAudioSrcPid != -1 )
                        {
                           m_replicatingAudio= true;
                           m_replicateAudioIndex= 0;
                           memcpy( m_replicateAudioPacket, packet, m_packetSize-m_ttsSize );
                        }
                    }
                } else {
                    endOfAdStream = true;
                    insertedSize = -1;
                }
            }
        }
    }
    return insertedSize;
}

void RBIStreamProcessor::reTimestampDuringTransitionBack(unsigned char *packet, int length, long long bufferStartPCR, unsigned int bufferStartPCRTTSValue) {
    //TRACE1("RBIStreamProcessor::reTimestamp: packet %p length %d", packet, length );
    for (int i = 0; i < length; i += m_packetSize) {
        int payloadStart = (packet[1] & 0x40);
        int adaptation = ((packet[3] & 0x30) >> 4);
        int payload = 4;
        int pid;

        pid = (((packet[1] << 8) | packet[2]) & 0x1FFF);

        {
            // Update PCR values
            if (adaptation & 0x02) {
                int adaptationFlags = packet[5];
                if (adaptationFlags & 0x10) {
                }
                payload += (1 + packet[4]);
            }

            // Update PTS/DTS values
            if (payloadStart) {
                if ((packet[payload] == 0x00) && (packet[payload + 1] == 0x00) && (packet[payload + 2] == 0x01)) {
                    int pesHeaderDataLen = packet[payload + 8];
                    int tsbase = payload + 9;
                    long long timeOffset;
                    long long PTS = 0, DTS = 0;
                    long long rateAdjustedPTS = 0, rateAdjustedDTS = 0;

                    if (packet[payload + 7] & 0x80) {
                        PTS = readTimeStamp(&packet[tsbase]);
                        timeOffset = PTS - m_baseTime;
                        rateAdjustedPTS = (((long long) (timeOffset) + m_segmentBaseTime)&0x1FFFFFFFFLL);
                        writeTimeStamp(&packet[tsbase], packet[tsbase] >> 4, rateAdjustedPTS);
                        if (pid == m_pcrPid) {
                            m_currentMappedPTS = rateAdjustedPTS;
                        }
                        tsbase += 5;
                    }
                    if (packet[payload + 7] & 0x40) {
                        DTS = readTimeStamp(&packet[tsbase]);
                        timeOffset = DTS - m_baseTime;
                        rateAdjustedDTS = (((long long) (timeOffset) + m_segmentBaseTime)&0x1FFFFFFFFLL);
                        writeTimeStamp(&packet[tsbase], packet[tsbase] >> 4, rateAdjustedDTS);
                        tsbase += 5;
                    }
                    if (rateAdjustedDTS == 0) {
                        rateAdjustedDTS = rateAdjustedPTS;
                        DTS = PTS;
                    }
                    if (packet[payload + 7] & 0x02) {
                        // CRC flag is set.  Following the PES header will be the CRC for the previous PES packet
                        WARNING("Warning: PES packet has CRC flag set");
                    }

                    payload = payload + 9 + pesHeaderDataLen;
                }
            }
        }
        packet += m_packetSize;
    }
}


int RBIStreamProcessor::processPackets( unsigned char* packets, int size, int *isTSFormatError )
{
   int acceptedSize= size;
   int copylen, inOffset, consume;
   int returnVal = 0;

   // The Loop flags are added to debug the issue if happend in the field.
   // Remove the flag if the issue not observed for a month or two.
   bool bfirstIfLoop = false,
        bSecondIfLoop = false,
        bThirdIfLoop = false,
        bFourthIfLoop = false,
        bFifthIfLoop = false;

   if ( !m_inSync && m_triggerEnable )
   {
      bfirstIfLoop = true;
      // If this processor is being used for insertion its input must be TS packet aligned.
      int ps, ts, ttsSize;
      bool sync= true;
      for( ps= 192; ps >= 188; ps -= 4 )
      {
         ts= 3*ps;         
         while( ts > size )
         {
            ts -= ps;
         }
         ttsSize= (ps == 192) ? 4 : 0;
         for( int i= ttsSize; i < ts; i += ps )
         {
            if ( packets[i] != 0x47 )
            {
               sync= false;
               break;
            }
         }
         if ( sync )
         {
            m_packetSize= ps;
            m_ttsSize= (ps == 192) ? 4 : 0;
            break;
         }
      }
      
      if ( !sync || ((size % m_packetSize) != 0) )
      {
         ERROR( "Input stream is not TS packet aligned - no insertion will be performed" );
         m_triggerEnable= false;
      }
      
      INFO( "RBIStreamProcessor::processPackets: synced at packet offset 0 : packetSize %d bufferSize %d", m_packetSize, size );
      m_inSync= true;
   }
   
   inOffset= 0;
   if ( !m_inSync )
   {
      bSecondIfLoop = true;
      copylen= 3*MAX_PACKET_SIZE-m_remainderOffset;
      if ( copylen > size )
      {
         copylen= size;
      }
      memcpy( &m_remainder[m_remainderOffset], packets, copylen );
      m_remainderSize += copylen;
      m_remainderOffset += copylen;
      inOffset += copylen;
      INFO("m_remainderSize %d", m_remainderSize);
      if ( m_remainderSize >= 3*MAX_PACKET_SIZE )
      {
         bThirdIfLoop = true;
         int i= 0;
         while( i < m_remainderSize )
         {
            bFourthIfLoop = true;
            if( i < m_remainderSize/3 )
            {
               bFifthIfLoop = true;
               if ( (m_remainder[i] == 0x47) && (m_remainder[i+188] == 0x47) && (m_remainder[i+188*2] == 0x47) )
               {
                  m_inSync= true;
                  m_packetSize= 188;
                  m_ttsSize= 0;
                  INFO( "RBIStreamProcessor::processPackets: synced at packet offset %d : packetSize 188 m_remainderSize %d isTSFormatError %d", i, m_remainderSize, *isTSFormatError );
                  break;
               }
               if ( (m_remainder[i+4] == 0x47) && (m_remainder[i+4+192] == 0x47) && (m_remainder[i+4+192*2] == 0x47) )
               {
                  m_inSync= true;
                  m_packetSize= 192;
                  m_ttsSize= 4;
                  INFO( "RBIStreamProcessor::processPackets: synced at packet offset %d : packetSize 192 m_remainderSize %d isTSFormatError %d", i, m_remainderSize, *isTSFormatError );
                  break;
               }
            }
            ++i;
         }
         m_streamOffset += i;
         if ( m_inSync )
         {
            consume= m_remainderSize-i;
            consume -= (consume % m_packetSize);
            returnVal = processPacketsAligned( &m_remainder[i], consume, isTSFormatError );
            if( (returnVal == 0) && *isTSFormatError )
            {
                *isTSFormatError = *isTSFormatError + 1;
                INFO( "RBIStreamProcessor::processPackets: processPacketsAligned failed. m_remainder[%d] isTSFormatError %d", i, *isTSFormatError );
            }
            m_remainderSize -= (i+consume);
            m_remainderOffset= i+consume;
            if ( m_remainderSize > 0 )
            {
               memmove( m_remainder, m_remainder+m_remainderOffset, m_remainderSize );
            }
            m_remainderOffset= m_remainderSize;
         }
      }
   }
   if ( m_inSync )
   {
      *isTSFormatError = 0;
      if ( m_remainderSize )
      {
         copylen= m_packetSize-m_remainderSize;
         if ( copylen > size-inOffset )
         {
            copylen= size-inOffset;
         }
         memcpy( &m_remainder[m_remainderOffset], &packets[inOffset], copylen );
         m_remainderSize += copylen;
         inOffset += copylen;
         if ( m_remainderSize == m_packetSize )
         {
            returnVal = processPacketsAligned( m_remainder, m_packetSize, isTSFormatError );
            if( (returnVal == 0) && *isTSFormatError )
            {
                *isTSFormatError = *isTSFormatError + 1;
                INFO( "RBIStreamProcessor::processPackets: processPacketsAligned failed. m_remainderOffset %d ", m_remainderOffset );
            }
            m_remainderSize -= m_packetSize;
            m_remainderOffset= 0;    
         }
         else
         {
            m_remainderOffset += copylen;
         }
      }
      if ( inOffset < size )
      {
         consume= size-inOffset;
         consume -= (consume % m_packetSize);
         if ( consume )
         {
            acceptedSize= processPacketsAligned( &packets[inOffset], consume, isTSFormatError );
            if( (acceptedSize == 0) && *isTSFormatError )
            {
                *isTSFormatError = *isTSFormatError + 1;
                INFO( "RBIStreamProcessor::processPackets: processPacketsAligned failed. packets[%d] *isTSFormatError %d", inOffset, *isTSFormatError );
            }
            inOffset += consume;
         }
         if ( inOffset < size )
         {
            copylen= size-inOffset;
            memcpy( &m_remainder[m_remainderOffset], &packets[inOffset], copylen );
            m_remainderSize += copylen;
            m_remainderOffset += copylen;
         }
      }
   }
   else
   {
      INFO("m_inSync not found. isTSFormatError %d. size %d test_insert %d 1[%d] 2[%d] 3[%d] 4 [%d] 5[%d]",
            *isTSFormatError, size, test_insert, bfirstIfLoop, bSecondIfLoop, bThirdIfLoop, bFourthIfLoop, bFifthIfLoop );

      *isTSFormatError = *isTSFormatError + 1;
      if (!test_insert)
      {
         acceptedSize = 0;
      }
   }
   if( acceptedSize == 0 )
   {
      INFO("acceptedSize is zero. size is %d", size);
   }
   return acceptedSize;
}

int RBIStreamProcessor::processPacketsAligned( unsigned char* packets, int size, int *isTSFormatError  )
{
   int processedSize= 0;
   unsigned char *packet = NULL, *bufferEnd = NULL, *inputEnd = NULL;
   int pid, payloadStart = 0, adaptation = 0, payloadOffset = 0;
   int packetCount= 0;
   bool wasMapping= m_mapping;
   bool wasArmed= m_mapStartArmed;   
   int trimSize = 0;
   long long trimmedCurrentPCR =0; 
   unsigned int trimmedPCRTTSValue =0;
   unsigned char *lastAdAudioPacket;
   long long alternateVideoSplicePointExitFramePTS = -1;
   long long alternateVideoSplicePointPTS = -1;
    
   
   lastAdAudioPacket = 0;
   
    // Initialize pointers to Network and AC3 PES packet length field to null
   adAC3FrameInfo.pesPacketLengthMSB = 0;
   adAC3FrameInfo.pesPacketLengthLSB = 0;
   netAC3FrameInfo.pesPacketLengthMSB = 0;
   netAC3FrameInfo.pesPacketLengthLSB = 0;
  

   packet= packets+m_ttsSize;

   // Insist on buffers being TS packet aligned
   if ( !((packet[0] == 0x47) && ((size%m_packetSize) == 0)) )
   {
      ERROR( "RBIStreamProcessor::processPacketsAligned: Error: buffer not TS packet aligned packet[0] = 0x%X size = %d m_packetSize= %d",packet[0],size,m_packetSize );
      *isTSFormatError = TS_FORMATERROR_RETRYCOUNT;
      return 0;
   }   
   else
      *isTSFormatError = 0;

   inputEnd= packets+size;
   bufferEnd= packets+size;

   if ( m_mapping && m_mapStartArmed && m_ttsSize && (size >= 2*m_packetSize) )
   {
      unsigned int ttsNew;
      int ttsDiff;
      ttsNew= ((packet[2*m_packetSize-4]<<24)|(packet[2*m_packetSize-3]<<16)|(packet[2*m_packetSize-2]<<8)|packet[2*m_packetSize-1]);
      ttsDiff= ttsNew-m_ttsInsert;
      TRACE3("network audio tts gap between buffers %d", ttsDiff);
      if ( ttsDiff < 22000 )
      {
         TRACE3("small TTS gap between network audio buffers %d : expect possible audio underflow", ttsDiff);
      }
   }
   
   // Do a prescan of Splice Points
   if (PTS_IN_RANGE( m_currentPTS, (m_mapSpliceInPTS - 90000), m_mapSpliceInAbortPTS) && !m_filterAudio && m_insertSrc)
   {
        long long videoPTS = 0;
        long long prevVideoPTS = -1;
        long long prevDiffPTS = MAX_90K;

        int adaptationLen= -1;
        int adaptationFlags= -1;
        while( packet < bufferEnd )
        {
            pid= (((packet[1] << 8) | packet[2]) & 0x1FFF);
            if ( pid == m_videoPid )
            {
                adaptation= ((packet[3] & 0x30)>>4);
                payloadStart= (packet[1] & 0x40);
                payloadOffset= 4;
                if ( adaptation & 0x02 )
                {
                    payloadOffset += (1+packet[4]);
                }
                if ( adaptation & 0x01 )
                {
                    if ( payloadStart )
                    {
                        if ( (packet[payloadOffset] == 0x00) && (packet[payloadOffset+1] == 0x00) && (packet[payloadOffset+2] == 0x01) )
                        {
                            int streamid= packet[payloadOffset+3];
                            if ( (streamid >= 0xE0) && (streamid <= 0xEF) )
                            {
                                videoPTS = 0;
                                m_frameType = 0;
                                int pesHeaderDataLen= packet[payloadOffset+8];                                
                                if ( packet[payloadOffset+7] & 0x80 )
                                {     
                                   videoPTS = readTimeStamp( &packet[payloadOffset+9] );     
                                }   
                                payloadOffset= payloadOffset+9+pesHeaderDataLen;   
                                m_prescanForFrameType = true;
                                adaptationLen= packet[4];
                                adaptationFlags= packet[5];
                            }
                        }
                    } 
                    // Scan for Frame type
                    if ( m_prescanForFrameType )
                    {
                        int i, imax;

                        if ( m_prescanHaveRemainder )
                        {
                            unsigned char *remainder= m_prescanRemainder;

                            memcpy( remainder+RBI_SCAN_REMAINDER_SIZE, packet+payloadOffset, RBI_SCAN_REMAINDER_SIZE );

                            for( i= 0; i < RBI_SCAN_REMAINDER_SIZE; ++i )
                            {
                                if ( (remainder[i] == 0x00) && (remainder[i+1] == 0x00) && (remainder[i+2] == 0x01) )
                                {
                                   processStartCode( &remainder[i], m_prescanForFrameType, 2*RBI_SCAN_REMAINDER_SIZE-i, 0 );
                                }
                            }

                           m_prescanHaveRemainder= false;
                        }

                        if ( m_prescanForFrameType )
                        {
                           // If we are at the last packet in the buffer we need to stop scanning at the point
                           // where we can no longer access all needed start code bytes.
                           //imax= ((packet+m_packetSize < inputEnd) ? m_packetSize : m_packetSize-RBI_SCAN_REMAINDER_SIZE)-m_ttsSize;
                           imax= m_packetSize-RBI_SCAN_REMAINDER_SIZE-m_ttsSize;

                            for( i= payloadOffset; i < imax; ++i )
                            {
                               if ( (packet[i] == 0x00) && (packet[i+1] == 0x00) && (packet[i+2] == 0x01) )
                                {
                                    processStartCode( &packet[i], m_prescanForFrameType, imax-i, i );

                                    if ( !m_prescanForFrameType )
                                    {
                                       break;
                                    }
                                }
                            }

                            if ( imax != m_packetSize-m_ttsSize )
                            {
                                m_prescanHaveRemainder= true;
                                memcpy( m_prescanRemainder, packet+m_packetSize-RBI_SCAN_REMAINDER_SIZE-m_ttsSize, RBI_SCAN_REMAINDER_SIZE );
                            }
                        }
                        if ( m_frameType != 0 && (m_transitioningToAd || m_mapping) )
                        {
                            if( m_frameType ==1 && videoPTS != 0 ) 
                            {
                                long long diffPTS = abs(m_mapSpliceInPTS - videoPTS);
                                if( diffPTS < prevDiffPTS )
                                {
                                    prevDiffPTS = diffPTS;    
                                    if ( (adaptationLen > 0) && adaptationFlags & 0x40 )
                                    {
                                        // Random access point
                                        DEBUG("Random access point detected, videoPTS %llx Splice In PTS %llx", videoPTS, m_mapSpliceInPTS );
                                        if ( PTS_IN_RANGE( videoPTS, (m_mapSpliceInPTS - (8*90000/VIDEO_FRAME_RATE)), m_mapSpliceInAbortPTS ) )
                                        {
                                            DEBUG("Detected Alternate Video Splice point frame PTS: %llx  Exit frame PTS %llx ", videoPTS, prevVideoPTS);
                                            alternateVideoSplicePointExitFramePTS = prevVideoPTS;
                                            alternateVideoSplicePointPTS = videoPTS;
                                            if(!m_insertSrc && prevVideoPTS == -1) // For Fixed spots
                                            {
                                                DEBUG("Stopping Fixed spot tracking at Outpoint detection, I frame is the first frame in the buffer, Outpoint Frame PTS %llX ", alternateVideoSplicePointPTS);
                                                m_mapSpliceUpdatedOutPTS = alternateVideoSplicePointPTS;
                                                stopInsertion();
                                            }
                                        }                                
                                    }
                                }
                            }   
                            adaptationLen= -1;
                            adaptationFlags= -1;
                            prevVideoPTS = videoPTS;
                        }                     
                    } // Done scanning for frame type
                }               
            }
            packet += m_packetSize;
        }
        if (alternateVideoSplicePointPTS != -1)
        {        
            if ( m_insertSrc )
            {
                INFO("Modifying splice in point PTS for replace spot %llx previous PTS %llx", alternateVideoSplicePointPTS, m_mapSpliceInPTS );
                m_mapSpliceInPTS = alternateVideoSplicePointPTS;
            }
        }
   }

   m_frameType = 0;
   
   packet= packets+m_ttsSize;
   while( packet < bufferEnd )
   {
      if ( packet < inputEnd )
      {
         pid= (((packet[1] << 8) | packet[2]) & 0x1FFF);

         if ( m_trackContinuityCountersEnable )
         {
            m_continuityCounters[pid]= (packet[3]&0x0F);
         }

         if ( pid == 0 )
         {
            payloadStart= (packet[1] & 0x40);
            if ( payloadStart )
            {
               adaptation= ((packet[3] & 0x30)>>4);
               if (adaptation & 0x01)
               {
                  payloadOffset = 4;
                  if (adaptation & 0x02)
                  {
                     payloadOffset += (1 + packet[4]);
                  }
                  int tableid = packet[payloadOffset+1];   
                  if ( tableid == 0x00 )
                  {
                     int version= packet[payloadOffset+6];
                     int current= (version & 0x01);
                     version= ((version >> 1)&0x1F);
                     
                     if ( !m_havePAT || (current && (version != m_versionPAT)) )
                     {
                        dumpPacket( packet, m_packetSize );
                        int length = ((packet[payloadOffset+2] & 0x0F) << 8) + (packet[payloadOffset+3]);
                        if ( length == 13 )
                        {
                           m_versionPAT= version;
                           m_program = (packet[payloadOffset+9]<<8)+(packet[payloadOffset+10]);
                           m_pmtPid= (((packet[payloadOffset+11]&0x1F)<<8)+packet[payloadOffset+12]);
                           if ( (m_program != 0) && (m_pmtPid != 0) )
                           {
                              if ( m_havePMT )
                              {
                                 INFO( "RBIStream: pmt change detected in pat" );
                              }
                              m_havePMT= false;
                              DEBUG("RBIStream: acquired PAT version %d program %X pmt pid %X", version, m_program, m_pmtPid );
                              
                              m_havePAT= true;
                              memcpy( m_pat, packet-m_ttsSize, m_packetSize );
                              if ( m_events )
                              {
                                 m_events->acquiredPAT( this );
                              }
                           }
                           else
                           {
                              WARNING( "Warning: RBIStream: ignoring pid 0 TS packet with suspect program %x and pmtPid %x", m_program, m_pmtPid );
                              dumpPacket( packet, m_packetSize );
                              m_program= -1;
                              m_pmtPid= -1;
                           }
                        }
                        else
                        {
                           WARNING( "Warning: RBIStream: ignoring pid 0 TS packet with length of %d", length );
                        }
                     }
                  }
                  else
                  {
                     WARNING( "Warning: RBIStream: ignoring pid 0 TS packet with tableid of %x", tableid );
                  }
               }
               else
               {
                  WARNING( "Warning: RBIStream: ignoring pid 0 TS packet with adaptation of %x", adaptation );
               }
            }
            else
            {
               WARNING( "Warning: RBIStream: ignoring pid 0 TS without payload start indicator" );
            }
         }
         else if ( pid == m_pmtPid )
         {
             adaptation= ((packet[3] & 0x30)>>4);
             if ( adaptation & 0x01 ) //Proceed only if there is a payload.
             {
                 payloadOffset = 4;
                 if( adaptation & 0x02 )
                 {
                     payloadOffset += (1+packet[4]);
                 }

                 payloadStart = (packet[1] & 0x40);
                 if ( payloadStart )
                 {
                     //TODO: fix limitation -> pointer field is assumed to be zero. If it isn't, parser will behave in an undefined manner.
                     int tableid= packet[payloadOffset+1];
                     if ( tableid == 0x02 )
                     {
                         int program= ((packet[payloadOffset+4] <<8)+packet[payloadOffset+5]);
                         if ( program == m_program )
                         {
                             int version= packet[payloadOffset+6];
                             int current= (version & 0x01);
                             version= ((version >> 1)&0x1F);

                             if ( !m_havePMT || (current && (version != m_versionPMT)) )
                             {
                                 dumpPacket( packet, m_packetSize );

                                 if ( m_havePMT && (version != m_versionPMT) )
                                 {
                                     INFO( "RBIStream: pmt change detected: version %d -> %d", m_versionPMT, version );
                                     m_havePMT= false;
                                     if(m_events)
                                     {
                                        if(m_events->getTriggerDetectedForOpportunity())
                                        {
                                           m_events->insertionStatusUpdate( this, RBI_DetailCode_patPmtChanged );
                                        }
                                     }
                                 }
                                 m_versionPMT = version;

                                 if ( !m_havePMT )
                                 {
                                     int sectionLength= (((packet[payloadOffset+2]&0x0F)<<8)+packet[payloadOffset+3]);
                                     tableInfo_t * ptr = &m_tableAggregator[TABLE_AGGREGATOR_INDEX_PMT];

                                     /*Update local copies of table to use for insertion*/
                                     memcpy( ptr->m_packets, packet-m_ttsSize, m_packetSize );
                                     ptr->m_packetWriteOffset = m_packetSize;

                                     if ( sectionLength <= (m_packetSize - m_ttsSize - TS_HEADER_SIZE - POINTER_FIELD_SIZE - PSI_TABLE_HEADER_SIZE))
                                     {
                                         processPMTSection(&packet[payloadOffset], sectionLength);
                                         m_havePMT= true;
                                         if ( m_events )
                                         {
                                             m_events->acquiredPMT( this );
                                         }
                                     }
                                     else
                                     {
                                         INFO( "RBIStream: processing multi-packet pmt (section length %d)", sectionLength );
                                         if(!ptr->m_section)
                                         {
                                             ptr->m_section = new unsigned char [MAX_PMT_SECTION_SIZE];
                                             DEBUG( "RBIStream: allocating buffer for PMT section." );
                                         }

                                         /*The contents of the section buffer is ptr_field + section start offset + section. In other words,
                                          * what remains of the packet after TTS, TS header and adaptation field are removed. */
                                         int dataLength = m_packetSize - m_ttsSize - payloadOffset; 
                                         rmf_osal_memcpy(ptr->m_section, &packet[payloadOffset], dataLength, MAX_PMT_SECTION_SIZE, dataLength);
                                         ptr->m_sectionLength = sectionLength;
                                         ptr->m_sectionWriteOffset = dataLength;
                                         ptr->m_nextContinuity =  ((packet[3]+1)&0xF);

                                     }
                                 }
                             }                     
                         }
                         else
                         {
                             WARNING( "Warning: RBIStream: ignoring pmt TS packet with mismatched program of %x (expecting %x)", program, m_program );
                         }                  
                     }
                     else
                     {
                         TRACE1( "Warning: RBIStream: ignoring pmt TS packet with tableid of %x", tableid );
                     }
                 }
                 else
                 {
                     TRACE1( "RBIStream: processing pmt TS without payload start indicator" );
                     tableInfo_t *ptr  = &m_tableAggregator[TABLE_AGGREGATOR_INDEX_PMT];
                     if(0 != ptr->m_sectionWriteOffset)
                     {
                         unsigned int continuity = (packet[3]&0xF);
                         if ( ((continuity+1)&0xF) == ptr->m_nextContinuity )
                         {
                             WARNING( "Warning: RecordContext: next packet of multi-packet pmt has wrong continuity count %d (expecting %d)",
                                     ptr->m_nextContinuity, continuity );
                             // Assume continuity counts for all packets of this pmt will remain the same.
                             // Allow this since it has been observed in the field
                             ptr->m_nextContinuity = continuity;
                         }
                         if( continuity == ptr->m_nextContinuity )
                         {
                             if(MAX_PMT_PACKET_BUFFER_SIZE >= (ptr->m_packetWriteOffset + m_packetSize))
                             {
                                 memcpy( &(ptr->m_packets[ptr->m_packetWriteOffset]), packet-m_ttsSize, m_packetSize );
                                 ptr->m_packetWriteOffset += m_packetSize;
                             }
                             else
                             {
                                 WARNING("Dropping PMT packet to avoid buffer overrun.");
                             }
                             int sectionRemaining = ptr->m_sectionLength + 4 /*headers and pointer field*/- ptr->m_sectionWriteOffset;
                             int payloadSize = m_packetSize - m_ttsSize - payloadOffset;

                             /* Copy the complete payload of this packet or the remainder of the section, whichever is less*/
                             int copyLength = ( sectionRemaining < payloadSize ? sectionRemaining : payloadSize );
                             TRACE1("Section remaining: %d, copyLength: %d", sectionRemaining, copyLength);

                             int spaceRemaining = MAX_PMT_SECTION_SIZE - ptr->m_sectionWriteOffset;
                             /* The buffer we have is limited in size. Limit copyLength so that it doesn't overshoot its boundaries. */
                             if(copyLength > spaceRemaining)
                             {
                                 WARNING("Truncating section data to stay within allocated buffer limits.");
                                 copyLength = spaceRemaining;
                             }
                             rmf_osal_memcpy(&(ptr->m_section[ptr->m_sectionWriteOffset]), &packet[payloadOffset], copyLength,
                                     spaceRemaining, payloadSize);
                             ptr->m_sectionWriteOffset += copyLength;

                             /*Evaluate whether a full section is now available. Process it.*/
                             TRACE1("RBIStream: section write offset = %d", ptr->m_sectionWriteOffset);
                             if( ptr->m_sectionWriteOffset == (ptr->m_sectionLength + 4) )//TODO: Fix this condition check. It would fail if pointer_field is non-zero 
                             {
                                 DEBUG("RBIStream: complete PMT section is now available. Parsing begins.");
                                 processPMTSection(ptr->m_section, ptr->m_sectionLength);
                                 ptr->m_sectionWriteOffset = 0; //To stop processing any more of these packets until there is a version change.
                                 m_havePMT= true;
                                 if ( m_events )
                                 {
                                     m_events->acquiredPMT( this );
                                 }
                             }
                             else
                             {
                                 ptr->m_nextContinuity = ((continuity+1)&0xF);
                             }

                         }
                         else
                         {
                             WARNING("RBIStream: not processing multi-packet PMT because of discontinuity.");
                             ptr->m_sectionWriteOffset = 0;
                         }

                     }
                     else
                     {
                         TRACE1("RBIStream: ignoring PMT packet until we see payload start.");
                     }
                 }
             }
             else
             {
                 TRACE1( "Warning: RBIStream: ignoring pmt TS packet with adaptation of %x", adaptation );
             }
         }
         else if ( m_triggerEnable && m_triggerPids[pid] )
         {
            payloadStart= (packet[1] & 0x40);
            if ( payloadStart )
            {
               adaptation= ((packet[3] & 0x30)>>4);
               if ( adaptation & 0x01 )
               {
                  payloadOffset= 4;
                  if ( adaptation & 0x02 )
                  {
                     payloadOffset += (1+packet[4]);
                  }
                  #ifdef USE_DVB
                  if ( (packet[payloadOffset] == 0x00) && (packet[payloadOffset+1] == 0x00) && (packet[payloadOffset+2] == 0x01) )
                  {
                     int streamid= packet[payloadOffset+3];
                     if (  streamid == 0xBD )
                     {
                        int pesPacketLength= (packet[payloadOffset+4]<<8)+packet[payloadOffset+5];
                        int pesHeaderLength= packet[payloadOffset+8];
                        
                        payloadOffset= payloadOffset+9+pesHeaderLength;
                        
                        // Parse DVB-SAD: ETSI TS 102 823 V1.1.1
                        //
                        // format 0x01 means payload consists of claus 5 descriptors
                        if ( (packet[payloadOffset] >> 4) == 0x01 )
                        {
                           // PES payload length is PES data length less 3 (2 bytes flags, 1 byte header length)
                           // less PES header length
                           int pesDataLength= pesPacketLength-3-pesHeaderLength;
                           int imax= payloadOffset+pesDataLength;

                           // Limit support to triggers that fit within a single TS packet
                           if ( imax <= m_packetSize-m_ttsSize )
                           {
                              // Parse descriptors
                              for( int i= payloadOffset+1; i < imax-1; ++i )
                              {
                                 int descId= packet[i];
                                 int descLen= packet[i+1];
                                 switch( descId )
                                 {
                                    // TVA id descriptor
                                    case 0x01:
                                    // Broadcast timeline descriptor
                                    case 0x02:
                                    // Time base mapping descriptor
                                    case 0x03:
                                    // Content labelling descriptor
                                    case 0x04:
                                    // Synchronized event cancel descriptor
                                    case 0x06:
                                       DEBUG( "RBIStream: ignoring DVB-SAD unsupported descriptor: %X len %d offset %X", descId, descLen, i );
                                       break;
                                    // Synchronized event descriptor
                                    case 0x05:
                                       {
                                          INFO( "RBIStream: detected DVB-SAD synchronized event descriptor: len %d", descLen );
                                          
                                          if ( descLen >= 9 )
                                          {
                                             int eventContext= (packet[i+2]&0xFF);
                                             int eventId= (((unsigned int)packet[i+3])<<8)|(packet[i+4]&0xFF);
                                             int eventIdInstance= (packet[i+5]&0xFF);
                                             int tickFormat= (packet[i+6]&0x3F);
                                             int referenceOffsetTicks= (((int)packet[i+7])<<8)|(packet[i+8]&0xFF);
                                             int dataLen= packet[i+9];
                                             unsigned char *data= &packet[i+10];
                                             
                                             INFO( "RBIStream: sync evt ctx %X id %X idinst %X tickformat 0x%X offset %d datalen %d",
                                                   eventContext, eventId, eventIdInstance, tickFormat, referenceOffsetTicks, dataLen );
                                             if ( dataLen > 0 )
                                             {
                                                dumpData( data, dataLen );
                                                DEBUG("\nas string: (%.*s)\n\n", dataLen, data );
                                             }
                                             
                                             if ( m_events )
                                             {
                                                RBITrigger *trigger= new RBITrigger( eventContext, eventId, eventIdInstance, -1LL, 0 );
                                                if ( trigger )
                                                {
                                                   if ( dataLen )
                                                   {
                                                      trigger->setData( data, dataLen );
                                                   }
                                                   
                                                   m_scanForFrameType= true;
                                                   
                                                   m_events->triggerDetected( this, trigger );
                                                   
                                                   delete trigger;
                                                }
                                             }
                                          }
                                       }
                                       break;
                                    // DVB Reserved
                                    case 0x00:
                                    default:
                                       if ( descId < 0x80 )
                                       {
                                          DEBUG( "RBIStream: ignoring DVB-SAD reserved descriptor: %X len %d offset %X", descId, descLen, i );
                                       }
                                       else
                                       {
                                          DEBUG( "RBIStream: ignoring DVB-SAD User private descriptor: %X len %d offset %X", descId, descLen, i );
                                       }
                                       break;
                                 }
                                 i += descLen;
                              }
                           }
                        }
                     }
                  }
                  #endif
                  #ifdef USE_SCTE35
                  // tableid
                  if (packet[payloadOffset+1] == 0xfc) 
                  {
                     // splice_info_section {
                     //   table_id:8
                     //   section_syntax_indicator:1
                     //   private_indicator:1
                     //   reserved:2
                     //   section_length:12
                     //   protocol_version:8
                     //   encrypted_packet:1
                     //   encrytion_algorithm:6
                     //   pts_adjustment:33
                     //   cw_index:8
                     //   tier:12
                     //   splice_command_length:12
                     //   splice_command_type: 8
                     int sectionLength = ((packet[payloadOffset+2]&0x0f)<<8)+packet[payloadOffset+3];
                     tableInfo_t * ptr = &m_tableAggregator[TABLE_AGGREGATOR_INDEX_SCTE35];
                     /*Update local copies of table */
                     memset( ptr->m_packets, 0, MAX_SCTE35_PACKET_BUFFER_SIZE);
                     memcpy( ptr->m_packets, packet-m_ttsSize, m_packetSize );
                     ptr->m_packetWriteOffset = m_packetSize;

                     if ( sectionLength <= (m_packetSize - m_ttsSize - TS_HEADER_SIZE - POINTER_FIELD_SIZE - PSI_TABLE_HEADER_SIZE))
                     {
                        processSCTE35Section(&packet[payloadOffset], sectionLength);
                     }
                     else
                     {
                        INFO( "RBIStream: processing multi-packet SCTE-35 (section length %d)", sectionLength );
                        if(!ptr->m_section)
                        {
                           ptr->m_section = new unsigned char [MAX_SCTE35_SECTION_SIZE];
                           INFO( "RBIStream: allocating buffer for SCTE-35 section." );
                        }
                        /*The contents of the section buffer is ptr_field + section start offset + section. In other words,
                         * what remains of the packet after TTS, TS header and adaptation field are removed. */
                        int dataLength = m_packetSize - m_ttsSize - payloadOffset;
                        rmf_osal_memcpy(ptr->m_section, &packet[payloadOffset], dataLength, MAX_SCTE35_SECTION_SIZE, dataLength);
                        ptr->m_sectionLength = sectionLength;
                        ptr->m_sectionWriteOffset = dataLength;
                        ptr->m_nextContinuity =  ((packet[3]+1)&0xF);
                     }
                  }
                  #endif
               }
            }
            else
            {
                TRACE1( "RBIStream: processing SCTE-35 TS without payload start indicator" );
                tableInfo_t *ptr  = &m_tableAggregator[TABLE_AGGREGATOR_INDEX_SCTE35];
                if(0 != ptr->m_sectionWriteOffset)
                {
                   unsigned int continuity = (packet[3]&0xF);
                   if ( ((continuity+1)&0xF) == ptr->m_nextContinuity )
                   {
                      WARNING( "Warning: RBIStream: next packet of multi-packet SCTE-35 has wrong continuity count %d (expecting %d)",
                      ptr->m_nextContinuity, continuity );
                      // Assume continuity counts for all packets of this SCTE-35 will remain the same.
                      // Allow this since it has been observed in the field
                      ptr->m_nextContinuity = continuity;
                   }
                   if( continuity == ptr->m_nextContinuity )
                   {
                      if(MAX_SCTE35_PACKET_BUFFER_SIZE >= (ptr->m_packetWriteOffset + m_packetSize))
                      {
                         memcpy( &(ptr->m_packets[ptr->m_packetWriteOffset]), packet-m_ttsSize, m_packetSize );
                         ptr->m_packetWriteOffset += m_packetSize;
                      }
                      else
                      {
                         WARNING("Dropping SCTE-35 packet to avoid buffer overrun. MAX_SCTE35_PACKET_BUFFER_SIZE [%d] ptr->m_packetWriteOffset [%d] m_packetSize [%d]", 
                            MAX_SCTE35_PACKET_BUFFER_SIZE, ptr->m_packetWriteOffset, m_packetSize );
                      }
                      int sectionRemaining = ptr->m_sectionLength + 4 /*headers and pointer field*/- ptr->m_sectionWriteOffset;
                      int payloadSize = m_packetSize - m_ttsSize - payloadOffset;

                      /* Copy the complete payload of this packet or the remainder of the section, whichever is less*/
                      int copyLength = ( sectionRemaining < payloadSize ? sectionRemaining : payloadSize );
                      TRACE1("Section remaining: %d, copyLength: %d", sectionRemaining, copyLength);

                      int spaceRemaining = MAX_SCTE35_SECTION_SIZE - ptr->m_sectionWriteOffset;
                      /* The buffer we have is limited in size. Limit copyLength so that it doesn't overshoot its boundaries. */
                      if(copyLength > spaceRemaining)
                      {
                         WARNING("Truncating section data to stay within allocated buffer limits.");
                         copyLength = spaceRemaining;
                      }
                      rmf_osal_memcpy(&(ptr->m_section[ptr->m_sectionWriteOffset]), &packet[payloadOffset], copyLength,
                      spaceRemaining, payloadSize);
                      ptr->m_sectionWriteOffset += copyLength;

                      /*Evaluate whether a full section is now available. Process it.*/
                      TRACE1("RBIStream: section write offset = %d", ptr->m_sectionWriteOffset);
                      if( ptr->m_sectionWriteOffset == (ptr->m_sectionLength + 4) )//TODO: Fix this condition check. It would fail if pointer_field is non-zero 
                      {
                         DEBUG("RBIStream: complete SCTE-35 section is now available. Parsing begins.");
                         processSCTE35Section(ptr->m_section, ptr->m_sectionLength);
                         ptr->m_sectionWriteOffset = 0; //To stop processing any more of these packets until there is a version change.
                      }
                      else
                      {
                         ptr->m_nextContinuity = ((continuity+1)&0xF);
                      }
                   }
                   else
                   {
                      WARNING("RBIStream: not processing multi-packet SCTE-35 because of discontinuity.");
                      ptr->m_sectionWriteOffset = 0;
                   }
                }
                else
                {
                   TRACE1("RBIStream: ignoring SCTE-35 packet until we see payload start.");
                }
            }
         }
         else if ( pid == m_videoPid )
         {
            payloadOffset= 4;
            adaptation= ((packet[3] & 0x30)>>4);
            payloadStart= (packet[1] & 0x40);

            if ( m_frameDetectEnable )
            {
               if ( adaptation & 0x02 )
               {
                  if ( pid == m_pcrPid )
                  {
                     int adaptationLen= packet[4];
                     int adaptationFlags= packet[5];
                     if ( (adaptationLen > 0) && adaptationFlags & 0x10 )
                     {
                        long long offset= m_frameStartOffset= m_streamOffset+(packetCount*m_packetSize);
                        m_currentPCR= readPCR( &packet[6] );
                        
                        m_lastPCRTTSValue = ((packet[-4] << 24) | (packet[-3] << 16) | (packet[-2] << 8) | packet[-1]);

                        if ( m_sample1PCR == -1LL )
                        {
                           m_sample1PCR= m_currentPCR;
                           m_sample1PCROffset= offset;
                        }
                        else if ( m_sample2PCR == -1LL )
                        {                           
                           if (m_currentPCR-m_sample1PCR > 3000)
                           {
                              double timeDiff;
                              double bitDiff;

                              m_sample2PCR= m_currentPCR;
                              m_sample2PCROffset= offset;
                              DEBUG("PCR sample1: pcr %llx offset %llx sample2 %llx offset %llx", 
                                    m_sample1PCR, m_sample1PCROffset, m_sample2PCR, m_sample2PCROffset );
                                

                              if ( m_currentPCR-m_sample1PCR < 1000*90LL )
                              {
                                 timeDiff= (m_sample2PCR-m_sample1PCR)/90000.0;
                                 bitDiff= (m_sample2PCROffset - m_sample1PCROffset)*8.0;

                                 m_bitRate= (unsigned int)(bitDiff / timeDiff);
                                 m_avgBitRate= ((m_avgBitRate == 0) ? m_bitRate : ((m_bitRate+m_avgBitRate)/2));
                                 
                                 DEBUG("RBIStream: bitrate %d avg bitrate %d\n", m_bitRate, m_avgBitRate ); 
                                 if ( m_events )
                                 {
                                    m_events->foundBitRate( this, m_bitRate, (m_sample2PCROffset-m_sample1PCROffset) );
                                 }
                                 m_sample1PCR= m_sample2PCR;
                                 m_sample1PCROffset= m_sample2PCROffset;
                                 m_sample2PCR= -1LL;
                              }
                              else
                              {
                                 // PCR discontinuity: reset search
                                 m_sample1PCR= -1LL;
                                 m_sample1PCROffset= 0LL;
                                 m_sample2PCR= -1LL;
                                 m_sample2PCROffset= 0LL;
                              }
                           }
                        }
                     }
                     if ( (adaptationLen > 0) && adaptationFlags & 0x40 )
                     {
                        if ( m_mapStartPending || m_mapEndPending || m_mapping )
                        {
                           if ( m_currentPTS != -1LL )
                           {
                                 DEBUG("randomAccess point: pts %llx", m_currentPTS );                                 
                           }
                           else
                           {
                                 DEBUG("randomAccess point");
                           }
                           m_randomAccess= true;
                        }
                     }
                  }
                  payloadOffset += (1+packet[4]);
               }

               if ( adaptation & 0x01 )
               {
                  if ( payloadStart )
                  {
                     if ( (packet[payloadOffset] == 0x00) && (packet[payloadOffset+1] == 0x00) && (packet[payloadOffset+2] == 0x01) )
                     {
                        int streamid= packet[payloadOffset+3];
                        int pesHeaderDataLen= packet[payloadOffset+8];
                        if ( packet[payloadOffset+7] & 0x80 )
                        {     
                           m_currentPTS= readTimeStamp( &packet[payloadOffset+9] );
                        }
                        if ( (streamid >= 0xE0) && (streamid <= 0xEF) )
                        {
                           m_scanForFrameType= true;
                           m_startOfSequence= false;
                           m_frameType= 0;
                           m_frameStartOffset= m_streamOffset+(packetCount*m_packetSize);

                           {
                              if ( m_mapEndPending && !m_randomAccess && !m_backToBack )
                              {
                                 // If we are at the first packet of a video frame but the
                                 // random access bit wasn't set, do a quick scan ahead
                                 // to see if there is a start of sequence in the packet.  This
                                 // is not 100% reliable since the start of sequence could
                                 // be in a later packet.  However, this will increase chances of
                                 // achieving good quality transitions.
                                 for( int i= payloadOffset; i < m_packetSize-m_ttsSize-4; ++i )
                                 {
                                    if( (packet[i] == 0x00) && (packet[i+1] == 0x00) && (packet[i+2] == 0x01) )
                                    {
                                       if ( m_isH264 && ((packet[i+3] & 0x1F) == 7) )
                                       {
                                          // Sequence Parameter Set
                                          m_startOfSequence= true;
                                          break;
                                       }
                                       else if ( !m_isH264 && (packet[i+3] == 0xB3) )
                                       {
                                          // Start of Sequence header
                                          m_startOfSequence= true;
                                          break;
                                       }
                                    }
                                 }
                              }

                              if ( (m_randomAccess || m_startOfSequence /* || (m_mapEndPending && m_backToBack) */) &&  m_insertSrc &&
                                    PTS_IN_RANGE( m_currentPTS, (m_mapSpliceInPTS - m_splicePTS_offset), m_mapSpliceInAbortPTS ) && !m_filterAudio
                                 )
                              {
                                 INFO("map ending: at random access point (%d,%d,%d)", m_randomAccess, m_startOfSequence, m_backToBack);
                                if( !m_backToBack )
                                {
                                    m_filterAudio= true;
                                    m_filterAudioStopPTS= m_mapSpliceInPTS;
                                    m_filterAudioStop2PTS= ( (m_filterAudioStopPTS + 1000LL*90LL)& MAX_90K);
                                    m_transitioningToNetwork = true; 
                                    m_determineNetAC3FrameSizeDuration = true;
                                    m_mapping = false;   
                                    m_mapEditGOP= true;
                                    // Updating the outpoint for the next ad spot
                                    m_mapSpliceUpdatedOutPTS = m_currentPTS; 
                                    
                                    int availableNullPacketsSize = ((packet - m_ttsSize) + m_packetSize - packets);
                                    unsigned char *nullPacket = packets + m_ttsSize;
                                    DEBUG("Replacing null packets with Ad audio packet during in point detection available NullPackets Size %d ", availableNullPacketsSize );

                                    int replacePacketCount = 0;
                                    while ( availableNullPacketsSize/m_packetSize > 0 )
                                    {
                                        if( (nullPacket[3] == 0xAA || replacePacketCount > 0))
                                        {
                                            bool netAudioPacket = false;
                                            if( nullPacket[3] == 0xAA && replacePacketCount == 0 )
                                            {
                                                netAudioPacket = true;
                                            }
                                            int byteCount = getInsertionAudioDataDuringTransitionBackToNetwork(nullPacket);
                                            int mappedPid = (((nullPacket[1] << 8) | nullPacket[2]) & 0x1FFF); 

                                            if (byteCount == -1) {
                                                m_transitioningToNetwork = false;                                                
                                                break;
                                            }
                                            else
                                            {
                                                lastAdAudioPacket = nullPacket;                                
                                            }
                                            if(replacePacketCount > 0)
                                            {
                                                --replacePacketCount;
                                            }
                                            else if ( netAudioPacket )
                                            {
                                                replacePacketCount = (adAC3FrameInfo.size/184);
                                                netAudioPacket = false;
                                            }
                                        }
                                        availableNullPacketsSize -= m_packetSize;
                                        if(availableNullPacketsSize/m_packetSize > 0)
                                        {
                                            nullPacket += m_packetSize;
                                        }
                                    }                                    
                                    TRACE3("map ending: start filtering network audio");                                    
                                }
                                TRACE3("map ending: continuing to pass Ad audio");
                                m_mapEnding= true;
                                m_mapEndingNeedVideoEOF= m_backToBack;                                
                              }
                              else
                              {
                                 if(m_insertSrc)
                                 {
                                    if(((m_randomAccessPrev != m_randomAccess) ||\
                                        (m_startOfSequencePrev != m_startOfSequence) ||\
                                        (m_mapEndPendingPrev != m_mapEndPending)) \
                                       )
                                    {
                                       if(PTS_IN_RANGE( m_currentPTS, m_mapSpliceInPTS, m_mapSpliceInAbortPTS ))
                                       {
                                          INFO("m_randomAccess[%d] m_startOfSequence[%d] m_mapEndPending[%d] m_backToBack[%d] m_currentPTS[%llx] m_mapSpliceInPTS[%llx] m_mapSpliceInAbortPTS[%llx] m_mapStartPending[%d] m_mapStartArmed[%d] m_mapEndPending[%d]",
                                             m_randomAccess, m_startOfSequence, m_mapEndPending, m_backToBack, m_currentPTS, m_mapSpliceInPTS,
                                             m_mapSpliceInAbortPTS, m_mapStartPending, m_mapStartArmed, m_mapEndPending );
                                       }
                                    }
                                 }
                              }

                              if ( m_mapStartPending )
                              {
                                 TRACE4("m_currentPTS %llx m_mapSpliceOutPTS %llx m_mapSpliceOutAbortPTS %llx m_mapSpliceInPTS %llx",
                                        m_currentPTS, m_mapSpliceOutPTS, m_mapSpliceOutAbortPTS, m_mapSpliceInPTS );
                                 if( !m_backToBackPrev )
                                 {
                                    if ( m_mapSpliceOutPTS == -1LL )
                                    {
                                       m_mapStartPTS= m_currentPTS+4500;
                                       m_mapStartPending= false;
                                       if( !m_backToBackPrev )
                                       {
                                           m_mapStartArmed= true;
                                       }                                    
                                       INFO("map start armed: arm PTS %llx", m_mapStartPTS );
                                    }
                                    else if ( PTS_IN_RANGE( m_currentPTS, (m_mapSpliceOutPTS - m_splicePTS_offset), m_mapSpliceInPTS ) )
                                    {
                                       if(m_currentPTS >= m_mapSpliceOutAbortPTS && m_insertSrc)
                                        {
                                            ERROR("missed start: CurrentPTS Exceeded m_insertSrc %x SpliceOutAbortPTS : m_currentPTS %llx m_mapSpliceOutPTS %llx m_mapSpliceOutAbortPTS %llx m_mapSpliceInPTS %llx m_bitRate %d m_mapStartPending %d m_mapEndPending %d m_mapping %d", 
                                                                                        m_insertSrc, m_currentPTS, m_mapSpliceOutPTS, m_mapSpliceOutAbortPTS, m_mapSpliceInPTS, m_bitRate, m_mapStartPending, m_mapEndPending, m_mapping );
                                            if ( m_events )
                                            {
                                               m_events->insertionError( this, RBI_DetailCode_unableToSplice_missed_spotWindow );
                                            }
                                            stopInsertion();
                                        }
                                       else
                                        {
                                            m_mapStartPTS= m_currentPTS;
                                            m_mapStartPCR= m_currentPCR;                              
                                            m_mapStartPending= false;
                                            m_insertStartTime= getCurrentTimeMicro();
                                            m_mapStartArmed= true;

                                            if ( m_ttsSize && (m_ttsInsert == 0xFFFFFFFF) )
                                            {
                                               m_ttsInsert= ((packet[-4]<<24)|(packet[-3]<<16)|(packet[-2]<<8)|packet[-1]);
                                               DEBUG("map start armed: ttsInsert %08X", m_ttsInsert);
                                            }

                                            INFO("map start armed: a: trigger PTS %llx arm PTS %llx PCR %llx", m_mapSpliceOutPTS, m_mapStartPTS, m_mapStartPCR );                                    
                                            if( m_insertSrc && !m_backToBackPrev )
                                            {
                                                m_transitioningToAd = true;
                                            } 
                                        }  
                                    }
                                }
                                else
                                {
                                    m_mapStartPTS= m_mapSpliceOutPTS;                                    
                                    m_mapStartPending= false;
                                    m_insertStartTime= getCurrentTimeMicro();
                                    m_mapping= true;
                                    m_trackContinuityCountersEnable= false;                                    
                                    m_continuityCounters[m_videoPid]= ((m_continuityCounters[m_videoPid]-1)&0xF);
                                    if ( m_events )
                                    {
                                       m_events->insertionStarted( this );
                                    }
                                    else
                                    {
                                       ERROR("m_events is NULL");
                                    }
                                    m_haveSegmentBasePCRAfterTransitionToAd = true;
                                    INFO("map start armed: back 2 back use case out PTS %llx ", m_mapSpliceOutPTS);
                                }
                                 if ( m_mapStartArmed )
                                 {
                                    INFO("m_currentPTS %llx m_mapSpliceOutPTS %llx m_mapSpliceOutAbortPTS %llx m_mapSpliceInPTS %llx m_bitRate %d m_mapStartPending %d m_mapEndPending %d m_mapping %d",
                                         m_currentPTS, m_mapSpliceOutPTS, m_mapSpliceOutAbortPTS, m_mapSpliceInPTS, m_bitRate, m_mapStartPending, m_mapEndPending, m_mapping );
                                 }
                              }
                              m_randomAccessPrev = m_randomAccess;
                              m_startOfSequencePrev = m_startOfSequence;
                              m_mapEndPendingPrev = m_mapEndPending;
                              
                              m_randomAccess= false;
                           }

                           if ( m_mapStartPending )
                           {
                              long long afterPTS;

                              afterPTS= (m_mapSpliceInPTS + 60000*90LL)&MAX_90K;

                              if ( m_insertSrc )
                              {
                                 if ( PTS_IN_RANGE( m_currentPTS, m_mapSpliceOutAbortPTS, afterPTS ) )
                                 {
                                    // This is a "replace" spot, but we haven't found a random access point
                                    // before our slice-in abort PTS.  End the spot with an error.
                                    ERROR("missed start: m_currentPTS %llx m_mapSpliceOutPTS %llx m_mapSpliceOutAbortPTS %llx m_mapSpliceInPTS %llx", m_currentPTS, m_mapSpliceOutPTS, m_mapSpliceOutAbortPTS, m_mapSpliceInPTS );
                                    m_leadTime= ((m_currentPTS - m_mapSpliceOutAbortPTS)&MAX_90K)/90LL;
                                    if ( m_events )
                                    {
                                       m_events->insertionError( this, RBI_DetailCode_unableToSplice_missed_spotWindow );
                                    }
                                    stopInsertion();
                                 }
                              }
                              else
                              {
                                 if ( PTS_IN_RANGE( m_currentPTS, m_mapSpliceOutAbortPTS, m_mapSpliceInPTS ) )
                                 {
                                    // This is a "fixed" spot.  If we haven't found a random access point
                                    // be we've hit the splice-in abort PTS then just proceed with arming
                                    m_mapStartPTS= m_currentPTS;
                                    m_mapStartPending= false;
                                    m_mapStartArmed= true;
                                    INFO("map start armed: b: trigger PTS %llx arm PTS %llx", m_mapSpliceOutPTS, m_mapStartPTS );
                                 }
                                 else
                                 {
                                    if ( PTS_IN_RANGE( m_currentPTS, m_mapSpliceInPTS, afterPTS ) )
                                    {
                                       // This is a "fixed" spot and we have completely missed the spot window.
                                       // End the spot with an error.
                                       ERROR("missed spot window: m_currentPTS %llx m_mapSpliceOutPTS %llx m_mapSpliceInPTS %llx",
                                             m_currentPTS, m_mapSpliceOutPTS, m_mapSpliceInPTS );
                                       m_leadTime= ((m_currentPTS - m_mapSpliceInPTS)&MAX_90K)/90LL;
                                       if ( m_events )
                                       {
                                          m_events->insertionError( this, RBI_DetailCode_unableToSplice_missed_spotWindow );
                                       }
                                       stopInsertion();
                                    }
                                 }
                              }
                           }
                           
                           if ( m_mapEndPending )
                           {
                              if ( m_insertSrc )
                              {
                                 if(((m_randomAccessPrev != m_randomAccess) ||\
                                        (m_startOfSequencePrev != m_startOfSequence) ||\
                                        (m_mapEndPendingPrev != m_mapEndPending)) \
                                       )
                                 {
                                    INFO("m_randomAccess[%d] m_startOfSequence[%d] m_mapEndPending[%d] m_backToBack[%d] m_currentPTS[%llx] m_mapSpliceInAbortPTS[%llx] m_mapSpliceInAbort2PTS[%llx] PTS_IN_RANGE[%d]",
                                       m_randomAccess, m_startOfSequence, m_mapEndPending, m_backToBack, m_currentPTS, m_mapSpliceInAbortPTS,
                                       m_mapSpliceInAbort2PTS, PTS_IN_RANGE( m_currentPTS, m_mapSpliceInAbortPTS, m_mapSpliceInAbort2PTS ) );
                                 }

                                 if ( (PTS_IN_RANGE( m_currentPTS, m_mapSpliceInAbortPTS, m_mapSpliceInAbort2PTS ) || (m_currentPTS >= m_mapSpliceInAbort2PTS)) 
                                        && !m_filterAudio )
                                 {
                                    // This is a "replace" spot, but we haven't found a random access point
                                    // in our splice-out window.  End the spot now.
                                    INFO("map ending: not at random access point : expect macroblocking");
                                    stopInsertion();
                                    m_filterAudio = false;
                                    m_mapEditGOP = true;
                                 }
                              }
                           }

                           if ( m_mapStartArmed )
                           {
                              if (
                                   m_mapStartArmed && (m_bitRate != 0)
                                   #ifndef XONE_STB 
                                   &&
                                   (
                                     m_backToBackPrev ||
                                     (
                                       (m_audioComponentCount > 0) &&
                                       PTS_IN_RANGE( m_currentAudioPTS, (m_mapSpliceOutPTS - m_spliceAudioPTS_offset), m_mapSpliceOutAbortPTS )
                                     )
                                   )
                                   #endif
                                 )
                              {
                                 if ( !m_mapping )
                                 {
                                    INFO("m_backToBackPrev %d" , m_backToBackPrev);
                                    m_mapStartArmed= false;
                                    
                                    m_mapping= true;
                                    m_trackContinuityCountersEnable= false;                                    
                                    m_continuityCounters[m_videoPid]= ((m_continuityCounters[m_videoPid]-1)&0xF);
                                    m_ttsInsert= 0xFFFFFFFF;
                                    if ( m_ttsSize && (m_ttsInsert == 0xFFFFFFFF) )
                                    {
                                       m_ttsInsert= ((packet[-4]<<24)|(packet[-3]<<16)|(packet[-2]<<8)|packet[-1]);
                                       INFO("map start armed: ttsInsert %08X", m_ttsInsert);
                                    }
                                    if( m_insertSrc )
                                    {                                        
                                        trimSize = size - ((packet - m_ttsSize) + m_packetSize - packets);  
                                        trimmedCurrentPCR = m_currentPCR;
                                        trimmedPCRTTSValue = m_lastPCRTTSValue;
                                        DEBUG("network audio reached splice-out PTS m_currentAudioPTS %llx  m_mapSpliceOutPTS %llx trimSize %d trimmedCurrentPCR %llx trimmedPCRTTSValue %08X", m_currentAudioPTS, m_mapSpliceOutPTS, trimSize, trimmedCurrentPCR, trimmedPCRTTSValue );        
                                    }
                                    
                                    if ( m_events )
                                    {
                                       m_events->insertionStarted( this );
                                    }
                                    else
                                    {
                                       ERROR("m_events is NULL");
                                    }
                                 }
                                 else if ( (m_audioComponentCount > 0) &&
                                           PTS_IN_RANGE( m_currentAudioPTS, (m_mapSpliceOutPTS - m_spliceAudioPTS_offset), m_mapSpliceOutAbortPTS ) )
                                 {
                                    m_mapStartArmed= false;                                    
                                    INFO("map start: network audio reached splice-out PTS");
                                 }
                                 else
                                 {
                                    if( (m_currentAudioPTS >= m_mapSpliceOutAbortPTS) && m_mapStartArmed && (m_bitRate != 0) && !m_backToBackPrev && (m_audioComponentCount > 0) )
                                    {
                                       ERROR("missed start: m_currentAudioPTS exceeded m_mapSpliceOutAbortPTS : m_currentPTS %llx m_mapSpliceOutPTS %llx m_mapSpliceOutAbortPTS %llx m_mapSpliceInPTS %llx m_mapStartPending %d m_mapEndPending %d m_mapping %d m_currentAudioPTS %llx",
                                          m_currentPTS, m_mapSpliceOutPTS, m_mapSpliceOutAbortPTS, m_mapSpliceInPTS, m_mapStartPending, m_mapEndPending, m_mapping, m_currentAudioPTS );
                                       if ( m_events )
                                       {
                                          m_events->insertionError( this, RBI_DetailCode_unableToSplice_missed_spotWindow );
                                       }
                                       stopInsertion();
                                    }
                                 }
                              }
                           }
                        }
                        payloadOffset= payloadOffset+9+pesHeaderDataLen;
                     }
                  }

                  if ( m_scanForFrameType )
                  {
                     int i, imax;

                     if ( m_scanHaveRemainder )
                     {
                        unsigned char *remainder= m_scanRemainder;
                        
                        memcpy( remainder+RBI_SCAN_REMAINDER_SIZE, packet+payloadOffset, RBI_SCAN_REMAINDER_SIZE );
                        
                        for( i= 0; i < RBI_SCAN_REMAINDER_SIZE; ++i )
                        {
                           if ( (remainder[i] == 0x00) && (remainder[i+1] == 0x00) && (remainder[i+2] == 0x01) )
                           {
                              processStartCode( &remainder[i], m_scanForFrameType, 2*RBI_SCAN_REMAINDER_SIZE-i, 0 );
                           }
                        }
                        
                        m_scanHaveRemainder= false;
                     }
                     
                     if ( m_scanForFrameType )
                     {
                        // If we are at the last packet in the buffer we need to stop scanning at the point
                        // where we can no longer access all needed start code bytes.
                        //imax= ((packet+m_packetSize < inputEnd) ? m_packetSize : m_packetSize-RBI_SCAN_REMAINDER_SIZE)-m_ttsSize;
                        imax= m_packetSize-RBI_SCAN_REMAINDER_SIZE-m_ttsSize;

                        for( i= payloadOffset; i < imax; ++i )
                        {
                           if ( (packet[i] == 0x00) && (packet[i+1] == 0x00) && (packet[i+2] == 0x01) )
                           {
                              processStartCode( &packet[i], m_scanForFrameType, imax-i, i );
                              
                              if ( !m_scanForFrameType )
                              {
                                 break;
                              }
                           }
                        }
                        if ( imax != m_packetSize-m_ttsSize )
                        {
                           m_scanHaveRemainder= true;
                           memcpy( m_scanRemainder, packet+m_packetSize-RBI_SCAN_REMAINDER_SIZE-m_ttsSize, RBI_SCAN_REMAINDER_SIZE );
                        }
                     }

                     if ( m_frameType != 0 )
                     {
                        if ( m_events )
                        {
                           m_events->foundFrame( this, m_frameType, m_frameStartOffset, m_currentPTS, m_currentPCR );
                        }                         
                     }    
                  }         
               }
            }
            else
            {
               if ( adaptation & 0x02 )
               {
                  if ( pid == m_pcrPid )
                  {
                     int adaptationLen= packet[4];
                     int adaptationFlags= packet[5];
                     if ( (adaptationLen > 0) && adaptationFlags & 0x10 )
                     {
                        m_currentPCR= readPCR( &packet[6] );
                     }
                  }
               }
            }
         }
         else if ( m_filterAudio || m_mapStartArmed || m_transitioningToAd)
         {
            if ( pid == m_audioPid )
            {                
                if(m_determineNetAC3FrameSizeDuration){
                    payloadOffset = 4;
                    adaptation = ((packet[3] & 0x30) >> 4);

                    if (adaptation & 0x02) {
                        payloadOffset += (1 + packet[4]);
                    }
                    payloadStart = (packet[1] & 0x40);
                    if ((adaptation & 0x01) && payloadStart && ((packet[payloadOffset] == 0x00) && (packet[payloadOffset + 1] == 0x00) && (packet[payloadOffset + 2] == 0x01))) {
                        payloadOffset += (9 + packet[payloadOffset + 8]);
                        getAC3FrameSizeDuration( packet, payloadOffset, &netAC3FrameInfo );
                        if( netAC3FrameInfo.duration != -1 && netAC3FrameInfo.size != -1)
                        {
                            m_determineNetAC3FrameSizeDuration = false;
                        }
                    }
                }
                if( !m_determineNetAC3FrameSizeDuration )
                {
                    if (!m_transitioningToAd && m_mapStartArmed && m_insertSrc)
                    {                        
                        packet[1]= ((packet[1] & 0xE0) | 0x1F );
                        packet[2]= 0xFF;
                    }
                    else
                    {
                        long long audioPTS = getAC3FramePTS( packet, &netAC3FrameInfo, m_currentAudioPTS);
                        if (audioPTS != -1)
                        {
                            m_currentAudioPTS = audioPTS;
                            if(m_insertSrc)
                            {
                                if ( (m_audioComponentCount > 0) &&
                                        PTS_IN_RANGE( m_currentAudioPTS, (m_mapSpliceOutPTS - m_spliceAudioPTS_offset), m_mapSpliceOutAbortPTS ) )
                                {
                                    if( m_transitioningToAd )
                                    {
                                        int netCorrectedPESPacketLength = netAC3FrameInfo.frameCnt*netAC3FrameInfo.size + netAC3FrameInfo.pesHeaderDataLen + 3;
                                        if( netAC3FrameInfo.pesPacketLengthMSB != 0 && netAC3FrameInfo.pesPacketLengthLSB != 0 )
                                        {
                                            *(netAC3FrameInfo.pesPacketLengthMSB) = (unsigned char)((netCorrectedPESPacketLength >> 8) & 0xFF);
                                            *(netAC3FrameInfo.pesPacketLengthLSB) = (unsigned char) (netCorrectedPESPacketLength & 0xFF);
                                        }
                                        if(netAC3FrameInfo.byteCnt >= 1)
                                        {
                                            unsigned char packetCopy[MAX_PACKET_SIZE];
                                            memcpy( packetCopy, packet, m_packetSize-m_ttsSize );

                                            packet[3] |= 0x30;                                                      
                                            packet[4] = netAC3FrameInfo.byteCnt - 1;
                                            if (netAC3FrameInfo.byteCnt > 1)
                                            {
                                                packet[5] = 0x00;
                                                for( int i = 0 ; i < netAC3FrameInfo.byteCnt -2; i++ )
                                                {                                                    
                                                    packet[6+i] = 0xFF;
                                                }                                                        
                                            } 
                                            for( int i = 4 + netAC3FrameInfo.byteCnt, j = 4; i < 188; i++, j++ )
                                            {
                                                packet[i] = packetCopy[j];
                                            }                                            
                                        }
                                        else
                                        {
                                            for( int i = 188 - netAC3FrameInfo.byteCnt; i < 188; i++ )
                                            {
                                                packet[i] = 0xFF;
                                            } 
                                        }

                                        m_transitioningToAd = false;
                                    }
                                }
                                else if( PTS_IN_RANGE(m_currentAudioPTS, m_filterAudioStopPTS, m_filterAudioStop2PTS) )
                                {
                                    long long filterAudioStop3PTS= ( (m_filterAudioStop2PTS + 1000LL*90LL)& MAX_90K);
                                    bool stopAudioFilter = false;
                                    if ( adAC3FrameInfo.syncByteDetectedInPacket )
                                    {
                                        if( lastAdAudioPacket != 0 && *lastAdAudioPacket == 0x47 && adAC3FrameInfo.byteCnt >= 1)
                                        {
                                            unsigned char packetCopy[MAX_PACKET_SIZE];
                                            memcpy( packetCopy, lastAdAudioPacket, m_packetSize-m_ttsSize );

                                            *(lastAdAudioPacket+3) |= 0x30;                                    
                                            for( int i = 4 + adAC3FrameInfo.byteCnt, j = 4; i < 188; i++, j++ )
                                            {
                                                *(lastAdAudioPacket+i) = packetCopy[j];
                                            }
                                            *(lastAdAudioPacket+4) = adAC3FrameInfo.byteCnt - 1;
                                            if (adAC3FrameInfo.byteCnt > 1)
                                            {
                                                *(lastAdAudioPacket+5) = 0x00;
                                                for( int i = 0 ; i < adAC3FrameInfo.byteCnt -2; i++ )
                                                {
                                                    *(lastAdAudioPacket+6+i) = 0xFF;
                                                }                                                        
                                            }
                                        }
                                        int adCorrectedPESPacketLength = adAC3FrameInfo.frameCnt*adAC3FrameInfo.size + adAC3FrameInfo.pesHeaderDataLen + 3;
                                        if( adAC3FrameInfo.pesPacketLengthMSB != 0 && adAC3FrameInfo.pesPacketLengthLSB != 0 )
                                        {
                                            *(adAC3FrameInfo.pesPacketLengthMSB) = (unsigned char)((adCorrectedPESPacketLength >> 8) & 0xFF);
                                            *(adAC3FrameInfo.pesPacketLengthLSB) = (unsigned char) (adCorrectedPESPacketLength & 0xFF);
                                        }
                                        stopAudioFilter = true;
                                    }
                                    else if( PTS_IN_RANGE(m_currentAudioPTS, m_filterAudioStop2PTS, filterAudioStop3PTS) )
                                    {
                                        DEBUG("In audio in point detected using filterAudioStop3PTS");
                                        stopAudioFilter = true;
                                    }

                                    if(stopAudioFilter)
                                    {
                                        payloadStart = (packet[1] & 0x40);
                                        if( !payloadStart && (184 - netAC3FrameInfo.byteCnt) >= 2)
                                        {
                                            packet[1] |= 0x40;
                                            packet[3] |= 0x30;
                                            packet[5] = 0x80;
                                            int numOfStuffingBytes = 0;

                                            if((184 - netAC3FrameInfo.byteCnt) >= 11 )
                                            {
                                                packet[4] = 188 - netAC3FrameInfo.byteCnt - 14;
                                                numOfStuffingBytes = 184 - netAC3FrameInfo.byteCnt -11;
                                                packet[6+numOfStuffingBytes] = 0x0;
                                                packet[7+numOfStuffingBytes] = 0x0;
                                                packet[8+numOfStuffingBytes] = 0x1;
                                                packet[9+numOfStuffingBytes] = 0xBD;
                                                int pesPacketLength = netAC3FrameInfo.pesPacketLength - (netAC3FrameInfo.frameCnt*netAC3FrameInfo.size) - netAC3FrameInfo.pesHeaderDataLen;
                                                packet[10+numOfStuffingBytes] = (pesPacketLength >> 8) & 0xFF;
                                                packet[11+numOfStuffingBytes] = pesPacketLength & 0xFF;
                                                packet[12+numOfStuffingBytes] = 0x84;
                                                packet[13+numOfStuffingBytes] = 0x0;
                                                packet[14+numOfStuffingBytes] = 0x0;                                   
                                            }
                                            else
                                            {
                                                packet[4] = 188 - netAC3FrameInfo.byteCnt - 5;
                                                numOfStuffingBytes = 184 - netAC3FrameInfo.byteCnt -2;
                                            }

                                            for( int i = 0 ; i < numOfStuffingBytes; i++ )
                                            {
                                                packet[i+6] = 0xFF;
                                            }                                                        
                                        }
                                        m_filterAudio= false;
                                        m_transitioningToNetwork = false;
                                        DEBUG("Audio inpoint detected, setting transitioning to network flag to false. stop filtering network audio");
                                        if ( m_mapEnding )
                                        {
                                           TRACE3("Stop passing Ad audio");
                                           m_mapEnding= false;
                                           stopInsertion(); 
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if ( m_filterAudio )
            {
                    if (pid == 0x1FFF ) {                          
                        if ( m_transitioningToNetwork )
                        {                            
                            int byteCount = getInsertionAudioDataDuringTransitionBackToNetwork(packet);
                            if (byteCount == -1) {
                                m_transitioningToNetwork = false;
                                DEBUG("Audio inpoint detected, setting transitioning to network flag to false null packet, no more audio data bytes");
                            }
                            else
                            {
                                lastAdAudioPacket = packet;                                
                            }
                        }
                    } 
                else {   
                    for( int i= 0; i < m_audioComponentCount; ++i )
                    {
                      if ( pid == m_audioComponents[i].pid )
                      {   
                        if ( m_transitioningToNetwork )
                        { 
                           int byteCount = getInsertionAudioDataDuringTransitionBackToNetwork(packet);
                            if (byteCount == -1 || byteCount == -2) {                                
                                packet[1]= ((packet[1] & 0xE0) | 0x1F);
                                packet[2]= 0xFF; 
                                if ( byteCount == -1 ) 
                                { 
                                    m_transitioningToNetwork = false;
                                    DEBUG("Audio inpoint detected, setting transitioning to network flag to false, no more audio data bytes");
                                }
                            }
                            else
                            {
                                lastAdAudioPacket = packet;
                            }                            
                        }
                        else
                        { 
                            packet[1]= ((packet[1] & 0xE0) | 0x1F);
                            packet[2]= 0xFF; 
                        }
                        break;
                      }
                   }
                }              
            }
        }

    if ( m_privateDataEnable && m_privateDataPids[pid] )
         {
            if ( m_events )
            {
               m_events->privateData( this, (unsigned char)m_privateDataTypes[pid], packet, 188 );
            }
         }
    }
      
      if ( m_mapStartArmed && m_insertSrc )
      {
         // During period between hitting the video splice-in PTS and
         // hitting the audio splice-in PTS convert video to null packets
         if ( pid == m_videoPid )
         {
            if (m_transitioningToAd) {
                int byteCount = getInsertionDataDuringTransitionToAd(packet);
                if (byteCount == -1) {
                    packet[1] = ((packet[1] & 0xE0) | 0x1F);
                    packet[2] = 0xFF;
                }                     
            }
            else
            {
              packet[1] = ((packet[1] & 0xE0) | 0x1F);
              packet[2] = 0xFF;
            }
         }
      }

      if ( m_mapping )
      {
         if ( m_insertSrc )
         {
            if ( !(m_mapStartArmed && !m_backToBackPrev) && !(m_mapEnding && !m_backToBack) )
            {
               int packetPid = (((packet[1] << 8) | packet[2]) & 0x1FFF);
               if( packetPid == m_audioPid )
               {
                packet[3] = 0xAA;
               }
               // Executing a "replace" spot.  The insertion data is coming via
               // calls to getInsertionDataSize() and getInsertionData().
               // Convert all packets to null packets.
               packet[1]= ((packet[1] & 0xE0) | 0x1F);
               packet[2]= 0xFF;
            }
         }
         else
         {
            bool atEnd;

            // Executing a "fixed" spot

            long long start= (m_mapSpliceOutPTS-500*90LL)&MAX_90K;
            if(alternateVideoSplicePointPTS != -1 && alternateVideoSplicePointExitFramePTS != -1)
            {
                if(m_currentPTS == alternateVideoSplicePointExitFramePTS)
                {
                    DEBUG("Stopping fixed spot tracking at exit frame PTS %llX Outpoint Frame PTS %llX ", alternateVideoSplicePointExitFramePTS, alternateVideoSplicePointPTS);
                    m_mapSpliceUpdatedOutPTS = alternateVideoSplicePointPTS;
                    stopInsertion();                    
                }
            }
            else 
            {
                atEnd= !PTS_IN_RANGE( m_currentPTS, start, m_mapSpliceInPTS );
                if ( atEnd )
                {
                   stopInsertion();
                }
            }
         }
      }

      packet += m_packetSize;
      ++packetCount;
   }
   
   processedSize= ((packet-m_ttsSize)-packets);
   processedSize -= trimSize;

   m_streamOffset += processedSize;

   if ( m_mapping && (wasMapping || m_backToBackPrev) && !m_mapStartArmed && !(m_mapEnding && !m_backToBack) && !wasArmed && (m_insertSrc != 0) )
   {
      processedSize= 0;
   }

   if ( m_ttsSize && m_mapping && !wasMapping && (m_insertSrc != 0) && !m_backToBackPrev)
   {
        unsigned char *lastPacket = packets + processedSize - m_packetSize + m_ttsSize ;
        m_ttsInsert = ((lastPacket[- 4] << 24) | (lastPacket[- 3] << 16) | (lastPacket[- 2] << 8) | lastPacket[- 1]);
        m_segmentBasePCR = (trimmedCurrentPCR + ((m_ttsInsert - trimmedPCRTTSValue) / 300)) & MAX_90K;

        m_firstAdPacketAfterTransitionTTSValue  = m_ttsInsert;
        TRACE3("last packet before mapping: ttsInsert %08X", m_ttsInsert);
        DEBUG("Last network packet before transition to all Ad packets new m_segmentBasePCR %llx first Ad packet TTS value %08X", m_segmentBasePCR, m_firstAdPacketAfterTransitionTTSValue);

   }
   else if ( m_ttsSize && m_mapping && processedSize )
   {
      m_ttsInsert= ((packet[-m_packetSize-4]<<24)|(packet[-m_packetSize-3]<<16)|(packet[-m_packetSize-2]<<8)|packet[-m_packetSize-1]);
      TRACE3("updating ttsInsert %08X processed size %d ", m_ttsInsert, processedSize);
   }

   if ( m_captureEnabled )
   {
      if ( m_mapStartPending || m_mapping || m_mapEndPending || wasMapping )
      {
         if ( !m_outFile )
         {
            m_outFile= fopen("/opt/rbicapture-out.ts","wb");
            m_captureEndCount= 40;
         }
         if ( m_outFile && (processedSize > 0) )
         {
            fwrite( packets, 1, processedSize, m_outFile );
         }
      }
      else if ( m_outFile )
      {
         if ( m_outFile && (processedSize > 0) )
         {
            fwrite( packets, 1, processedSize, m_outFile );
         }
         --m_captureEndCount;
         if ( m_captureEndCount <= 0 )
         {
            fclose( m_outFile );
            m_outFile= 0;
         }
      }
   }
    
   return processedSize;
}

long long RBIStreamProcessor::getAC3FramePTS( unsigned char* packet, AC3FrameInfo* ac3FrameInfo, long long currentAudioPTS)
{
    long long audioPTS = -1;
    int payloadOffset = 4;
    int adaptation = ((packet[3] & 0x30) >> 4);
    int payloadStart = (packet[1] & 0x40);
    ac3FrameInfo->syncByteDetectedInPacket = false;

    if (adaptation & 0x02) {
        payloadOffset += (1 + packet[4]);
    }
 

    if (adaptation & 0x01) {
        if (payloadStart) {
            if ((packet[payloadOffset] == 0x00) && (packet[payloadOffset + 1] == 0x00) && (packet[payloadOffset + 2] == 0x01)) {
                int pesHeaderDataLen = packet[payloadOffset + 8];
                ac3FrameInfo->pesPacketLength = ((packet[payloadOffset + 4] << 8) | packet[payloadOffset+5]);
                ac3FrameInfo->pesHeaderDataLen = pesHeaderDataLen;
                ac3FrameInfo->frameCnt=0;
                ac3FrameInfo->pesPacketLengthMSB = &packet[payloadOffset + 4];
                ac3FrameInfo->pesPacketLengthLSB = &packet[payloadOffset + 5];
                if (packet[payloadOffset + 7] & 0x80) {
                    audioPTS = readTimeStamp(&packet[payloadOffset + 9]);                                
                }
                payloadOffset += (9 + pesHeaderDataLen);

                // Scan for AC-3 frame sync word 0x0B77
                for( int i= payloadOffset; i < m_packetSize-5 ; ++i )
                {
                    if ( (packet[i] == 0x0B) && (packet[i+1] == 0x77))
                    {
                        ac3FrameInfo->byteCnt = 188 - i;
                        ac3FrameInfo->syncByteDetectedInPacket = true;
                    }
                }
            }
        }
        else
        {
            ac3FrameInfo->byteCnt += (188 - payloadOffset);                        
            if(ac3FrameInfo->byteCnt >= ac3FrameInfo->size)
            {                               
                audioPTS = currentAudioPTS + ac3FrameInfo->duration;                
                ac3FrameInfo->byteCnt -= ac3FrameInfo->size;
                ac3FrameInfo->frameCnt++;
                ac3FrameInfo->syncByteDetectedInPacket = true;
            }
        }
    }
    return audioPTS;
}


void RBIStreamProcessor::getAC3FrameSizeDuration(unsigned char* packet, int offset, AC3FrameInfo* frameInfo)
{    
    frameInfo->duration = -1;
    frameInfo->size = -1;
    frameInfo->byteCnt = 0;

    for( int i= offset; i < m_packetSize-5; ++i )
    {
        // Detect AC-3 frame sync word
        if ( (packet[i] == 0x0B) && (packet[i+1] == 0x77))
        {
            int fscod = (packet[i+4]>>6)&0x3;
            int frmsizecod = (packet[i+4])&0x3F;
            int bitrateIndex = frmsizecod>>1;
            int nominalBitrateInkbps[] = {32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 448, 512, 576, 640}; 

            switch ( fscod )
            {
                case 0x00: // 48kHz sampling rate
                    frameInfo->duration = 0.032*90000;
                    frameInfo->size = (32 * nominalBitrateInkbps[bitrateIndex])/8;
                    break;
                case 0x01: // 44.1kHz sampling rate
                    frameInfo->duration = 0.03483*90000;
                    frameInfo->size = (34.83 * nominalBitrateInkbps[bitrateIndex])/8;
                    frameInfo->size = (frmsizecod & 0x1)? (frameInfo->size+1):frameInfo->size;
                    break;
                case 0x02: // 32kHz sampling rate
                    frameInfo->duration = 0.048*90000;
                    frameInfo->size = (48 * nominalBitrateInkbps[bitrateIndex])/8;
                    break;
                default:
                    break;
            }             
        }
    }
 }

void RBIStreamProcessor::processSCTE35Section( unsigned char* section, int sectionLength )
{
   bool triggerEmitted= false;
   bool ignoreCommand= false;
   int encryptedPacket= section[5] & 0x80;

   if ( !encryptedPacket )
   {
      long long ptsAdjustment= (((long long)(section[5]&1))<<32);
      ptsAdjustment |= (((long long)section[6])<<24);
      ptsAdjustment |= (((long long)section[7])<<16);
      ptsAdjustment |= (((long long)section[8])<<8);
      ptsAdjustment |= (((long long)section[9]));

      int spliceCmdLength= ((section[12]&0x0F)<<8);
      spliceCmdLength |= section[13];
      int spliceCmd= section[14];
      int inferredSpliceCmdLen= 0;
      INFO("SCTE-35: cmd %d len %d ptsAdj %llx", spliceCmd, spliceCmdLength, ptsAdjustment );
      switch ( spliceCmd )
      {
         case 0x00: // splice_null
            DEBUG("SCTE-35: ignoring splice_null cmd");
            ignoreCommand= true;
            break;
         case 0x04: // splice_schedule
            DEBUG("SCTE-35: ignoring splice_schedule cmd");
            ignoreCommand= true;
            break;
         case 0x05: // splice_insert
#ifdef USE_SPLICE_INSERT_COMMAND
         {
            // splice_event_id:32
            // splice_event_cancel_indicator:1
            // reserved:7
            // if ( splice_event_cancel_indicator == 0 ) {
            //	 out_of_network_indicator:1
            //	 program_splice_flag:1
            //	 duration_flag:1
            //	 splice_immediate_flag:1
            //	 reserved:4
            //	 if ( program_splice_flag==1 && splice_immediate_flag==0) {
            //	   splice_time():
            //		 time_specified_flag:1
            //		 if ( time_specified_flag == 1 ) {
            //		   reserved:6
            //		   pts_time:33
            //		 } else {
            //		   reserved:7
            //		 }
            //	 }
            //	 if ( program_splice_flag == 0 ) {
            //	   component_count:8
            //	   for(i=0; i<component_count; ++i) {
            //		 component_tag:8
            //		 if ( splice_immediate_flag == 0 ) {
            //		   splice_time()
            //		 }
            //	   }
            //	 }
            //	 if ( duration_flag == 1 ) {
            //	   break_duration():
            //		 auto_return:1
            //		 reserved:6
            //		 duration:33
            //	 }
            //	 unique_program_id:16
            //	 avail_num:8
            //	 avails_expected:8
            // }
            int cmdOffset;
            int spliceEventId;
            int cancelIndicator;
            int timeSpecified;
            long long spliceTime= 0;

            cmdOffset= 15;
            spliceEventId= (section[cmdOffset]<<24);
            spliceEventId |= (section[cmdOffset+1]<<16);
            spliceEventId |= (section[cmdOffset+2]<<8);
            spliceEventId |= (section[cmdOffset+3]);
            cancelIndicator= ((section[cmdOffset+4]>>7)&0x1);
            INFO("SCTE-35: spliceEventId: %X cancelIndication %d", spliceEventId, cancelIndicator );

            if ( spliceCmdLength == 0xFFF )
            {
               inferredSpliceCmdLen += 5;
            }

            if ( !cancelIndicator )
            {
               int outOfNetworkIndicator;
               int programSpliceFlag;
               int durationFlag;
               int spliceImmediateFlag;

               outOfNetworkIndicator= ((section[cmdOffset+5]>>7)&0x1);
	           programSpliceFlag= ((section[cmdOffset+5]>>6)&0x1);
               durationFlag= ((section[cmdOffset+5]>>5)&0x1);
               spliceImmediateFlag= ((section[cmdOffset+5]>>4)&0x1);
               INFO("SCTE-35: outOfNetwork:%d programSpliceFlag %d durationFlag %d spliceImmediateFlag %d",
                  outOfNetworkIndicator, programSpliceFlag, durationFlag, spliceImmediateFlag );

               if ( spliceCmdLength == 0xFFF )
               {
                  inferredSpliceCmdLen += 1;
               }

               if ( outOfNetworkIndicator )
               {
                  if ( programSpliceFlag )
                  {
                     if ( !spliceImmediateFlag )
                     {
                        if ( spliceCmdLength == 0xFFF )
                        {
                           inferredSpliceCmdLen += 1;
                        }
                        timeSpecified= ((section[cmdOffset+6]>>7)&0x1);
                        INFO("SCTE-35: timeSpecified: %d", timeSpecified);
                        if ( timeSpecified )
                        {
                           if ( spliceCmdLength == 0xFFF )
                           {
                              inferredSpliceCmdLen += 4;
                           }
                           spliceTime= ((long long)(section[cmdOffset+6]&1)<<32);
                           spliceTime |= ((long long)(section[cmdOffset+7])<<24);
                           spliceTime |= ((long long)(section[cmdOffset+8])<<16);
                           spliceTime |= ((long long)(section[cmdOffset+9])<<8);
                           spliceTime |= ((long long)(section[cmdOffset+10]));

                           INFO("SCTE-35: splice_insert spliceEventId %X spliceTime: %llx", spliceEventId, spliceTime );

                           spliceTime= ((spliceTime + ptsAdjustment)&MAX_90K);

                           INFO("SCTE-35: splice_insert spliceEventId %X adjusted spliceTime: %llx", spliceEventId, spliceTime );
                        }
                     }
                     if ( spliceCmdLength == 0xFFF )
                     {
                        if ( durationFlag )
                        {
                           // time specified
                           inferredSpliceCmdLen += 1;
                           if ( section[cmdOffset+11] & 0x80 )
                           {
                              // pts time
                              inferredSpliceCmdLen += 4;
                           }
                        }
                        // unique_program_id, avail_num, avails_expected
                        inferredSpliceCmdLen += 4;
                     }
                  }
                  else
                  {
                     WARNING("SCTE-35: ignoring splice_insert using component mode");
                     ignoreCommand= true;
                  }
               }
               else
               {                  
                  DEBUG("Detected via Splice Insert SpliceTime %llX mapSpliceOutPTSIn %llX ", spliceTime, m_mapSpliceInPTS);
                  WARNING("SCTE-35: ignoring splice_insert with out_of_network_indicator of 0");
                  ignoreCommand= true;
               }
            }

            if ( spliceCmdLength == 0xFFF )
            {
               spliceCmdLength= inferredSpliceCmdLen;
               INFO("SCTE-35: infer spliceCmdLen is %d", spliceCmdLength );
            }

            if ( spliceTime )
            {
               int descriptorOffset= 15+spliceCmdLength;
               int descriptorLoopLength= (section[descriptorOffset]<<8);

               triggerEmitted= processSCTEDescriptors( section, spliceCmd, descriptorOffset, descriptorLoopLength, spliceTime );
               TRACE4("triggerEmitted %d", triggerEmitted);
            }
            if ( !triggerEmitted && !ignoreCommand )
            {
               INFO("SCTE-35: trigger: spliceEventId %X", spliceEventId );
               if ( m_events )
               {
                  RBITrigger *trigger= 0;

                  if ( spliceTime )
                  {
                     long long leadTime= 0;

                     if ( m_currentPCR != -1LL )
                     {
                        leadTime= (spliceTime-m_currentPCR)&MAX_90K;
                     }
                     trigger= new RBITrigger( 0, 0, spliceEventId, spliceTime, leadTime );
                  }
                  else
                  {
                     trigger= new RBITrigger( 0, 0, spliceEventId, -1LL, 0LL );
                  }
                  if ( trigger )
                  {
                     m_scanForFrameType= true;

                     m_events->triggerDetected( this, trigger );

                     delete trigger;

                     triggerEmitted= true;
                  }
               }
            }
         }
            break;
      #else
            INFO("SCTE-35: ignoring splice_insert cmd");
            ignoreCommand= true;
            break;
      #endif
         case 0x06: // time_signal
            {
               //	   splice_time():
               //		 time_specified_flag:1
               //		 if ( time_specified_flag == 1 ) {
               //		   reserved:6
               //		   pts_time:33
               //		 } else {
               //		   reserved:7
               //		 }
               int cmdOffset;
               int timeSpecified;
               long long spliceTime= 0;

               cmdOffset= 15;
               if ( spliceCmdLength == 0xFFF )
               {
                  inferredSpliceCmdLen += 1;
               }
               timeSpecified= ((section[cmdOffset]>>7)&0x1);
               INFO("SCTE-35: timeSpecified: %d", timeSpecified);
               if ( timeSpecified )
               {
                  if ( spliceCmdLength == 0xFFF )
                  {
                     inferredSpliceCmdLen += 4;
                  }
                  spliceTime= ((long long)(section[cmdOffset+0]&1)<<32);
                  spliceTime |= ((long long)(section[cmdOffset+1])<<24);
                  spliceTime |= ((long long)(section[cmdOffset+2])<<16);
                  spliceTime |= ((long long)(section[cmdOffset+3])<<8);
                  spliceTime |= ((long long)(section[cmdOffset+4]));

                  INFO("SCTE-35: time_signal spliceTime: %llx", spliceTime );

                  spliceTime= ((spliceTime + ptsAdjustment)&MAX_90K);

                  INFO("SCTE-35: time_signal adjusted spliceTime: %llx", spliceTime );
               }

               if ( spliceCmdLength == 0xFFF )
               {
                  spliceCmdLength= inferredSpliceCmdLen;
                  INFO("SCTE-35: infer spliceCmdLen is %d", spliceCmdLength );
               }

               if ( spliceTime )
               {
                  unsigned char *m_spliceInfoBuffer = (unsigned char *)calloc(sectionLength+3+1, sizeof(unsigned char));
                  if(m_spliceInfoBuffer)
                  {
                     memcpy( m_spliceInfoBuffer, &section[1], sectionLength+3);
                     m_spliceInfoBuffer[sectionLength+3] = '\0';
                     // LSA-134: For testing the splice_info_section - base64 conversion.
                     /*for(int i=0; i<(sectionLength+3);i++)
                     {
                        INFO("m_spliceInfoBuffer[%d] = %0x\n",i,m_spliceInfoBuffer[i]);
                     }*/
                     size_t  bytesReq = b64_encoded_size(sectionLength+3);
                     uint8_t *protectedDataPtr = (uint8_t*)calloc(bytesReq+1, sizeof(uint8_t));
                     if(protectedDataPtr)
                     {
                        base64_encode((const  uint8_t*) m_spliceInfoBuffer, sectionLength+3, protectedDataPtr);
                        protectedDataPtr[bytesReq] = '\0';

                        m_TimeSignalbase64Msg = (char*)protectedDataPtr;
                     }
                     else
                     {
                        ERROR("protectedDataPtr memory failed for %d bytes", bytesReq+1);
                     }
                     free(m_spliceInfoBuffer);
                     m_spliceInfoBuffer = NULL;
                  }
                  else
                  {
                     ERROR("m_spliceInfoBuffer memory failed for %d bytes", sectionLength+3+1);
                  }

                  int descriptorOffset= 15+spliceCmdLength;
                  int descriptorLoopLength= (section[descriptorOffset]<<8);

                  triggerEmitted= processSCTEDescriptors( section, spliceCmd, descriptorOffset, descriptorLoopLength, spliceTime );
               }
            }
            break;
         case 0x07: // bandwidth_reservation
            DEBUG("SCTE-35: ignoring bandwidth_reservation cmd");
            ignoreCommand= true;
            break;
         case 0xFF: // private_command
            DEBUG("SCTE-35: ignoring private_command cmd");
            ignoreCommand= true;
            break;
         default:
            DEBUG("SCTE-35: ignoring unknown cmd %X", spliceCmd );
            ignoreCommand= true;
            break;
      }
   }
   else
   {
      WARNING( "SCTE-35 encrypted splice_info_section detected and ignored" );
   }
   TRACE4("ignoreCommand %d", ignoreCommand);
}

void RBIStreamProcessor::processPMTSection( unsigned char* section, int sectionLength )
{
    unsigned char *programInfo, *programInfoEnd;
    int videoComponentCount, videoPids[MAX_PIDS], videoTypes[MAX_PIDS];
    int audioComponentCount, audioPids[MAX_PIDS], audioTypes[MAX_PIDS];
    int dataComponentCount, dataPids[MAX_PIDS], dataTypes[MAX_PIDS];
    int privateComponentCount, privatePids[MAX_PIDS], privateTypes[MAX_PIDS];
    char* audioLanguages[MAX_PIDS];
    int streamType, pid, len;
    char work[32];
    int pcrPid= (((section[9]&0x1F)<<8)+section[10]);
    int infoLength= (((section[11]&0x0F)<<8)+section[12]);

    videoComponentCount= audioComponentCount= dataComponentCount= privateComponentCount= 0;
    programInfo= &section[infoLength+13];
    programInfoEnd= section+4/*to account for byte count till section_length */+sectionLength-4/*Subtract CRC length*/;
    while ( programInfo < programInfoEnd )
    {
        streamType= programInfo[0];
        pid= (((programInfo[1]&0x1F)<<8)+programInfo[2]);
        len= (((programInfo[3]&0x0F)<<8)+programInfo[4]);
        switch( streamType )
        {
            case 0x02: // MPEG2 Video
            case 0x80: // ATSC Video
            case 0x1B: // H.264 Video
                videoPids[videoComponentCount]= pid;
                videoTypes[videoComponentCount]= streamType;
                ++videoComponentCount;
                if ( streamType == 0x1B )
                {
                    m_isH264= true;
                    if ( !m_H264Enabled )
                    {
                        INFO("SCTE-35 is currently disabled for H.264");
                        m_triggerEnable= false;
                    }
                }
                break;
            case 0x03: // MPEG1 Audio                                    
            case 0x04: // MPEG2 Audio                                    
            case 0x0F: // MPEG2 AAC Audio                                    
            case 0x11: // MPEG4 LATM AAC Audio                                    
            case 0x81: // ATSC AC3 Audio                                    
            case 0x82: // HDMV DTS Audio                                    
            case 0x83: // LPCM Audio                                    
            case 0x84: // SDDS Audio                                    
            case 0x87: // ATSC E-AC3 Audio                                    
            case 0x8A: // DTS Audio                                    
            case 0x91: // A52b/AC3 Audio                                    
            case 0x94: // SDDS Audio
                audioPids[audioComponentCount]= pid;
                audioTypes[audioComponentCount]= streamType;
                audioLanguages[audioComponentCount]= 0;
                if( len > 2 )
                {
                    int descIdx, maxIdx;
                    int descrTag, descrLen;

                    descIdx= 5;
                    maxIdx= descIdx+len;

                    while ( descIdx < maxIdx )
                    {
                        descrTag= programInfo[descIdx];
                        descrLen= programInfo[descIdx+1];

                        switch ( descrTag )
                        {
                            // ISO_639_language_descriptor
                            case 0x0A:
                                memcpy( work, &programInfo[descIdx+2], descrLen );
                                work[descrLen]= '\0';
                                audioLanguages[audioComponentCount]= strdup(work);
                                break;
                        }

                        descIdx += (2+descrLen);
                    }
                }
                ++audioComponentCount;
                break;
#ifdef USE_DVBSAD   
            case 0x06: // ISO 13818-1 PES private data
                // DVB-SAD triggers carried by streamType 6
                dataPids[dataComponentCount]= pid;
                dataTypes[dataComponentCount]= streamType;
                ++dataComponentCount;
                break;
#endif
#ifdef USE_SCTE35
            case 0x86: // SCTE-35 / DTS-HD Audio
                dataPids[dataComponentCount]= pid;
                dataTypes[dataComponentCount]= streamType;
                ++dataComponentCount;
                break;
#endif
            case 0x01: // MPEG1 Video
            case 0x07: // ISO 13522 MHEG
            case 0x08: // ISO 13818-1 DSM-CC
            case 0x09: // ISO 13818-1 auxiliary
            case 0x0a: // ISO 13818-6 multi-protocol encap	
            case 0x0b: // ISO 13818-6 DSM-CC U-N msgs
            case 0x0c: // ISO 13818-6 stream descriptors
            case 0x0d: // SO 13818-6 sections
            case 0x0e: // ISO 13818-1 auxiliary
            case 0x10: // MPEG4 Video
            case 0x12: // MPEG-4 generic
            case 0x13: // ISO 14496-1 SL-packetized
            case 0x14: // ISO 13818-6 Synchronized Download Protocol
            case 0x85: // ATSC Program ID
            case 0x92: // DVD_SPU vls Subtitle
            case 0xA0: // MSCODEC Video
            case 0xea: // Private ES (VC-1)
            default:
                // Ignore other stream types
                break;
        }
        if ( m_privateDataMonitor.numStreams > 0 )
        {
            bool match= false;
            for( int n= 0; n < m_privateDataMonitor.numStreams; ++n )
            {
                if ( streamType == m_privateDataMonitor.specs[n].streamType )
                {
                    if ( m_privateDataMonitor.specs[n].matchTag )
                    {
                        if( len > 2 )
                        {
                            int descIdx, maxIdx;
                            int descrTag, descrLen;

                            descIdx= 5;
                            maxIdx= descIdx+len;

                            while ( descIdx < maxIdx )
                            {
                                descrTag= programInfo[descIdx];
                                descrLen= programInfo[descIdx+1];

                                if ( descrTag == m_privateDataMonitor.specs[n].descrTag )
                                {
                                    if ( m_privateDataMonitor.specs[n].descrDataLen > 0 )
                                    {
                                        if ( descrLen == m_privateDataMonitor.specs[n].descrDataLen )
                                        {
                                            if ( memcmp( (char*)&programInfo[7], m_privateDataMonitor.specs[n].descrData, descrLen ) == 0 )
                                            {
                                                match= true;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        match= true;
                                    }
                                }
                                if ( match )
                                {
                                    break;
                                }
                                descIdx += (2+descrLen);
                            }
                        }
                    }
                    else
                    {
                        match= true;
                    }
                }
                if ( match )
                {
                    break;
                }
            }
            if ( match )
            {
                privatePids[privateComponentCount]= pid;
                privateTypes[privateComponentCount]= streamType;
                ++privateComponentCount;
            }
        }
        programInfo += (5 + len);
    }

    INFO( "RBIStream: found %d video (%s), %d audio, and %d data pids in program %x with pcr pid %x",
            videoComponentCount, (m_isH264 ? "H264":"MPEG2"), audioComponentCount, dataComponentCount, m_program, pcrPid );

    setComponentInfo( pcrPid,
            videoComponentCount, videoPids, videoTypes,
            audioComponentCount, audioPids, audioTypes, audioLanguages,
            dataComponentCount, dataPids, dataTypes,
            privateComponentCount, privatePids, privateTypes );

}
// Call with buffer pointing to beginning of start code (iex 0x00, 0x00, 0x01, ...) 
bool RBIStreamProcessor::processStartCode( unsigned char *buffer, bool& keepScanning, int length, int base )
{
   bool result= true;
   int frameTypeBits;

   if ( m_isH264 )
   {
      int unitType= (buffer[INDEX(3)] & 0x1F);
      switch( unitType )
      {                 
         case 1:  // Non-IDR slice
         case 5:  // IDR slice
            // Check if first_mb_in_slice is 0.  It will be 0 for the start of a frame, but could be non-zero
            // for frames with multiple slices.  This is encoded as a variable length Exp-Golomb code.  For the value
            // of zero this will be a single '1' bit.
            if ( buffer[INDEX(4)] & 0x80 )
            {
               int slice_type_bits= (buffer[INDEX(4)] & 0x7F);
               int slice_type= expGolombTable7[slice_type_bits].value;
               
               switch ( slice_type )
               {
                  case 0:  // P slice
                  case 3:  // SP slice
                  case 5:  // P slice
                  case 8:  // SP slice
                     m_frameType= P_FRAME;
                     break;
                  case 1:  // B slice
                  case 6:  // B slice
                     m_frameType= B_FRAME;
                     break;
                  case 2:  // I slice
                  case 4:  // SI slice
                  case 7:  // I slice
                  case 9:  // SI slice
                     m_frameType= I_FRAME;
                     break;
               }               
               if ( m_frameType != 0 )
               {
                  if ( unitType == 5 )
                  {
                     TRACE3("found IDR frame");
                  }
                  //m_scanForFrameType= false;
                  keepScanning= false;
               }
            }
            break;         
         case 2:  // Slice data partiton A         
         case 3:  // Slice data partiton B         
         case 4:  // Slice data partiton C         
            break;
         case 6:  // SEI
            break;         
         case 7:  // Sequence paramter set
            {
               bool scanForAspect= false;
               bool splitSPS= false;
               
               m_startOfSequence= true;
               if ( !m_emulationPrevention || (m_emulationPreventionCapacity < (m_emulationPreventionOffset+length)) )
               {
                  unsigned char *newBuff= 0;
                  int newSize= m_emulationPreventionCapacity*2+length;
                  newBuff= (unsigned char *)malloc( newSize*sizeof(char) );
                  if ( !newBuff )
                  {
                     ERROR( "Error: unable to allocate emulation prevention buffer" );
                     break;
                  }
                  if ( m_emulationPrevention )
                  {
                     if ( m_emulationPreventionOffset > 0 )
                     {
                        memcpy( newBuff, m_emulationPrevention, m_emulationPreventionOffset );
                     }
                     free( m_emulationPrevention );
                  }
                  m_emulationPreventionCapacity= newSize;
                  m_emulationPrevention= newBuff;
               }
               if ( m_emulationPreventionOffset > 0 )
               {
                  splitSPS= true;
                  TRACE4("splitSPS %d", splitSPS);
               }
               for( int i= 4; i < length-3; ++i )
               {
                  if ( (buffer[INDEX(i)] == 0x00) && (buffer[INDEX(i+1)] == 0x00) )
                  {
                     if ( buffer[INDEX(i+2)] == 0x01 )
                     {
                        scanForAspect= true;
                        break;
                     }
                     else if ( buffer[INDEX(i+2)] == 0x03 )
                     {
                        m_emulationPrevention[m_emulationPreventionOffset++]= 0x00;
                        m_emulationPrevention[m_emulationPreventionOffset++]= 0x00;
                        i += 3;
                     }
                  }
                  m_emulationPrevention[m_emulationPreventionOffset++]= buffer[INDEX(i)];
               }
               if ( scanForAspect )
               {
                  processSeqParameterSet( m_emulationPrevention, m_emulationPreventionOffset );  
                  m_emulationPreventionOffset= 0;
               }
            }
            break;         
         case 8:  // Picture paramter set
            break;         
         case 9:  // NAL access unit delimiter
            break;                     
         case 10: // End of sequence
            break;                     
         case 11: // End of stream
            break;         
         case 12: // Filler data
            break;         
         case 13: // Sequence parameter set extension
            break;         
         case 14: // Prefix NAL unit
            break;         
         case 15: // Subset sequence parameter set
            break;
         // Reserved
         case 16:
         case 17:
         case 18:
            break;
         // unspecified
         case 0:
         case 21:
         case 22:
         case 23:
         default:
            break;
            
            
      }
   }
   else
   {
       switch( buffer[INDEX(3)] )
       {
          // Picture
          case 0x00:
             frameTypeBits= ((buffer[INDEX(5)]>>3)&0x7);
             switch( frameTypeBits )
             {
                case 1:
                   m_frameType= I_FRAME;
                   m_GOPSize= m_frameInGOPCount;
                   m_frameInGOPCount= 0;
                   break;
                case 2:
                   m_frameType= P_FRAME;
                   break;
                case 3:
                   m_frameType= B_FRAME;
                   break;
                default:
                   m_frameType= 0;
                   break;
             }
             ++m_frameInGOPCount;
             //m_scanForFrameType= false;
             keepScanning= false;
             break;
          // Sequence Header
          case 0xB3:
             {                                      
                int frameWidth= (((int)buffer[INDEX(4)])<<4)|(((int)buffer[INDEX(5)])>>4);
                int frameHeight= ((((int)buffer[INDEX(5)])&0x0F)<<8)|((int)buffer[INDEX(6)]);
                m_startOfSequence= true;
                if ( (m_frameWidth == -1) || (frameWidth != m_frameWidth) ||
                     (m_frameHeight == -1) || (frameHeight != m_frameHeight) )
                {
                   m_frameWidth= frameWidth;
                   m_frameHeight= frameHeight;
                   INFO( "RBIStreamProcessor::processStartCode: frame size is %dx%d\n", m_frameWidth, m_frameHeight );
                }
             }
             break;
          // GOP
          case 0xB8:
             if ( m_mapEditGOP )
             {
                int closedGOP, brokenGOP;
                
                closedGOP= (buffer[INDEX(7)] & 0x40) ? 1 : 0;
                brokenGOP= (buffer[INDEX(7)] & 0x20) ? 1 : 0;
                if ( !( (brokenGOP == 1) && (closedGOP == 0) ) )
                {
                   DEBUG( "RBIStreamProcessor::processStartCode: marking GOP as broken and open\n" );
                   buffer[INDEX(7)]= ((buffer[INDEX(7)] & ~(0x40|0x20)) | 0x20);
                }
                
                m_mapEditGOP= false;
             }
             break;
          default:
             break;
       }
   }
   
   return result;
}

bool RBIStreamProcessor::processSCTEDescriptors( unsigned char *packet, int cmd, int descriptorOffset, int descriptorLoopLength, long long spliceTime )
{
   bool triggerEmitted= false;
   int descriptorTag;
   int descriptorLen;
   int descriptorEndOffset;                                    
   int cancelIndicator;

   descriptorLoopLength |= packet[descriptorOffset+1];
   descriptorEndOffset= descriptorOffset+descriptorLoopLength;
   DEBUG("SCTE-35: descriptor_loop_length: %d", descriptorLoopLength);
   descriptorOffset+=2;
   while( descriptorOffset < descriptorEndOffset )
   {
      descriptorTag= packet[descriptorOffset];
      descriptorLen= packet[descriptorOffset+1];
      switch( descriptorTag )
      {
         case 0x00: // avail_descriptor
            DEBUG("SCTE-35: ignoring avail descriptor");
            break;
         case 0x01: // DTMF_descriptor
            DEBUG("SCTE-35: ignoring DTMF descriptor");
            break;
         case 0x02: // segmentation_descriptor
            {
               DEBUG("SCTE-35: segment_descriptor len %d", descriptorLen );
               // splice_descriptor_tag:8
               // descriptor_length:8
               // identifier:32
               // segment_event_id:32
               // segment_event_cancel_indicator:1
               // reserved: 7
               // if ( segment_event_cancel_indicator == 0 ) {
               //   program_segmentation_flag:1
               //   segmentation_duration_flag:1
               //   delivery_not_restricted_flag:1
               //   if ( delivery_not_restricted_flag == 0 ) {
               //     web_delivery_allowed_flag:1
               //     no_regional_blackout_flag:1
               //     archive_allowed_flag:1
               //     device_restrictions:1
               //   } else {
               //     reserved:5
               //   }
               //   if ( program_segmentation_flag == 0 ) {
               //     component_count:8
               //     for(i=0;i<component_count;++i) {
               //       component_tag:8
               //       reserved:7
               //       pts_offset:33
               //     }
               //   }
               //   if ( segmentation_duration_flag == 1 ) {
               //     segmentation_duration:40
               //   }
               //   segmentation_upid_type:8
               //   segmentation_upid_length:8
               //   segmentation_upid
               //   segmentation_type_id:8
               //   segment_num:8
               //   segments_expected:8
               // }
               unsigned int identifier;
               unsigned int segmentEventId;
               identifier= (packet[descriptorOffset+2]<<24);
               identifier |= (packet[descriptorOffset+3]<<16);
               identifier |= (packet[descriptorOffset+4]<<8);
               identifier |= (packet[descriptorOffset+5]);
               segmentEventId= (packet[descriptorOffset+6]<<24);
               segmentEventId |= (packet[descriptorOffset+7]<<16);
               segmentEventId |= (packet[descriptorOffset+8]<<8);
               segmentEventId |= (packet[descriptorOffset+9]);
               cancelIndicator= ((packet[descriptorOffset+10]>>7)&0x1);
               if ( !cancelIndicator )
               {
                  int programSegmentationFlag = 0;
                  int segmentationDurationFlag = 0;
                  int deliveryNotRestrictedFlag = 0;
                  programSegmentationFlag= ((packet[descriptorOffset+11]>>7)&0x1);
                  segmentationDurationFlag= ((packet[descriptorOffset+11]>>6)&0x1);
                  deliveryNotRestrictedFlag= ((packet[descriptorOffset+11]>>5)&0x1);
                  if ( deliveryNotRestrictedFlag == 0 )
                  {
                     WARNING( "SCTE-35: ignoring delivery restrictions: 0x%02X", packet[descriptorOffset+11] );
                  }
                  if ( programSegmentationFlag == 1 )
                  {
                     int segmentationUpidType = 0;
                     int segmentationUpidLen = 0;
                     int segmentationTypeId = 0;
                     int segmentNum = 0;
                     int segmentsExpected = 0;
                     bool provider = false;
                     bool validRequest = false;
                     bool distributor = false;
                     bool firstSegmentationUpid = true;
                     SegmentUpidElement segmentUpidElement;
                     bool segmentationUsed = false;
                     PlacementOpportunityData poData;

                     memset(&segmentUpidElement, 0, sizeof(segmentUpidElement));
                     memset(&poData, 0, sizeof(poData));

                     descriptorOffset += 12;
                     if ( segmentationDurationFlag )
                     {
                         poData.poDuration = (((long long)(packet[descriptorOffset]&0xFF))<<(40-8)) |
                                                                        (((long long)(packet[descriptorOffset+1]&0xFF))<<(40-8-8)) |
                                                                        (((long long)(packet[descriptorOffset+2]&0xFF))<<(40-8-8-8)) |
                                                                        (((long long)(packet[descriptorOffset+3]&0xFF))<<(40-8-8-8-8)) |
                                                                        ((long long)(packet[descriptorOffset+4]&0xFF));
                         DEBUG("SCTE-35 Segmentation duration %llx ", poData.poDuration );
                         
                        descriptorOffset += 5;
                     }
                     segmentationUpidType= packet[descriptorOffset];
                     segmentationUpidLen= packet[descriptorOffset+1];

                     poData.poType = segmentationTypeId= packet[descriptorOffset + 2 + segmentationUpidLen];
                     segmentNum= packet[descriptorOffset + 2 + segmentationUpidLen + 1];
                     segmentsExpected= packet[descriptorOffset + 2 + segmentationUpidLen + 2];

                     provider = ((segmentationTypeId == 0x30) || (segmentationTypeId == 0x44) || (segmentationTypeId == 0x34));
                     distributor = (segmentationTypeId == 0x36);

                     if(firstSegmentationUpid)
                     {
                        firstSegmentationUpid = false;
                        poData.segmentUpidElement.eventId = segmentEventId;
                        poData.segmentUpidElement.typeId = segmentationTypeId;
                        poData.segmentUpidElement.segmentNum = segmentNum;
                        poData.segmentUpidElement.segmentsExpected = segmentsExpected;
                     }

                     char signal[] = "SIGNAL:";
                     char temp[RBI_MAX_DATA];
                     memset( temp, 0, sizeof(temp) );
                     INFO("SCTE-35: upidtype %d\n", segmentationUpidType);

                     switch( segmentationUpidType )
                     {
                        case 13:   // MID
                           {
                              int moff= 0;
                              int ulen;
                              while( moff < segmentationUpidLen )
                              {
                                 segmentationUsed = false;
                                 segmentationUpidType= packet[descriptorOffset+2+moff];
                                 ulen= packet[descriptorOffset+3+moff];
                                 if(ulen <= RBI_MAX_DATA)
                                 {
                                    memcpy( temp, &packet[descriptorOffset+2+2+moff], ulen);
                                    temp[ulen]= 0;
                                    INFO("SCTE-35: MID upidtype %d upidlen %d upid(%s)",
                                        segmentationUpidType, ulen, temp );
                                    if (segmentationUpidType == 9 )
                                    {
                                       if( strstr( (char*)temp, (const char*)signal ) != NULL )
                                       {
                                          strncpy( poData.signalId, (char*)&packet[descriptorOffset+2+2+moff], ulen);
                                          poData.signalId[ulen]= 0;

                                          if(distributor)
                                             validRequest = true;
                                       }
                                       else if(provider)
                                       {
                                          if(strncmp(temp, "PO:", strlen("PO:")) == 0)
                                          {
                                             strncpy( poData.elementId, (char*)&packet[descriptorOffset+2+2+moff], ulen);
                                             poData.elementId[ulen]= 0;
                                             segmentationUsed = true;
                                          }
                                          else if(strncmp(temp, "BREAK:", strlen("BREAK:")) == 0)
                                          {
                                             strncpy( poData.elementBreakId, (char*)&packet[descriptorOffset+2+2+moff], ulen);
                                             poData.elementBreakId[ulen]= 0;
                                             segmentationUsed = true;
                                          }
                                       }
                                    }
                                    else if ((segmentationUpidType == 0xF) && (distributor || provider ))
                                    {
                                       if( provider && ( strstr( (char*)temp, (const char*)PROVIDER_STR ) != NULL ) )
                                       {
                                          validRequest = true;
                                          segmentationUsed = true;
                                       }

                                       if( strstr( (char*)temp, (const char*)MERLIN_STR ) != NULL )
                                       {
                                          strncpy( poData.streamId, (char*)&packet[descriptorOffset+2+2+moff], ulen);
                                          poData.streamId[ulen]= 0;
                                          INFO("poData.streamId %s %d", poData.streamId, ulen );
                                          segmentationUsed = true;
                                       }
                                       else if( strstr( (char*)temp, (const char*)SERVICESOURCE_STR ) != NULL )
                                       {
                                          strncpy( poData.serviceSource, (char*)&packet[descriptorOffset+2+2+moff], ulen);
                                          poData.serviceSource[ulen]= 0;
                                          segmentationUsed = true;
                                       }
                                       else if( strstr( (char*)temp, (const char*)ADZONE_STR ) != NULL )
                                       {
                                          strncpy( poData.zone, (char*)&packet[descriptorOffset+2+2+moff], ulen);
                                          poData.zone[ulen]= 0;
                                          segmentationUsed = true;
                                       }
                                    }

                                    if(segmentationUsed == false)
                                    {
                                       std::string segmentationPOUpid = "";
                                       int segmentationPOUpidLen = 0;
                                       constexpr char hexmap[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
                                       for (int i = 0; i < ulen; ++i)
                                       {
                                          segmentationPOUpid.append(1,hexmap[(temp[i] & 0xF0) >> 4]);
                                          segmentationPOUpid.append(1,hexmap[temp[i] & 0x0F]);
                                       }
                                       segmentationPOUpidLen = (ulen*2);
                                       if(segmentationPOUpidLen > RBI_MAX_DATA)
                                       {
                                          ERROR("segmentationPOUpidLen lenth exceeded %d", segmentationPOUpidLen);
                                          segmentationPOUpidLen = RBI_MAX_DATA;
                                       }

                                       if ( segmentationUpidType == 0x8 )
                                       {
                                          memcpy(poData.contentId, reinterpret_cast<const unsigned char*>(segmentationPOUpid.c_str()), segmentationPOUpidLen);
                                          INFO("Airing / Content Id %s %s", (char*)temp, reinterpret_cast<const unsigned char*>(segmentationPOUpid.c_str()));
                                       }
                                       else
                                       {
                                          memcpy(poData.segmentUpidElement.segments[poData.segmentUpidElement.totalSegments].segmentUpid, reinterpret_cast<const unsigned char*>(segmentationPOUpid.c_str()), segmentationPOUpidLen);
                                          INFO("segmentUpidElement %s %s", (char*)temp, reinterpret_cast<const char*>(poData.segmentUpidElement.segments[poData.segmentUpidElement.totalSegments].segmentUpid));

                                          poData.segmentUpidElement.segments[poData.segmentUpidElement.totalSegments].upidType = segmentationUpidType;
                                          poData.segmentUpidElement.totalSegments++;
                                       }
                                    }
                                 }
                                 else
                                 {
                                    ERROR("segmentationUpid length is exceeded the limit ulen %d", ulen);
                                 }
                                 moff += (ulen+2);
                              }
                           }
                           break;
                        case 9:    // ADI
                        default:
                           {
                              if(segmentationUpidLen < RBI_MAX_DATA)
                              {
                                 memcpy( temp, &packet[descriptorOffset+2], segmentationUpidLen);
                                 temp[segmentationUpidLen]= 0;
                                 INFO("SCTE-35: ADI upidtype %d upidlen %d upid(%s)",
                                        segmentationUpidType, segmentationUpidLen, temp );
                                 if( strstr( (char*)temp, (const char*)signal ) != NULL )
                                 {
                                    strncpy( poData.signalId, (char*)&packet[descriptorOffset+2], segmentationUpidLen);
                                    poData.signalId[segmentationUpidLen]= 0;

                                    if(distributor)
                                       validRequest = true;
                                 }
                              }
                              else
                              {
                                 ERROR("segmentationUpid length is exceeded the limit segmentationUpidLen %d", segmentationUpidLen);
                              }
                           }
                           break;
                     }
                     descriptorOffset += 2+segmentationUpidLen + 3;

                     std::string sourceUri;
                     if( m_events )
                        sourceUri = m_events->getSourceUri();
                     else
                        sourceUri = "m_events is NULL";

                     INFO("SCTE-35: trigger id (%X) segmentEventId (%X) segtype (%X) upidtype (%d) upidlen (%d) upid(%s) cmd(%d) sourceUri(%s)",
                           identifier, segmentEventId, poData.poType, segmentationUpidType, segmentationUpidLen, poData.signalId, cmd, sourceUri.c_str());

                     INFO("SCTE-35 eventId (%u) typeId (%X) segmentNum %d segmentsExpected %d totalSegmets %d",
                        poData.segmentUpidElement.eventId,
                        poData.segmentUpidElement.typeId,
                        poData.segmentUpidElement.segmentNum,
                        poData.segmentUpidElement.segmentsExpected,
                        poData.segmentUpidElement.totalSegments );

                     for(unsigned int i=0; i < poData.segmentUpidElement.totalSegments; i++)
                     {
                        INFO("SCTE-35 upidType %d %s",
                           poData.segmentUpidElement.segments[i].upidType,
                           reinterpret_cast<const char*>(poData.segmentUpidElement.segments[i].segmentUpid) );
                     }

                     // Generate a trigger for slice_insert commands (0x05), or time_signal (0x06)
                     // commands if the segmentation type is DistributorOpportunityStart
                     if ( 
                          (cmd == 0x05) ||
                          ((cmd == 0x06) && validRequest )
                        )
                     {
                        if(RBIManager::getInstance()->isSTTAcquired() == true)
                        {
                           if ( m_events )
                           {
                              if(m_events->getTriggerDetectedForOpportunity() == false)
                              {
                                 m_events->setTriggerDetectedForOpportunity(true);
                                 long long leadTime= 0;

                                 if ( m_currentPCR != -1LL )
                                 {
                                    leadTime= (spliceTime-m_currentPCR)&MAX_90K;
                                    INFO("m_currentPCR %llx", m_currentPCR);
                                 }

                                 RBITrigger *trigger= new RBITrigger( 0, 0, segmentEventId, spliceTime, leadTime );
                                 if ( trigger )
                                 {
                                    if (validRequest)
                                    {
                                       trigger->setPOData(&poData);
                                       m_scanForFrameType= true;
                                       if(m_events->triggerDetected( this, trigger ) == false)
                                       {
                                           m_events->setTriggerDetectedForOpportunity(false);
                                           INFO("triggerDetected return false. setTriggerDetectedForOpportunity set to FALSE. segmentationTypeId (%X) sourceUri (%s)", segmentationTypeId, sourceUri.c_str());
                                       }
                                       delete trigger;
                                       triggerEmitted= true;
                                    }
                                    else
                                    {
                                        m_events->setTriggerDetectedForOpportunity(false);
                                        INFO("segmentationUpid is empty. setTriggerDetectedForOpportunity set to FALSE. segmentationTypeId (%X) sourceUri (%s)", segmentationTypeId, sourceUri.c_str());
                                    }
                                 }
                                 else
                                 {
                                     m_events->setTriggerDetectedForOpportunity(false);
                                     INFO("triggerDetected is NULL. setTriggerDetectedForOpportunity set to FALSE. segmentationTypeId (%X) sourceUri (%s)", segmentationTypeId, sourceUri.c_str());
                                 }
                              }
                              else
                              {
                                 ERROR("The previous placement in progress, skip the trigger. segmentationTypeId (%X) sourceUri (%s)", segmentationTypeId, sourceUri.c_str());
                              }
                           }
                        }
                        else
                        {
                           ERROR("STT not acquired, skip the trigger. segmentationTypeId (%X) sourceUri (%s)", segmentationTypeId, sourceUri.c_str());
                        }
                     }

                    if ( ( (segmentationTypeId == 0x35 && RBIManager::getInstance()->isProgrammerEnablementEnabled()) /* || segmentationTypeId == 0x37 */ ) && m_insertSrc )
                    {                        
                        if( abs(spliceTime - m_mapSpliceInPTS) < 12000)
                        {
                            m_mapSpliceInPTS = spliceTime;
                            m_netReplaceableVideoFramesCnt = round((double)( (spliceTime - m_mapStartPTS)*VIDEO_FRAME_RATE/90000) );
                            DEBUG("New number of network video frames to replace %d New In point PTS %llX segmentationTypeId %d ", m_netReplaceableVideoFramesCnt, m_mapSpliceInPTS, segmentationTypeId);
                        }                        
                    }
                  }
                  else
                  {
                     WARNING("SCTE-35: ignoring component mode segment");
                  }
               }
            }
            break;
         default:
            INFO("SCTE-35: ignoring unsupported descriptor tag: %X len %d", descriptorTag, descriptorLen );
            break;
      }
      descriptorOffset += (2+descriptorLen);
   }
   
   return triggerEmitted;
}

bool RBIStreamProcessor::setComponentInfo
  ( int pcrPid,
    int videoCount, int *videoPids, int *videoTypes,
    int audioCount, int *audioPids, int *audioTypes, char **audioLanguages,
    int dataCount, int *dataPids, int *dataTypes,
    int privateCount, int *privatePids, int *privateTypes )
{
   bool result= true;
   RBIStreamComponent *videoComponents= 0;
   RBIStreamComponent *audioComponents= 0;
   RBIStreamComponent *dataComponents= 0;
   int i;

   if ( videoCount > 0 )
   {
      videoComponents= (RBIStreamComponent*)calloc( 1, videoCount*sizeof(RBIStreamComponent) );
      if ( !videoComponents )
      {
         result= false;
         goto exit;
      }
      for( i= 0; i < videoCount; ++i )
      {
         videoComponents[i].streamType= videoTypes[i];
         videoComponents[i].pid= videoPids[i];
         TRACE1( "video %d: streamType %X pid %X", i, videoTypes[i], videoPids[i] );
      }
   }
   if ( audioCount > 0 )
   {
      audioComponents= (RBIStreamComponent*)calloc( 1, audioCount*sizeof(RBIStreamComponent) );
      if ( !audioComponents )
      {
         result= false;
         goto exit;
      }
      for( i= 0; i < audioCount; ++i )
      {
         audioComponents[i].streamType= audioTypes[i];
         audioComponents[i].pid= audioPids[i];
         audioComponents[i].associatedLanguage= audioLanguages[i];
         TRACE1( "audio %d: streamType %X pid %X lang(%s)", i, audioTypes[i], audioPids[i], (audioLanguages[i] ? audioLanguages[i] : "none") );
      }
   }
   if ( dataCount+privateCount > 0 )
   {
      dataComponents= (RBIStreamComponent*)calloc( 1, (dataCount+privateCount)*sizeof(RBIStreamComponent) );
      if ( !dataComponents )
      {
         result= false;
         goto exit;
      }
      for( i= 0; i < dataCount; ++i )
      {
         dataComponents[i].streamType= dataTypes[i];
         dataComponents[i].pid= dataPids[i];
      }
      for( i= 0; i < privateCount; ++i )
      {
         dataComponents[i+dataCount].streamType= privateTypes[i];
         dataComponents[i+dataCount].pid= privatePids[i];
      }
   }
   
   termComponentInfo();
   
   m_pcrPid= pcrPid;

   m_videoComponentCount= videoCount;
   m_videoComponents= videoComponents;
   if ( videoCount )
   {
      m_videoPid= videoPids[0];
   }

   m_audioComponentCount= audioCount;
   m_audioComponents= audioComponents;
   if ( audioCount )
   {
      // Use first audio pid for purposes of PTS sampling with the 
      // assumption that all audio is more or less synchronized in the multiplex.
      m_audioPid= audioPids[0];
   }

   m_dataComponentCount= dataCount;
   m_dataComponents= dataComponents;
   memset( m_triggerPids, 0, sizeof(m_triggerPids) );
   memset( m_privateDataPids, 0, sizeof(m_privateDataPids) );
   for( i= 0; i < dataCount; ++i )
   {
      #ifdef USE_DVBSAD
      if ( dataTypes[i] == 0x06 )
      {
         INFO("expecting DVB-SAD triggers on pid %X : enable(%d)", dataPids[i], m_triggerEnable );
         m_triggerPids[dataPids[i]]= 0x01;
      }
      #endif
      #ifdef USE_SCTE35
      if ( dataTypes[i] == 0x86 )
      {
         INFO("expecting SCTE-35 triggers on pid %X : enable(%d)", dataPids[i], m_triggerEnable );
         m_triggerPids[dataPids[i]]= 0x01;
      }
      #endif
   }
   for( i= 0; i < privateCount; ++i )
   {
      INFO("expecting private data on pid %X streamType %X: enable(%d)", privatePids[i], privateTypes[i], m_privateDataEnable );
      m_privateDataPids[privatePids[i]]= 0x01;
      m_privateDataTypes[privatePids[i]]= privateTypes[i];
   }
   
exit:

   if ( !result )
   {
      termComponentInfo();
   }
   
   return result;
}

void RBIStreamProcessor::termComponentInfo()
{
   memset( m_triggerPids, 0x00, sizeof(m_triggerPids) );
   
   if ( m_videoComponents )
   {
      for( int i= 0; i < m_videoComponentCount; ++i )
      {
         termComponent( &(m_videoComponents[i]) );
      }
      free( m_videoComponents );
      m_videoComponents= 0;
      m_videoComponentCount= 0;
   }
   if ( m_audioComponents )
   {
      for( int i= 0; i < m_audioComponentCount; ++i )
      {
         termComponent( &(m_audioComponents[i]) );
      }
      free( m_audioComponents );
      m_audioComponents= 0;
      m_audioComponentCount= 0;
   }
   if ( m_dataComponents )
   {
      for( int i= 0; i < m_dataComponentCount; ++i )
      {
         termComponent( &(m_dataComponents[i]) );
      }
      free( m_dataComponents );
      m_dataComponents= 0;
      m_dataComponentCount= 0;
   }
}

void RBIStreamProcessor::termComponent( RBIStreamComponent *component )
{
   if ( component )
   {
      if ( component->associatedLanguage )
      {
         free( component->associatedLanguage );
         component->associatedLanguage= 0;
      }
   }
}

void RBIStreamProcessor::reTimestamp( unsigned char *packet, int length )
{
   long long timeOffset, rateAdjustedPCR;
   double adNetPCRDelta = 0;
                  
   if ( !m_haveBaseTime )
   {
        m_haveBaseTime = true;
        if( m_b2bSavedAudioPacketsCnt > 0 )
        {
            m_prevAdBaseTime = m_baseTime;
            m_prevAdSegmentBaseTime = m_segmentBaseTime;
            DEBUG("In reTimestamp setting prev ad base time m_prevAdBaseTime %llx m_prevAdSegmentBaseTime %llx", m_prevAdBaseTime, m_prevAdSegmentBaseTime);
        }
        m_baseTime = m_insertSrc->startPTS();
        m_segmentBaseTime = m_currentMappedPTS + (long long)(90000/VIDEO_FRAME_RATE);   
        
        m_basePCR = m_insertSrc->startPCR();
        if ( (m_ttsInsert - m_lastMappedPCRTTSValue)/(300 * 90000) < 1 )
        {
            m_segmentBasePCR = m_currentMappedPCR + (m_ttsInsert - m_lastMappedPCRTTSValue)/300;
        }
        m_currentMappedPCR = m_segmentBasePCR;
        DEBUG("In reTimestamp m_baseTime %llx m_basePCR %llx m_segmentBaseTime %llx m_segmentBasePCR %llx m_currentPCR %llx ", m_baseTime, m_basePCR, m_segmentBaseTime, m_segmentBasePCR, m_currentPCR);        
   }
      
   TRACE1("RBIStreamProcessor::reTimestamp: packet %p length %d", packet, length );
   for( int i= 0; i < length; i += m_packetSize )
   {
      int payloadStart= (packet[1] & 0x40);
      int adaptation= ((packet[3] & 0x30)>>4);
      int payload= 4;
      int pid;

      pid= (((packet[1] << 8) | packet[2]) & 0x1FFF);

      
      {
         // Update PCR values
         if ( adaptation & 0x02 )
         {
            int adaptationLen= packet[4];
            int adaptationFlags= packet[5];
            if ( (adaptationLen > 0) && (adaptationFlags & 0x10) )
            {
               m_lastAdPCR= readPCR( &packet[6] );
               m_currentInsertPCR= m_lastAdPCR;
               
               if (!m_haveSegmentBasePCRAfterTransitionToAd) {
                    m_haveSegmentBasePCRAfterTransitionToAd = true;
                    rateAdjustedPCR = (m_segmentBasePCR + ((m_ttsInsert - m_firstAdPacketAfterTransitionTTSValue) / 300)) & MAX_90K;
                    m_segmentBasePCR = rateAdjustedPCR;
                    m_basePCR = m_lastAdPCR;
                    DEBUG("RBIStreamProcessor::reTimestamp: setting segment base PCR %llx m_ttsInsert %08x Ad base PCR: %llx ", m_segmentBasePCR, m_ttsInsert, m_basePCR );
                } else {
                        if( m_adVideoPktsDelayed || m_adAudioPktsDelayed || m_adVideoPktsTooFast || m_adVideoPktsDelayedPCRBased)
                        {
                            timeOffset = (m_ttsInsert - m_lastMappedPCRTTSValue)/300;
                            rateAdjustedPCR = (((long long) (timeOffset + 0.5) + m_currentMappedPCR)&0x1FFFFFFFFLL);
                            m_segmentBasePCR = rateAdjustedPCR;
                            m_basePCR = m_lastAdPCR;                            
                        }
                        else
                        {
                            timeOffset= m_lastAdPCR-m_basePCR;
                            rateAdjustedPCR= (((long long)(timeOffset+0.5)+m_segmentBasePCR)&0x1FFFFFFFFLL);
                        }
                }

               if ( rateAdjustedPCR < m_mapStartPCR )
               {
                  m_mapStartPCR= (m_mapStartPCR+10)&0x1FFFFFFFFLL;
                  DEBUG("RBIStreamProcessor::reTimestamp: pid %X rateAdjustedPCR clamped from %llx to %llx", pid, rateAdjustedPCR, m_mapStartPCR );
                  rateAdjustedPCR= m_mapStartPCR;
               }
               m_currentMappedPCR= rateAdjustedPCR;
               writePCR( &packet[6], rateAdjustedPCR ); 
               packet[0] = 0x48;
               
                if ( m_currentPCR != m_prevCurrentPCR )
                {
                    adNetPCRDelta = (m_currentMappedPCR - m_currentPCR)/90000.0;
                    m_prevCurrentPCR = m_currentPCR;
                    
                    if ( m_adVideoFrameCount < (m_netReplaceableVideoFramesCnt - 120) )
                    {
                        if( !m_adVideoPktsDelayedPCRBased && !m_adVideoPktsTooFast && adNetPCRDelta > 0.2 )
                        {
                            DEBUG("Ad Video Packets delayed speedup, PCR based adNetPCRDelta %f ", adNetPCRDelta );
                            m_adVideoPktsDelayedPCRBased = true;
                        }
                        else if( m_adVideoPktsDelayedPCRBased && adNetPCRDelta < 0 )
                        {
                            DEBUG("Ad Video Packets back to normal, PCR based adNetPCRDelta %f ", adNetPCRDelta );
                            m_adVideoPktsDelayedPCRBased = false;
                        }

                        if( !m_adVideoPktsTooFast && adNetPCRDelta < -0.2 )
                        {
                            DEBUG("Ad Video Packets too fast PCR based adNetPCRDelta %f ", adNetPCRDelta );
                            m_adVideoPktsDelayedPCRBased = false;
                            m_adVideoPktsTooFast = true;
                        }
                        else if( m_adVideoPktsTooFast && adNetPCRDelta > 0 )
                        {
                            DEBUG("Ad Video Packets back to normal, PCR based adNetPCRDelta %f ", adNetPCRDelta );
                            m_adVideoPktsTooFast = false;
                        } 
                    }
                }              
            }
            payload += (1+packet[4]);
         }      

    if (m_b2bSavedAudioPacketsCnt > 0 && pid == m_audioPid )
    {
        long long PTS= 0;
        
        if ( payloadStart )
         {
            if ( (packet[payload] == 0x00) && (packet[payload+1] == 0x00) && (packet[payload+2] == 0x01) )
            {
                int tsbase= payload+9;
                if ( packet[payload+7] & 0x80 )
                {     
                  PTS= readTimeStamp( &packet[tsbase] );
                }
            }
         }
    }
         // Update PTS/DTS values
         if ( payloadStart )
         {
            if ( (packet[payload] == 0x00) && (packet[payload+1] == 0x00) && (packet[payload+2] == 0x01) )
            {
               int pesHeaderDataLen= packet[payload+8];
               int tsbase= payload+9;
               long long timeOffset;
               long long PTS= 0, DTS=0;
               long long rateAdjustedPTS= 0, rateAdjustedDTS=0;

               if ( packet[payload+7] & 0x80 )
               {     
                  PTS= readTimeStamp( &packet[tsbase] );

                  timeOffset= PTS-m_baseTime;
                  rateAdjustedPTS= (((long long)(timeOffset+0.5)+m_segmentBaseTime)&0x1FFFFFFFFLL);
                  
                  if( pid == m_audioPid && m_b2bSavedAudioPacketsCnt > 0  )
                  {
                    timeOffset= PTS-m_prevAdBaseTime;
                    rateAdjustedPTS= (((long long)(timeOffset+0.5)+m_prevAdSegmentBaseTime)&0x1FFFFFFFFLL);
                  }

                  writeTimeStamp( &packet[tsbase], packet[tsbase]>>4, rateAdjustedPTS );
                  if ( pid == m_pcrPid )
                  {
                     m_currentMappedPTS= rateAdjustedPTS;
                  }
                  tsbase += 5;
                  if( pid == m_videoPid )
                  {
                    m_lastAdVideoFramePTS = PTS;
                  }
                  if (pid == m_videoPid)
                  {
                    long long elapsed= getCurrentTimeMicro()-m_insertStartTime;
                    int inFrameCnt = (elapsed*VIDEO_FRAME_RATE)/1000000;
                    
                    int timeDeltaFrames = ((elapsed*VIDEO_FRAME_RATE)/1000000) - m_adVideoFrameCount;

                    if (m_adVideoFrameCount >= (m_netReplaceableVideoFramesCnt - 120))
                    {
                        if( !m_adVideoPktsDelayed && timeDeltaFrames > 8 )
                        {
                            DEBUG("Ad Video Packets delayed timeDeltaFrames %d ", timeDeltaFrames );
                            m_adVideoPktsDelayed = true;
                        }
                        else if( m_adVideoPktsDelayed && timeDeltaFrames < -4 ) 

                        {
                            DEBUG("Ad Video Packets back to normal, timeDeltaFrames %d ", timeDeltaFrames );
                            m_adVideoPktsDelayed = false;
                        }
                        m_adVideoPktsTooFast = false;
                        m_adVideoPktsDelayedPCRBased = false;
                    }
                  }
               }
               if ( packet[payload+7] & 0x40 )
               {
                  DTS= readTimeStamp( &packet[tsbase] );                 
                  timeOffset= DTS-m_baseTime;
                  rateAdjustedDTS= (((long long)(timeOffset+0.5)+m_segmentBaseTime)&0x1FFFFFFFFLL);
                  if( pid == m_audioPid && m_b2bSavedAudioPacketsCnt > 0  )
                  {
                    timeOffset= DTS-m_prevAdBaseTime;
                    rateAdjustedDTS= (((long long)(timeOffset+0.5)+m_prevAdSegmentBaseTime)&0x1FFFFFFFFLL);
                  }
                  writeTimeStamp( &packet[tsbase], packet[tsbase]>>4, rateAdjustedDTS );
                  tsbase += 5;
               }

                if (rateAdjustedDTS == 0) {
                    rateAdjustedDTS = rateAdjustedPTS;
                    DTS = PTS;
                }               

                if( m_currentMappedPCR != -1 && pid == m_audioPid && rateAdjustedDTS != 0 )
                { 
                    if( !m_adAudioPktsDelayed && ( m_currentMappedPCR > rateAdjustedDTS || ((m_adVideoFrameCount >= (m_netReplaceableVideoFramesCnt - 120)) && (rateAdjustedDTS - m_currentMappedPCR)/90000.0 < 0.5) ) )
                    {
                        m_adAudioPktsDelayed = true;
                        m_adVideoPktsTooFast = false;
                        DEBUG("Ad Audio Packets delayed, decoding delay %f m_currentMappedPCR %llx rateAdjustedDTS %llx ", (rateAdjustedDTS - m_currentMappedPCR) / 90000.0, m_currentMappedPCR, rateAdjustedDTS );
                    }
                    else if( m_adAudioPktsDelayed && ( ((m_adVideoFrameCount < (m_netReplaceableVideoFramesCnt - 120)) && m_currentMappedPCR <= rateAdjustedDTS) || ((m_adVideoFrameCount >= (m_netReplaceableVideoFramesCnt - 120)) && (rateAdjustedDTS - m_currentMappedPCR)/90000.0 > 0.6 ) ) ) 
                    {
                        m_adAudioPktsDelayed = false;
                        DEBUG("Ad Audio Packets back to normal, decoding delay %f m_currentMappedPCR %llx rateAdjustedDTS %llx ", (rateAdjustedDTS - m_currentMappedPCR) / 90000.0, m_currentMappedPCR, rateAdjustedDTS );
                    }
                } 

               if ( packet[payload+7] & 0x02 )
               {
                  // CRC flag is set.  Following the PES header will be the CRC for the previous PES packet
                  WARNING("Warning: PES packet has CRC flag set");
               }
               payload= payload+9+pesHeaderDataLen;
            }                
         }
      }
      packet += m_packetSize;
   }
}

// PTS/DTS format:
// YYYY vvvM vvvv vvvv vvvv vvvM vvvv vvvv vvvv vvvM
//
// value formed by concatinating all 'v''s.
// M bits are 1, YYYY are 0010 for PTS only
// and for PTS+DTS 0011 and 0001 respectively
//
// From ISO 13818-1
long long RBIStreamProcessor::readTimeStamp( unsigned char *p )
{
   long long TS;
   
   if ( (p[4] & 0x01) != 1 )
   {
      WARNING("TS:============ TS p[4] bit 0 not 1");
   }
   if ( (p[2] & 0x01) != 1 )
   {
      WARNING("TS:============ TS p[2] bit 0 not 1");
   }
   if ( (p[0] & 0x01) != 1 )
   {
      WARNING("TS:============ TS p[0] bit 0 not 1");
   }
   switch ( (p[0] & 0xF0) >> 4 )
   {
      case 1:
      case 2:
      case 3:
         break;
      default:
         WARNING("TS:============ TS p[0] YYYY bits have value %X", p[0] );
         break;
   }
   
   TS= ((((long long)(p[0]&0x0E))<<30)>>1) |
       (( (long long)(p[1]&0xFF))<<22) |
       ((((long long)(p[2]&0xFE))<<15)>>1) |
       (( (long long)(p[3]&0xFF))<<7) |
       (( (long long)(p[4]&0xFE))>>1);
   
   return TS;
}

void RBIStreamProcessor::writeTimeStamp( unsigned char *p, int prefix, long long TS )
{
   p[0]= (((prefix&0xF)<<4)|(((TS>>30)&0x7)<<1)|0x01);
   p[1]= ((TS>>22)&0xFF);
   p[2]= ((((TS>>15)&0x7F)<<1)|0x01);
   p[3]= ((TS>>7)&0xFF);
   p[4]= ((((TS)&0x7F)<<1)|0x01);   
}

long long RBIStreamProcessor::readPCR( unsigned char *p )
{
   long long PCR= (((long long)(p[0]&0xFF))<<(33-8)) |
                  (((long long)(p[1]&0xFF))<<(33-8-8)) |
                  (((long long)(p[2]&0xFF))<<(33-8-8-8)) |
                  (((long long)(p[3]&0xFF))<<(33-8-8-8-8)) |
                  (((long long)(p[4]&0xFF))>>7);
   return PCR;
}

void RBIStreamProcessor::writePCR( unsigned char *p, long long PCR )
{
   p[0]= ((PCR>>(33-8))&0xFF);
   p[1]= ((PCR>>(33-8-8))&0xFF);
   p[2]= ((PCR>>(33-8-8-8))&0xFF);
   p[3]= ((PCR>>(33-8-8-8-8))&0xFF);
   p[4]= ((PCR&0x01)<<7)|(p[4]&0x7F);
}

// Parse through the sequence parameter set data to determine the frame size
bool RBIStreamProcessor::processSeqParameterSet( unsigned char *p, int length )
{
   bool result= false;
   int profile_idc;
   int seq_parameter_set_id;
   int mask= 0x80;
   int frameWidth= -1;
   int frameHeight= -1;
   
   profile_idc= p[0];
   
   // constraint_set0_flag : u(1)
   // constraint_set1_flag : u(1)
   // constraint_set2_flag : u(1)
   // constraint_set3_flag : u(1)
   // constraint_set4_flag : u(1)
   // constraint_set5_flag : u(1)
   // reserved_zero_2bits : u(2)
   // level_idc : u(8)
   
   p += 3;
   
   seq_parameter_set_id= getUExpGolomb( p, mask );
   
   m_SPS[seq_parameter_set_id].separateColorPlaneFlag= 0;
   switch( profile_idc )
   {
      case 44:  case 83:  case 86:  
      case 100: case 110: case 118: 
      case 122: case 128: case 244:
         {
            int chroma_format_idx= getUExpGolomb( p, mask );
            if ( chroma_format_idx == 3 )
            {
               // separate_color_plane_flag
               m_SPS[seq_parameter_set_id].separateColorPlaneFlag= getBits( p, mask, 1 );
            }
            // bit_depth_luma_minus8
            getUExpGolomb( p, mask );
            // bit_depth_chroma_minus8
            getUExpGolomb( p, mask );
            // qpprime_y_zero_transform_bypass_flag
            getBits( p, mask, 1 );
            
            int seq_scaling_matrix_present_flag= getBits( p, mask, 1 );
            if ( seq_scaling_matrix_present_flag )
            {
               int imax= ((chroma_format_idx != 3) ? 8 : 12);
               for( int i= 0; i < imax; ++i )
               {
                  int seq_scaling_list_present_flag= getBits( p, mask, 1 );
                  if ( seq_scaling_list_present_flag )
                  {
                     if ( i < 6 )
                     {
                        processScalingList( p, mask, 16 );
                     }
                     else
                     {
                        processScalingList( p, mask, 64 );
                     }
                  }
               } 
            }
         }
         break;
   }

   // log2_max_frame_num_minus4
   int log2_max_frame_num_minus4= getUExpGolomb( p, mask );
   m_SPS[seq_parameter_set_id].log2MaxFrameNumMinus4= log2_max_frame_num_minus4;

   int pic_order_cnt_type= getUExpGolomb( p, mask );
   m_SPS[seq_parameter_set_id].picOrderCountType= pic_order_cnt_type;
   if ( pic_order_cnt_type == 0 )
   {
      // log2_max_pic_order_cnt_lsb_minus4
      int log2_max_pic_order_cnt_lsb_minus4= getUExpGolomb( p, mask );
      m_SPS[seq_parameter_set_id].log2MaxPicOrderCntLsbMinus4= log2_max_pic_order_cnt_lsb_minus4; 
      m_SPS[seq_parameter_set_id].maxPicOrderCount= (2<<(log2_max_pic_order_cnt_lsb_minus4+4));
   }
   else if ( pic_order_cnt_type == 1 )
   {
      // delta_pic_order_always_zero_flag
      getBits( p, mask, 1 );
      // offset_for_non_ref_pic
      getSExpGolomb( p, mask );
      // offset_for_top_top_bottom_field
      getSExpGolomb( p, mask );
      
      int num_ref_frames_in_pic_order_cnt_cycle= getUExpGolomb( p, mask );
      for( int i= 0; i < num_ref_frames_in_pic_order_cnt_cycle; ++i )
      {
         // offset_for_ref_frame[i]
         getUExpGolomb( p, mask );
      }
   }

   // max_num_ref_frames   
   getUExpGolomb( p, mask );
   
   // gaps_in_frame_num_value_allowed_flag
   getBits( p, mask, 1 );
   
   int pic_width_in_mbs_minus1= getUExpGolomb( p, mask );

   int pic_height_in_map_units_minus1= getUExpGolomb( p, mask );
   
   int frame_mbs_only_flag= getBits( p, mask, 1 );
   m_SPS[seq_parameter_set_id].frameMBSOnlyFlag= frame_mbs_only_flag;

   frameWidth= (pic_width_in_mbs_minus1+1)*16;
   
   frameHeight= (pic_height_in_map_units_minus1+1)*16;
   
   m_isInterlaced= (frame_mbs_only_flag == 0 );
   m_isInterlacedKnown= true;
   
   if ( m_isInterlaced )
   {
      frameHeight *= 2;

      if ( !frame_mbs_only_flag )
      {
         // mb_adaptive_frame_field_flag
         getBits( p, mask, 1 );
      }      
   }

   if ( (m_frameWidth == -1) || (frameWidth != m_frameWidth) ||
        (m_frameHeight == -1) || (frameHeight != m_frameHeight) )
   {
      m_frameWidth= frameWidth;
      m_frameHeight= frameHeight;
      DEBUG("H.264 sequence frame size %dx%d interlaced=%d", m_frameWidth, m_frameHeight, m_isInterlaced );
   }
   
   return result;
}

// Consume all bits used by the scaling list
void RBIStreamProcessor::processScalingList( unsigned char *& p, int& mask, int size )
{   
   int nextScale= 8;
   int lastScale= 8;
   for( int j= 0; j < size; ++j )
   {
      if ( nextScale )
      {
         int deltaScale= getSExpGolomb( p, mask );
         nextScale= (lastScale + deltaScale + 256) % 256;
      }
      lastScale= (nextScale == 0 ) ? lastScale : nextScale;
   }
}

unsigned int RBIStreamProcessor::getBits( unsigned char *& p, int& mask, int bitCount )
{
   int bits= 0;
   while( bitCount )
   {
      --bitCount;
      bits <<= 1;
      if (*p & mask)
      {
         bits |= 1;
      }
      mask >>= 1;
      if ( mask == 0 )
      {
         ++p;
         mask= 0x80;
      }
   }   
   return bits;
}

unsigned int RBIStreamProcessor::getUExpGolomb( unsigned char *& p, int& mask )
{
   int codeNum= 0;
   int leadingZeros= 0;
   int factor= 2;
   bool bitClear;
   do
   {
      bitClear= !(*p & mask);
      mask >>= 1;
      if ( mask == 0 )
      {
         ++p;
         mask= 0x80;
      }
      if ( bitClear )
      {
         ++leadingZeros;
         codeNum += (factor>>1);
         factor <<= 1;
      }
   }
   while( bitClear );
   if ( leadingZeros )
   {
      codeNum += getBits( p, mask, leadingZeros );
   }
   return codeNum;   
}

int RBIStreamProcessor::getSExpGolomb( unsigned char *& p, int& bit )
{
   unsigned int u= getUExpGolomb( p, bit );
   int n= (u+1)>>1;
   if ( !(u & 1) )
   {
      n= -n;
   }
   return n;
}

