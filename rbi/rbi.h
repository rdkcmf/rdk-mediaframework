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

#ifndef _RBI_H
#define _RBI_H

#include <string>
#include <map>
#include <vector>
#include <list>
#include <algorithm>

#include <uuid/uuid.h>
#include "rbirpc.h"
#include <mutex>
#include <thread>


#include "pxcore/rtObject.h"
#include "pxcore/rtHttpCache.h"
#include "pxcore/rtFileCache.h"
#include "pxcore/rtFileDownloader.h"

#define MAX_PACKET_SIZE (192)
#define MAX_TABLE_AGGREGATOR_CAPACITY (2)
#define TABLE_AGGREGATOR_INDEX_PMT (0)
#define TABLE_AGGREGATOR_INDEX_SCTE35 (1)
#define MAX_PMT_PACKET_BUFFER_SIZE (192 * 7) //Sufficient to cover a full size PMT plus some extra room for space lost to adaptation field.
#define MAX_SCTE35_PACKET_BUFFER_SIZE (192 * 7) //Sufficient to cover a full size SCTE-35 plus some extra room for space lost to adaptation field.

#define INT_FATAL(FORMAT, ...)      RDK_LOG(RDK_LOG_FATAL, "LOG.RDK.RBI", FORMAT "\n", __VA_ARGS__)
#define INT_ERROR(FORMAT, ...)      RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.RBI", FORMAT "\n", __VA_ARGS__)
#define INT_WARNING(FORMAT, ...)    RDK_LOG(RDK_LOG_WARN, "LOG.RDK.RBI", FORMAT "\n",  __VA_ARGS__)
#define INT_INFO(FORMAT, ...)       RDK_LOG(RDK_LOG_INFO, "LOG.RDK.RBI", FORMAT "\n",  __VA_ARGS__)
#define INT_DEBUG(FORMAT, ...)      RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.RBI", FORMAT "\n", __VA_ARGS__)
#define INT_TRACE1(FORMAT, ...)     RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.RBI", FORMAT "\n",  __VA_ARGS__)
#define INT_TRACE2(FORMAT, ...)     RDK_LOG(RDK_LOG_TRACE2, "LOG.RDK.RBI", FORMAT "\n",  __VA_ARGS__)
#define INT_TRACE3(FORMAT, ...)     RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.RBI", FORMAT "\n",  __VA_ARGS__)
#define INT_TRACE4(FORMAT, ...)     RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.RBI", FORMAT "\n",  __VA_ARGS__)

#define FATAL(...)                  INT_FATAL(__VA_ARGS__, "")
#define ERROR(...)                  INT_ERROR(__VA_ARGS__, "")
#define WARNING(...)                INT_WARNING(__VA_ARGS__, "")
#define INFO(...)                   INT_INFO(__VA_ARGS__, "")
#define DEBUG(...)                  INT_DEBUG(__VA_ARGS__, "")
#define TRACE1(...)                 INT_TRACE1(__VA_ARGS__, "")
#define TRACE2(...)                 INT_TRACE2(__VA_ARGS__, "")
#define TRACE3(...)                 INT_TRACE3(__VA_ARGS__, "")
#define TRACE4(...)                 INT_TRACE4(__VA_ARGS__, "")

class RBIStreamProcessor;
class RBIInsertSource;


class RBIInsertSourceEvents
{
   public:
      RBIInsertSourceEvents() {}
      
     virtual ~RBIInsertSourceEvents() {}
     
     virtual void sourceError( RBIInsertSource *src ) {}
     virtual void httpTimeoutError( RBIInsertSource *src ) {}
     virtual void tsFormatError( RBIInsertSource *src ) {}
     virtual void insertionStatusUpdate( RBIStreamProcessor *sp, int detailCode ){}
     virtual bool getTriggerDetectedForOpportunity(void){return false; }
};

class RBIInsertSource
{
   public:
      RBIInsertSource(int id) : m_maxBuffering( 1*1024*1024 ), m_id(id) {}
      virtual ~RBIInsertSource() {}
  
      virtual long long splicePTS()= 0;
      virtual bool dataReady()= 0;
      virtual long long startPTS()= 0;
      virtual long long startPCR()= 0;
      virtual int bitRate()=0 ;
      virtual int avail()= 0;
      virtual int read( unsigned char *buffer, int length )=0;
      virtual unsigned int getTotalRetryCount()=0;
      virtual RBIStreamProcessor& getStreamProcessor()= 0;
      
      virtual std::vector<int> getAdVideoExitFrameInfo()= 0;
      virtual int getNextAdPacketPid()=0;

      int getId()
      {
         return m_id;
      }
   
      void setEvents( RBIInsertSourceEvents *events )
      {
         m_sourceEvents= events;
      }
      
   protected:      
      int m_maxBuffering;
      RBIInsertSourceEvents *m_sourceEvents;

   private:
      
      void setMaxBuffering( int max )
      {
         m_maxBuffering= max;
      }
      virtual bool open()= 0;
      virtual void close()= 0;
      
      int m_id;
      
      friend class RBIManager;    
};

class RBIInsertDefinition
{
   public:
      RBIInsertDefinition( int triggerId, bool replace, int duration, const char *assetID, const char* providerID, const char *uri, const char *trackingIdString, bool validSpot, int detailCode );
     ~RBIInsertDefinition() {}
      
      int getTriggerId() const
      {
         return m_triggerId;
      }
      
      bool isReplace() const
      {
         return m_replace;
      }
      
      int getDuration() const
      {
         return m_duration;
      }
      
      const char* getAssetID() const
      {
         return m_assetID.c_str();
      }

      const char* getProviderID() const
      {
         return m_providerID.c_str();
      }
      
      const char* getUri() const
      {
         return m_uri.c_str();
      }
      
      const char* getTrackingIdString() const
      {
         return m_trackingIdString.c_str();
      }

      bool isSpotValid() const
      {
         return m_validSpot;
      }	  
      
      int getSpotDetailCode() const
      {
         return m_spotDetailCode;
      }

      RBIInsertSource* getInsertSource() const;
      
   private:
      int m_triggerId;
      bool m_replace;
      bool m_validSpot;
      int m_duration;
      int m_spotDetailCode;
      std::string m_assetID;
      std::string m_providerID;
      std::string m_uri;
      std::string m_trackingIdString;
};

typedef struct _SpotTimeInfo
{
   long long utcTimeStart;
   long long startPTS;
} SpotTimeInfo;      
      
class RBIInsertDefinitionSet
{
   public:
      RBIInsertDefinitionSet( long long utcTrigger, long long utcSplice, long long splicePTS );
     ~RBIInsertDefinitionSet() {}
      
      int getNumSpots()
      {
         return m_spots.size();
      }

      long long getUTCTriggerTime()
      {
         return m_utcTimeTrigger;
      }
      
      long long getUTCSpliceTime()
      {
         return m_utcTimeSplice;
      }
      
      long long getSplicePTS()
      {
         return m_splicePTS;
      }

      bool addSpot( RBIInsertDefinition spot );
      const RBIInsertDefinition* getSpot( int index );
      const SpotTimeInfo* getSpotTime( int index );
      
   private:
      int m_refCount;
      long long m_utcTimeTrigger;
      long long m_utcTimeSplice;
      long long m_splicePTS;
      std::vector<RBIInsertDefinition> m_spots;
      std::vector<SpotTimeInfo> m_spotTimes;
      
      friend class RBIManager;
};

class RBITrigger
{
   public:
      int getUnifiedId()
      {
         return m_unifiedId;
      }
      
      int getEventContext()
      {
         return m_eventContext;
      }
      
      int getEventId()
      {
         return m_eventId;
      }
      
      int getEventIdInstance()
      {
         return m_eventIdInstance;
      }
      
      long long getSplicePTS()
      {
         return m_splicePTS;
      }
      
      long long getUTCTriggerTime()
      {
         return m_utcTimeTrigger;
      }
      
      long long getUTCSpliceTime()
      {
         return m_utcTimeSplice;
      }

      const PlacementOpportunityData *poData()
      {
         return m_poData;
      }      

   private:
      RBITrigger( int ctx, int id, int idInstance, long long splicePTS, long long leadTime );
      
     ~RBITrigger() {}

     void setPOData( const PlacementOpportunityData *segmentUpidElement )
     {
        m_poData = segmentUpidElement;
     }

     int m_eventContext;
     int m_eventId;
     int m_eventIdInstance;
     long long m_splicePTS;
     long long m_utcTimeTrigger;
     long long m_utcTimeSplice;
     int m_unifiedId;
     const PlacementOpportunityData* m_poData;

     friend class RBIStreamProcessor;
};

class RBIStreamProcessorEvents
{
   public:
      RBIStreamProcessorEvents() {}
      
     virtual ~RBIStreamProcessorEvents() {}
     
     virtual void acquiredPAT( RBIStreamProcessor *sp ) {}
     virtual void acquiredPMT( RBIStreamProcessor *sp ) {}
     virtual void foundFrame( RBIStreamProcessor *sp, int frameType, long long frameStartOffset, long long pts, long long pcr ) {}
     virtual void foundBitRate( RBIStreamProcessor *sp, int bitRate, int chunkSize ) {}
     virtual bool triggerDetected( RBIStreamProcessor *sp, RBITrigger* trigger ) {return false;}
     virtual bool getTriggerDetectedForOpportunity(void) {return false;}
     virtual std::string getSourceUri(void){ return std::string(); }
     virtual void setTriggerDetectedForOpportunity(bool bTriggerDetectedForOpportunity) {}
     virtual void insertionError( RBIStreamProcessor *sp, int detailCode ) {}
     virtual void insertionStarted( RBIStreamProcessor *sp ) {}
     virtual void insertionStatusUpdate( RBIStreamProcessor *sp, int detailCode ) {}
     virtual void insertionStopped( RBIStreamProcessor *sp ) {}
     virtual void privateData( RBIStreamProcessor *sp, int streamType, unsigned char *packet, int len ) {}
};

struct RBIStreamComponent
{
   int streamType;
   int pid;
   char *associatedLanguage;
};

typedef struct _RBIPriaveDataStreamSpec
{
   int streamType;
   bool matchTag;
   unsigned char descrTag;
   int descrDataLen;
   unsigned char descrData[32];
} RBIPrivateDataStreamSpec;

typedef struct _RBIPrivateDataMonitor
{
   int numStreams;
   RBIPrivateDataStreamSpec specs[5];
} RBIPrivateDataMonitor; 

#define MAX_PACKET_REMAINDER ((3*MAX_PACKET_SIZE)+4)

// Maximum number of bytes needed to examine in a start code
#define RBI_SCAN_REMAINDER_SIZE (8)

class RBIStreamProcessor
{
   private:
       typedef struct
       {
           unsigned char * m_section;
           unsigned int m_sectionLength;
           unsigned int m_sectionWriteOffset;
           unsigned char * m_packets;
           unsigned int m_packetWriteOffset;
           unsigned int m_nextContinuity;
       } tableInfo_t;
      class RBIPidMap
      {
         public:
         
           ~RBIPidMap() {}

            RBIPidMap() {}
         
            int m_pmtPid;
            unsigned char m_replacementPAT[MAX_PACKET_SIZE];
            unsigned char m_replacementPMT[MAX_PMT_PACKET_BUFFER_SIZE];
            unsigned int m_replacementPMTLength;
            unsigned int m_replacementPMTReadOffset;
            bool m_psiInsertionIncomplete;
            unsigned short m_pidMap[8192];
            bool m_needPayloadStart[8192];
            bool m_pidIsMapped[8192];
      };
   
   public:
      RBIStreamProcessor();
      
     ~RBIStreamProcessor();

      void setTriggerEnable( bool enable )
      {
         m_triggerEnable= enable;
      }
      
      bool getTriggerEnable()
      {
         return m_triggerEnable;
      }

      void setPrivateDataEnable( bool enable )
      {
         m_privateDataEnable= enable;
      }
      
      bool getPrivateDataEnable()
      {
         return m_privateDataEnable;
      }
      
      void setPrivateDataMonitor( RBIPrivateDataMonitor mon )
      {
         m_privateDataMonitor= mon;
         m_havePMT= false;
      }

      void setFrameDetectEnable( bool enable )
      {
         m_frameDetectEnable= enable;
      }
      
      bool getFrameDetectEnable()
      {
         return m_frameDetectEnable;
      }

      void setTrackContinuityCountersEnable( bool enable )
      {
         m_trackContinuityCountersEnable= enable;
      }
      
      bool getTrackContinuityCountersEnable()
      {
         return m_trackContinuityCountersEnable;
      }
      
      void setBackToBack( bool backToBack )
      {
         m_backToBack= backToBack;
      }
      
      bool getBackToBack()
      {
         return m_backToBack;
      }
      
      bool isInserting()
      {
         return m_mapping;
      }
      
      long long getCurrentPTS()
      {
         return m_currentPTS;
      }

      long long getCurrentPCR()
      {
         return m_currentPCR;
      }

      long long getLeadTime()
      {
         return m_leadTime;
      }

      void setEvents( RBIStreamProcessorEvents *events )
      {
         m_events= events;
      }

      void setAudioInsertCount( int insertCount )
      {
         m_audioInsertCount= insertCount;
      }
      int getLastAudioInsertCount()
      {
         return m_audioInsertCount;
      }
      void setLastBufferInsertSize( int size )
      {
         m_lastBufferInsertSize= size;
      }
      int getLastBufferInsertSize()
      {
         return m_lastBufferInsertSize;
      }

      void markNextGOPBroken();
      
      void useSI(RBIStreamProcessor *other); 
      
      bool startInsertion( RBIInsertSource *insert, long long startPTS, long long endPTS );
      
      void stopInsertion();

      int getInsertionDataSize();
      
      int getInsertionData( unsigned char* packets, int size );

      bool isNextPacketAudio();
      int getInsertionDataDuringTransitionToAd(unsigned char*);
      void reTimestampDuringTransitionToAd(unsigned char*, int);
      int getInsertionAudioDataDuringTransitionBackToNetwork(unsigned char*);
      void reTimestampDuringTransitionBack(unsigned char*, int, long long int, unsigned int);
   
      int processPackets( unsigned char* packets, int size, int *isTSFormatError );
      float getRetimestampRate()
      {
          return m_rate;
      }

      char* getTimeSignalBase64Msg()
      {
         return m_TimeSignalbase64Msg;
      }

      long long getInsertionStartTime(void)
      {
         return m_insertStartTime;
      }

      long long getInsertionEndTime()
      {
         return m_insertEndTime;
      }

      void setInsertionEndTime(long long insertEndTime)
      {
         m_insertEndTime = insertEndTime;
      }

      long long getSpliceUpdatedOutPTS(void)
      {
        return m_mapSpliceUpdatedOutPTS;
      }

   private:
      bool setComponentInfo( int pcrPid,
                             int videoCount, int *videoPids, int *videoTypes,
                             int audioCount, int *audioPids, int *audioTypes, char **audioLanguages,
                             int dataCount, int *dataPids, int *dataTypes,
                             int privateCount, int *privatePids, int *privateTypes );
      void termComponentInfo();
      void termComponent( RBIStreamComponent *component );
      int processPacketsAligned( unsigned char* packets, int size, int *isTSFormatError );
      void processPMTSection( unsigned char* section, int sectionLength );
      void processSCTE35Section( unsigned char* section, int sectionLength );
      bool processStartCode( unsigned char *buffer, bool& keepScanning, int length, int base );
      bool processSCTEDescriptors( unsigned char *packet, int cmd, int descriptorOffset, int descriptorLoopLength, long long spliceTime );
      void reTimestamp( unsigned char *packet, int length );
      long long readTimeStamp( unsigned char *p );
      void writeTimeStamp( unsigned char *p, int prefix, long long TS );
      long long readPCR( unsigned char *p );
      void writePCR( unsigned char *p, long long PCR );
      bool processSeqParameterSet( unsigned char *p, int length );
      void processScalingList( unsigned char *& p, int& mask, int size );
      unsigned int getBits( unsigned char *& p, int& mask, int bitCount );
      unsigned int getUExpGolomb( unsigned char *& p, int& mask );
      int getSExpGolomb( unsigned char *& p, int& mask );

        typedef struct _AC3FrameInfo
        {
           int duration;
           int size;
           int byteCnt;
           bool syncByteDetectedInPacket;
           int pesPacketLength;
           unsigned char *pesPacketLengthMSB;
           unsigned char *pesPacketLengthLSB;
           int pesHeaderDataLen;
           int frameCnt;
        } AC3FrameInfo;

      long long getAC3FramePTS( unsigned char* packet, AC3FrameInfo *ac3FrameInfo, long long m_currentAudioPTS);
      void getAC3FrameSizeDuration(unsigned char* packet, int offset, AC3FrameInfo *ac3FrameInfo);

      bool m_havePAT;
      int m_versionPAT;
      int m_program;
      int m_pmtPid;
      tableInfo_t m_tableAggregator[MAX_TABLE_AGGREGATOR_CAPACITY];

      bool m_havePMT;
      int m_versionPMT;

      int m_pcrPid;
      int m_videoPid;
      int m_audioPid;
      
      bool m_triggerEnable;
      char m_triggerPids[8192];

      bool m_privateDataEnable;
      char m_privateDataPids[8192];
      unsigned char m_privateDataTypes[8192];
      
      RBIPrivateDataMonitor m_privateDataMonitor;

      bool m_isH264;
      bool m_inSync;
      int m_packetSize;
      int m_ttsSize;
      int m_remainderOffset;
      int m_remainderSize;
      unsigned char m_remainder[MAX_PACKET_REMAINDER];

      bool m_frameDetectEnable;
      long long m_streamOffset;
      long long m_frameStartOffset;
      int m_bitRate;
      int m_avgBitRate;
      unsigned int m_ttsInsert;
      int m_frameWidth;
      int m_frameHeight;
      bool m_isInterlaced;
      bool m_isInterlacedKnown;
      int m_frameType;
      bool m_scanForFrameType;
      bool m_scanHaveRemainder;
      bool m_prescanForFrameType;
      bool m_prescanHaveRemainder;
      unsigned char m_scanRemainder[RBI_SCAN_REMAINDER_SIZE*3];
      unsigned char m_prescanRemainder[RBI_SCAN_REMAINDER_SIZE*3];

      int m_emulationPreventionCapacity;
      int m_emulationPreventionOffset;
      unsigned char * m_emulationPrevention;
      
      typedef struct _H264SPS
      {
         int picOrderCountType;
         int maxPicOrderCount;
         int log2MaxFrameNumMinus4;
         int log2MaxPicOrderCntLsbMinus4;
         int separateColorPlaneFlag;
         int frameMBSOnlyFlag;
      } H264SPS;
      
      H264SPS m_SPS[32];

      bool m_haveBaseTime;
      long long m_baseTime;
      long long m_basePCR;
      long long m_segmentBaseTime;
      long long m_segmentBasePCR;
      long long m_currentPCR;
      long long m_prevCurrentPCR;
      long long m_currentPTS;
      long long m_leadTime;
      long long m_currentAudioPTS;
      long long m_currentMappedPTS;
      long long m_currentMappedPCR;
      long long m_currentInsertPCR;
      double m_rate;

      long long m_sample1PCR;
      long long m_sample1PCROffset;
      long long m_sample2PCR;
      long long m_sample2PCROffset;

      bool m_trackContinuityCountersEnable;
      char m_continuityCounters[8192];

      unsigned char m_pat[MAX_PACKET_SIZE];
      unsigned char m_pmt[MAX_PACKET_SIZE];

      int m_GOPSize;
      int m_frameInGOPCount;

      bool m_mapStartPending;
      bool m_mapStartArmed;
      bool m_mapping;
      bool m_mapEndPending;
      bool m_mapEndPendingPrev;
      bool m_mapEnding;
      bool m_mapEndingNeedVideoEOF;
      bool m_backToBack;
      bool m_backToBackPrev;
      bool m_randomAccess;
      bool m_randomAccessPrev;
      bool m_startOfSequence;
      bool m_startOfSequencePrev;
      bool m_mapEditGOP;
      int m_spliceOffset;
      long long m_mapSpliceOutPTS;
      long long m_mapSpliceInPTS;
      long long m_mapSpliceOutAbortPTS;
      long long m_mapSpliceInAbortPTS;
      long long m_mapSpliceInAbort2PTS;
      long long m_mapStartPTS;
      long long m_mapStartPCR;
      long long m_insertStartTime;
      long long m_insertEndTime;
      long long m_lastInsertBufferTime;
      long long m_nextInsertBufferTime;
      long long m_lastInsertGetDataDuration;
      int m_insertByteCount;
      int m_insertByteCountError;
      int m_minInsertBitRateOut;
      int m_maxInsertBitRateOut;
      RBIPidMap *m_map;
      RBIInsertSource *m_insertSrc;
         
      int m_videoComponentCount;
      RBIStreamComponent *m_videoComponents;
      int m_audioComponentCount;
      RBIStreamComponent *m_audioComponents;
      int m_dataComponentCount;
      RBIStreamComponent *m_dataComponents;
      
      RBIStreamProcessorEvents *m_events;
      
      bool m_filterAudio;
      long long m_filterAudioStopPTS;
      long long m_filterAudioStop2PTS;

      bool m_H264Enabled;
      bool m_captureEnabled;
      bool m_audioReplicationEnabled;
      int m_captureEndCount;
      FILE *m_outFile;
      int m_audioInsertCount;
      int m_lastBufferInsertSize;
      bool m_replicatingAudio;
      int m_replicateAudioSrcPid;
      int m_replicateAudioStreamType;
      std::vector<int> m_replicateAudioTargetPids;
      unsigned char m_replicateAudioPacket[MAX_PACKET_SIZE];
      unsigned int m_replicateAudioIndex;
      char *m_TimeSignalbase64Msg;
      
      unsigned int m_adVideoFrameCount;
      unsigned int m_adReadVideoFrameCnt; 
      bool m_transitioningToAd;
      bool m_transitioningToNetwork;
      unsigned int m_splicePTS_offset;
      unsigned int m_netReplaceableVideoFramesCnt;
      long long  m_lastAdVideoFramePTS;
      bool m_maxAdVideoFrameCountReached;
      bool m_maxAdAudioFrameCountReached;
      bool m_lastAdVideoFrameDetected;

      bool m_haveSegmentBasePCRAfterTransitionToAd;
      long long m_lastAdPCR;
      long long m_firstAdPacketAfterTransitionTTSValue;
      unsigned int m_lastPCRTTSValue;
      unsigned int m_lastMappedPCRTTSValue;
      bool PCRBaseTime;
      bool m_adAudioPktsDelayed;
      bool m_adVideoPktsDelayed;
      bool m_adVideoPktsDelayedPCRBased;
      bool m_adVideoPktsTooFast;
      bool m_adPktsAccelerationEnable;
      bool m_skipNullPktsInAd;
      unsigned int m_ac3FrameByteCnt;
      bool m_adAccelerationState;
      bool m_removeNull_PSIPackets;
      bool m_determineNetAC3FrameSizeDuration;
      bool m_determineAdAC3FrameSizeDuration;
      int m_spliceAudioPTS_offset;
      long long m_currentAdAudioPTS;
      int m_adFramePacketCount;
      bool m_delayAdAudioPacketDuringTransition;
      long long m_mapSpliceUpdatedOutPTS;
      unsigned char *m_b2bPrevAdAudioData;
      unsigned char *m_b2bPrevAdAudioDataCopy;
      unsigned int m_b2bSavedAudioPacketsCnt;
      unsigned int m_b2bBackedAdAudioPacketsCnt;
      long long m_prevAdBaseTime;
      long long m_prevAdSegmentBaseTime;


        AC3FrameInfo netAC3FrameInfo;
        AC3FrameInfo adAC3FrameInfo;
};

class RBIContext : public RBIStreamProcessorEvents, public RBIInsertSourceEvents
{
   public:
      void setSourceURI( const char *uri );
      void setPrivateDataMonitor( RBIPrivateDataMonitor mon );
      bool processPackets( unsigned char* packets, int* size );
      int getInsertionDataSize();
      int getInsertionData( unsigned char* packets, int size );
      std::string getSourceUri(void);
      
   private:
      pthread_mutex_t m_mutex;
      int m_refCount;
      int m_sessionNumber;
      uuid_t m_sessionId;
      std::string m_sessionIdStr;
      bool m_insertPending;
      bool m_inserting;
      bool m_tuneInEmitted;
      bool m_insertionStartedEmitted;
      bool m_insertionStatusUpdateEmitted;
      int m_triggerId;
      std::string m_sourceUri;
      unsigned char m_deviceId[6];
      bool m_newMonPending;
      RBIPrivateDataMonitor m_mon;

      RBIStreamProcessor *m_primaryStreamProcessor;
      RBIInsertDefinitionSet *m_dfnSet;
      const RBIInsertDefinition *m_dfnCurrent;
      const RBIInsertDefinition *m_dfnNext;
      int m_spotIndex;
      int m_spotCount;
      bool m_spotIsReplace;
      long long m_utcTimeSpot;
      long long m_utcSpliceTime;
      long long m_prefetchPTS;
      long long m_startPTS;
      long long m_endPTS;
      long long m_prefetchPTSNext;
      long long m_startPTSNext;
      long long m_timeReady; // unableToSplice error time particular window expired
      long long m_timeExpired;// unableToSplice error time when Placement is ready
      int m_detailCode;
      int m_detailCodeNext;
      RBIInsertSource *m_insertSrc;
      RBIInsertSource *m_insertSrcNext;

      int m_insertDataCapacity;
      int m_insertDataSize;
      unsigned char *m_insertData;

      int m_privateDataPacketCount;

      FILE *m_inFile;
      int m_inDataCapacity;
      unsigned char *m_inData;
      bool m_triggerDetectedForOpportunity;

      RBIContext();
     ~RBIContext();

      bool init();
      void term();

      void acquire();
      int release();

      void updateInsertionState(long long startPTS);
      bool triggerDetected( RBIStreamProcessor *sp, RBITrigger* trigger );
      bool getTriggerDetectedForOpportunity(void);
      void setTriggerDetectedForOpportunity(bool bTriggerDetectedForOpportunity);
      void insertionStarted( RBIStreamProcessor *sp );
      void insertionStatusUpdate( RBIStreamProcessor *sp, int detailCode );
      void insertionStopped( RBIStreamProcessor *sp );
      void insertionError( RBIStreamProcessor *sp, int detailCode );
      void privateData( RBIStreamProcessor *sp, int streamType, unsigned char *packet, int len );
      void sourceError( RBIInsertSource *src );
      void httpTimeoutError( RBIInsertSource *src );
      void tsFormatError( RBIInsertSource *src );
      void termInsertionSpot();
      void termInsertionAll();
      void sendTuneInEvent();
      void sendTuneOutEvent();
      void sendInsertionOpportunity( RBITrigger *trigger );
      void sendInsertionStatusEvent( int event );
      
   friend class RBIManager;   
};

typedef enum _RBIResult
{
   RBIResult_ok= 0,
   RBIResult_noMemory,
   RBIResult_notFound,
   RBIResult_spliceTimeout,
   RBIResult_alreadyExists
} RBIResult;

typedef struct _XREActivity
{
   std::string activityState;
   std::string lastActivityTime;
}XREActivity;

typedef std::list<ReceiverActivity> ReceiverList;

// Map < Key, list>
// Map < ocap_locator, list of ReceiverList>
typedef std::map<std::string, ReceiverList> SrcUriMap;

// Map < Key, string>
// Map < receiverid, Activity>
typedef std::map<std::string, XREActivity> ReceiverIdMap;

class RBIManager
{
   private:
      static RBIManager *m_instance;
      static pthread_mutex_t m_mutex;

   public:
      static RBIManager* getInstance();

      RBIContext* getContext();
      void releaseContext( RBIContext *ctx );
      
      int getSessionNumber( std::string sessionId );

      void setPrivateDataMonitor( RBIPrivateDataMonitor mon );
      
      int registerInsertOpportunity( std::string sessionId, long long utcTrigger, long long utcSplice, long long splicePTS );
      void unregisterInsertOpportunity( std::string sessionId );
      bool findInsertOpportunity( std::string sessionId, long long& utcTrigger, long long& utcSplice, long long& splicePTS );
      int ageInsertOpportunities( std::string sessionId );

      int registerInsertDefinitionSet(std::string sessionId, RBIInsertDefinitionSet defnSet );
      void unregisterInsertDefinitionSet(std::string sessionId);
      RBIInsertDefinitionSet* getInsertDefinitionSet( std::string sessionId );
      void releaseInsertDefinitionSet( std::string sessionId );
      
      void releaseInsertSource( RBIInsertSource *is);
      
      void getDeviceId( unsigned char deviceId[6] );
      void setTimeZoneName( );
      int getTimeZoneMinutesWest()
      {
         return m_timeZoneMinutesWest;
      }
      const char* getTimeZoneName()
      {
         return m_timeZoneName;
      }
      int getTriggerDefinitionTimeout()
      {
         return m_definitionRequestTimeout;
      }
      int getBufferSize()
      {
         return m_bufferSize;
      }
      int getRetryCount()
      {
         return m_retryCount;
      }
      bool getH264Enabled()
      {
         return m_H264Enabled;
      }
      bool getCaptureEnabled()
      {
         return m_captureEnabled;
      }
      int getSpliceOffset()
      {
         return m_spliceOffset;
      }
      bool getAudioReplicationEnabled()
      {
         return m_audioReplicationEnabled;
      }

      bool isProgrammerEnablementEnabled()
      {
         return m_progammerEnablementEnabled;
      }

      bool isSTTAcquired(void)
      {
         return m_STTAcquired;
      }

      void setSourceTunerStatus( const char *receiverId, const char *tunerActivityStatus, const char *lastActivityTime, const char *lastActivityAction );
      void addReceiverId(const char *uri, const char *receiverId, int isLiveSrc);
      void removeReceiverId(const char *uri, const char *receiverId, int isLiveSrc);
      void getTunerStatus(std::string sourceUri, ReceiverActivity recActivity[], size_t *totalReceivers);
      void printTunerActivityStatusMap(void);
      const char* curlUserAgent(void);

   private:
      RBIManager();
     ~RBIManager();
     
      typedef struct _InsertSourceInfo
      {
         bool cached;
         std::string uri;
         long long splicePTS;
      } InsertSourceInfo;
      
      typedef struct _PendingOpportunity
      {
         long long utcTrigger;
         long long utcSplice;
         long long splicePTS;
      } PendingOpportunity;
      
      std::map<std::string,PendingOpportunity> m_pendingOpportunities;
      std::map<std::string,RBIInsertDefinitionSet> m_insertDefinitions;
            
      int m_definitionRequestTimeout;
      bool m_H264Enabled;
      bool m_captureEnabled;
      bool m_audioReplicationEnabled;
      int m_spliceOffset;
      int m_spliceTimeout;
      int m_marginOfError;
      int m_bufferSize;
      int m_retryCount;
      bool m_progammerEnablementEnabled;
      RBIPrivateDataMonitor m_mon;

      bool m_haveDeviceId;
      unsigned char m_deviceId[6];
      
      int m_timeZoneMinutesWest;
      char m_timeZoneName[RBI_MAX_TIMEZONE_NAME_LEN+1];
      bool m_STTAcquired;
      int m_nextSessionNumber;
      std::map<std::string,RBIContext*> m_sessionIdMap;
      // Map < Key, list>
      // Map < ocap_locator, list of receiver_id>
      SrcUriMap m_srcUriMap;
      // Map < Key, string>
      // Map < receiverid, Status>
      ReceiverIdMap m_ReceiverIdMap;
      std::string mUserAgent;
};

#endif

