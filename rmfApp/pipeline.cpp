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
//#include "rmfbase.h"
#include <stdio.h>
#include <stdlib.h>

//#ifdef USE_SYSRES_MLT
//#include "sysResource.h"
//#else
#include "string.h"
//#endif
#include "hnsource.h"

#ifdef USE_MEDIAPLAYERSINK
  #include "mediaplayersink.h"
#endif

#ifdef USE_DVR
  #include "DVRSource.h"
  #include "DVRSink.h"
#endif
#ifdef USE_QAMSRC
  #include "rmfqamsrc.h"
  #include "rmf_platform.h"

  #ifdef TEST_WITH_PODMGR
    #include "rmf_pod.h"
  #endif
  
#endif
#include <unistd.h>
#ifdef USE_VODSRC
  #include "rmfvodsource.h"
#endif
#ifdef USE_DTVSRC
  #include "rmfdtvsource.h"
#endif
#if defined(ENABLE_CLOSEDCAPTION)
#include "closedcaption.h"
#endif
#include <glib.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <sys/time.h>
#include "rmf_osal_init.h"
#include <iostream>
#include <fstream>
#include <sstream>

#ifdef USE_DVR
 #include "dvrmanager.h"
#endif

#if 0
#include "manager.hpp"
#include "libIBus.h"
#include "logger.hpp"
#include "frontpanel.hpp"
#endif

#include "init.h"
#include "pipeline.h"
#include <string.h>
#if USE_SYSRES_MLT
#include "rpl_malloc.h"
#include "rpl_new.h"
#endif


using namespace std;

int pipeline::podInitialized=0;

int pipeline::initQAM()
{
  FILE *fp;
  //bool stt_flag;
#ifdef USE_QAMSRC
  if(!pipeline::podInitialized)
  {
#ifdef QAMSRC_FACTORY
    /*To support sinks which require prerolling - mediaplayersink*/
    RMFQAMSrc::disableCaching();
#endif
#ifdef TEST_WITH_PODMGR
    if (!pipeline::podInitialized)
    {

      g_print("-------- Need to initialize pod for the first time ------------\n");
      RMFQAMSrc::init_platform();
#ifdef USE_VODSRC
	RMFVODSrc::init_platform();
#endif      
//    #ifdef USE_FRONTPANEL
    #if 0
      // Initialize front panel
      fp_init();
      fp_setupBootSeq();
    #endif  

      while (1)
      {
	fp = fopen("/tmp/stt_received","r");
	if (fp )
	{
	  fclose(fp);
	  break;
	}
	else
	{
	  sleep(1);
	}
      }
      //sleep(30);
      pipeline::podInitialized=1;
    }
#endif
  }
#endif

  return 1;

}


RMFMediaSourceBase* pipeline::createSource()
{
#ifdef USE_HNSRC
    if( c_source == HN_SOURCE)
    {
        return new HNSource();
    }
#endif    
#ifdef USE_DVR
    if( c_source == DVR_SOURCE)
    {
        return new DVRSource();
    }
#endif
#ifdef USE_QAMSRC
    if( c_source == QAM_SOURCE)
    {
	pipeline::initQAM();
        return new RMFQAMSrc();
    }
#endif
#ifdef USE_VODSRC
    if( c_source == VOD_SOURCE)
    {
	pipeline::initQAM();
	return new RMFVODSrc();
    }
#endif
#ifdef USE_DTVSRC
    if( c_source == DTV_SOURCE)
    {
        return RMFDTVSource::getDTVSourceInstance(c_url.c_str());
    }
#endif
    {
        return 0;
    }
}

RMFMediaSinkBase* pipeline::createSink()
{
#ifdef USE_MEDIAPLAYERSINK
    if( c_sink == MEDIAPLAYER_SINK)
    {
        MediaPlayerSink* pSink = new MediaPlayerSink();
        pSink->init();
        pSink->setVideoRectangle(0, 0, 1280, 720);
        if ( pSink->setSource(c_pSource) != RMF_RESULT_SUCCESS)
        {
            /* Unhandled error */
            g_print("Error: setSource failed\n");
        }
	c_pMediaPlayerSink = pSink;
        return pSink;
    }
#endif
#ifdef USE_DVR
    if ( c_sink == DVR_SINK)
    {
        g_print("createSink: recordingId=%s\n", c_recordingId.c_str() );
        DVRSink* pSink = new DVRSink(c_recordingId);
        pSink->init();
        pSink->setSource(c_pSource);
        return pSink;
    }
#endif
    return 0;
}


long long pipeline::getCurrentTime()
{
   struct timeval tv;
   long long currentTime;
   
   gettimeofday( &tv, 0 );
   
   currentTime= (((unsigned long long)tv.tv_sec) * 1000 + ((unsigned long long)tv.tv_usec) / 1000);
   
   return currentTime;
}
#ifdef USE_MEDIAPLAYERSINK
#ifdef USE_DVR
bool pipeline::switchToLive( MediaPlayerSink* &pMediaPlayerSink )
{
   RMFResult res;
   
   if ( c_useBufferedOnly )
   {
      RMFMediaInfo info;
      c_PlaySource->play();
      pMediaPlayerSink->setMuted(false);
      c_PlaySource->setSpeed( 1.0 );
      if ( c_PlaySource->getMediaInfo(info) == RMF_RESULT_SUCCESS )
      {                    
         c_PlaySource->setMediaTime(info.m_duration);
      }
   }
   else
   {
      //c_PlaySource->setEvents(NULL);
      c_PlaySource->pause();
      c_PlaySource->close();
      
      c_PlaySink->setSource( NULL );
      c_PlaySink->term();
      delete c_PlaySink;
      c_PlaySink= 0;
      
      c_PlaySource->term();
      delete c_PlaySource;
      c_PlaySource= 0;
      
      pMediaPlayerSink = new MediaPlayerSink();
      if ( pMediaPlayerSink == 0 )
      {
         g_print("Error: unable to create MediaPlayerSink\n");
         return false;
      }
      pMediaPlayerSink->init();
      pMediaPlayerSink->setVideoRectangle(0, 0, 1280, 720);
      c_PlaySink= pMediaPlayerSink;
      
      res= c_PlaySink->setSource( c_pSource );

      c_isLive= true;
   }
   
   return true;   
}

bool pipeline::switchToTSB( MediaPlayerSink* &pMediaPlayerSink, const char *dvrLocator, bool start )
{
    //g_print("@ -------------- %s : %d\n",__FUNCTION__,__LINE__);
    //g_print("gplaysink = %p",c_PlaySink);
   c_PlaySink->setSource( NULL );
   //g_print("@ -------------- %s : %d\n",__FUNCTION__,__LINE__);
   c_PlaySink->term();
   //g_print("@ -------------- %s : %d\n",__FUNCTION__,__LINE__);
   delete c_PlaySink;
   //g_print("@ -------------- %s : %d\n",__FUNCTION__,__LINE__);
   c_PlaySink= 0;
   //g_print("@ -------------- %s : %d\n",__FUNCTION__,__LINE__);
   
   pMediaPlayerSink = new MediaPlayerSink();
   if ( pMediaPlayerSink == 0 )
   {
      g_print("Error: unable to create MediaPlayerSink\n");
      return false;
   }
   //g_print("@ -------------- %s : %d\n",__FUNCTION__,__LINE__);
   
   pMediaPlayerSink->init();
   pMediaPlayerSink->setVideoRectangle(0, 0, 1280, 720);
   c_PlaySink= pMediaPlayerSink;
   
   //g_print("@ -------------- %s : %d\n",__FUNCTION__,__LINE__);
   
   c_PlaySource= new DVRSource();
   c_PlaySource->init();
   c_PlaySource->open( dvrLocator, 0 );
   c_PlaySink->setSource( c_PlaySource );
   if ( start )
   {
      c_PlaySource->play();
   }
   else
   {
      c_PlaySource->pause();
   }

   //gPlaySource->setEvents(g_playBackEvents);

   c_isLive= false;
   
   return true;   
}
#endif
#endif

bool pipeline::processRequest()
{
  RMFMediaSourceBase* pSource;
  RMFMediaSinkBase* pSink;
  char *dvrLocator= 0;
  bool result= true;
  RMFResult rc;
  
  // Instantiate source and sink.
  c_pSink=0;
  c_pSource=0;
  c_RecordSink=0;
  pSource = createSource();
  g_print(" >>>>>>>>>>>>>>>>>>>>>>>RMFResult rc;>>>>>>>>. source created\nrmfApp->");
  do
  {
    if(pSource == 0)
    {
	g_print("rmfApp: source '%s' is invalid.  Try rmfApp -help for more info.\n", c_source.c_str());
	result = false;
	break;
    }
    c_pSource= pSource;

    rc = pSource->init();
    if ( RMF_RESULT_SUCCESS != rc )
    {
	g_print("[%s:%d] Source->init failed\n",__FUNCTION__, __LINE__ );
	result = false;
	break;
    }
    g_print(" >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>. source init done \nrmfApp->");
    rc =pSource->open(c_url.c_str(), 0);
    if ( RMF_RESULT_SUCCESS != rc )
    {
	g_print("[%s:%d] Source->open failed\n",__FUNCTION__, __LINE__ );
	result = false;
	break;
    }
    g_print(" >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>. source opened \nrmfApp->");
#ifdef USE_DVR
    if(c_tsb)
    {
      // Get instance of DVRManager
      c_dvm= DVRManager::getInstance();
      if ( !c_dvm )
      {
	g_print("Error: unable to get DVRManager instance\n" );
	result= false;
	break;
      }
      // Create TSB with desired duration in seconds
      c_dvrres= DVRManager::getInstance()->createTSB(c_duration*60, c_tsbId );
      if ( c_dvrres != DVRResult_ok )
      {
	g_print( "Error: unable to create TSB (error %X)\n", c_dvrres );
	result= false;
	break;
      }
      g_print(">>>>>>>>>>>>>>> TSB created \nrmfApp->");
      /*replacing unsafe function*/
      snprintf( c_work, sizeof(c_work),"dvr://local/%s#0", c_tsbId.c_str() );
      dvrLocator= strdup( c_work );
      c_dvrLocator = dvrLocator;
      
      // Create DVR sink for TSB
      c_RecordSink= new DVRSink( c_tsbId );
      if ( c_RecordSink == 0 )
      {
	g_print("Error: unable to create DVRSink for TSB\n");
	result= false;
	break;
      }
      c_RecordSink->init();
      c_RecordSink->setSource(c_pSource);
    }

    if( c_sink == DVR_SINK)
    {
	char properties[256];
	/*replacing unsafe function*/
	snprintf( properties, sizeof(properties),"{\"title\":\"%s\"}", (c_recordingTitle ? c_recordingTitle : "Delia test recording") );
	RecordingSpec spec;
	spec.setRecordingId(c_recordingId);
	spec.setStartTime( getCurrentTime() );
	spec.setDuration(c_duration*60*1000);
	spec.setDeletePriority("P3");
	spec.setBitRate( RecordingBitRate_high );
	spec.setProperties( properties ); 
	spec.addLocator( c_url );
	int result= DVRManager::getInstance()->createRecording( spec );
	if ( result != DVRResult_ok )
	{
	    g_print( "rmfApp: unable to create recording for id %s (error %X)\n", c_recordingId.c_str(), result );
	    return 0;
	}
    }
#endif
    pSink = createSink();
    if(c_sink == MEDIAPLAYER_SINK)
    {
        std::size_t aud_opt_pos;

        aud_opt_pos = c_url.find("&audio=", 0);
        if (aud_opt_pos != std::string::npos)
        {
            std::string aud_opt;
            std::size_t aud_opt_end_pos;

            aud_opt_pos += strlen("&audio=");
            aud_opt_end_pos = c_url.find("&", aud_opt_pos);
            aud_opt = c_url.substr(aud_opt_pos, (aud_opt_end_pos != std::string::npos) ? aud_opt_end_pos - aud_opt_pos : std::string::npos);
            c_pMediaPlayerSink->setAudioLanguage(aud_opt.c_str());
        }
    }

    if(pSink == 0)
    {
	g_print("rmfApp: sink '%s' is invalid.  Try rmfApp -help for more info.\n", c_sink.c_str());
	delete pSource;
	result = false;
    }
    c_pSink= pSink;
    c_PlaySink= c_pSink;
  }while (0);
  #if defined(ENABLE_CLOSEDCAPTION)
  startcCaption();
  #endif

  return result;
}

  #if defined(ENABLE_CLOSEDCAPTION)
void pipeline::startcCaption()
{
  unsigned long videoDecoderHndl;
  
  if(c_pMediaPlayerSink != NULL)
  {
  videoDecoderHndl = c_pMediaPlayerSink->getVideoDecoderHandle();
  g_print("[play] ccStart");
  ClosedCaptions::instance().start((void*)videoDecoderHndl);
  }
}

void pipeline::stopcCaption()
{
  ClosedCaptions::instance().stop();
}
#endif
bool g_inTrickMode=false;
#ifdef USE_MEDIAPLAYERSINK
int pipeline::dvrKeys(int cmd )
{
  g_print(" trick cmd = %c\n",cmd);
  switch(cmd)
  {
    case 'p':
    case 'P':
    {
      float speed;
      double time;
      c_pSource->getSpeed( speed );
      if ( c_isPaused || (speed > 1.0) || (speed < -1.0))
      {
        c_pSource->pause();
	      c_pMediaPlayerSink->getMediaTime(time);
        c_pMediaPlayerSink->setMuted(false);
        speed= 1.0f;
        printf("\nsetting speed to %f time %f\n", speed, time );
        c_pSource->play( speed, time );
        c_pMediaPlayerSink->onSpeedChange( speed );
        c_isPaused= false;
        printf("\nresuming\n" );
      }
      else
      {
        c_pSource->pause();
        c_isPaused= true;
        printf("\npausing\n" );
      }
    }
    break;
    case 's':
    case 'S':
      {
        c_pSource->setMediaTime(0.0);
        printf("\nskipping back to the beginning\n" );
      }
      break;
    case 'e':
    case 'E':
    {
      RMFMediaInfo info;
      if ( c_pSource->getMediaInfo(info) == RMF_RESULT_SUCCESS )
      {                    
        c_pSource->setMediaTime(info.m_startTime+info.m_duration);
        printf("\nskipping to end of recording\n" );
      }
    }
    break;
    case 'b':
    case 'B':
    {               
      double time;

      c_pSource->getMediaTime(time);
      c_pSource->setMediaTime(time-SKIPBACK_AMOUNT);
      printf("\nskipping back %f seconds\n", SKIPBACK_AMOUNT );
    }
    break;
    case 'f':
    case 'F':
    {
      float speed;
      double time;
      c_pSource->getSpeed( speed );
      c_pMediaPlayerSink->setMuted(true);
      if (c_isPaused )
      {
        if ( speed == 1.0 )
        {
          c_isPaused= false;
          speed= 0.5;
        }
      }
      else
      {
        if ( speed < 4.0 )
        {
          speed= 4.0;
        }
        else if ( speed < 15.0 )
        {
          speed= 15.0;
        }
        else if ( speed < 30.0 )
        {
          speed= 30.0;
        }
        else if ( speed < 60.0 )
        {
          speed= 60.0;
        }
        else
        {
          speed= 4.0;
        }
      }
      c_pMediaPlayerSink->getMediaTime(time);
      printf("\nsetting speed to %f time %f\n", speed, time );
      c_pSource->play( speed, time );
      c_pMediaPlayerSink->onSpeedChange( speed );
    }
    break;
    case 'r':
    case 'R':
    {
      float speed;
      double time;
      c_pSource->getSpeed( speed );
      c_pMediaPlayerSink->setMuted(true);
      if ( c_isPaused )
      {
        if ( speed == 1.0 )
        {
          c_isPaused= false;
          speed= -0.5;
        }
      }
      else
      {
        if ( speed > -4.0 )
        {
          speed= -4.0;
        }
        else if ( speed > -15.0 )
        {
          speed= -15.0;
        }
        else if ( speed > -30.0 )
        {
          speed= -30.0;
        }
        else if ( speed > -60.0 )
        {
          speed= -60.0;
        }
        else
        {
          speed= -4.0;
        }
      }
      c_pMediaPlayerSink->getMediaTime(time);
      printf("\nsetting speed to %f time %f\n", speed, time );
      c_pSource->play( speed, time );
      c_pMediaPlayerSink->onSpeedChange( speed );
    }
    break;
  default :
    break;
  }
  return 1;
}
#endif
//static void handleCommand(const std::vector<std::string>& args)
void pipeline::handleCommand(int cmd, const char* cmd_option)
{
#ifdef USE_DVR
#ifdef USE_MEDIAPLAYERSINK
//g_print("---------------VENU: got command = %c\n",cmd);
    if (c_sink == MEDIAPLAYER_SINK && c_source == DVR_SOURCE)
      dvrKeys(cmd);
    else
#endif
#endif
    {
    switch(cmd)
    {
      case 'a':
        if (cmd_option)
        {
            c_pMediaPlayerSink->setAudioLanguage(cmd_option);
        } else {
            g_print("Audio lanugages:%s\n", c_pMediaPlayerSink->getAudioLanguages());
        }
        break;
      case '>':
      {
	//Go to next channel;
	if((c_currentChannel <= c_numOfChannels) && !c_channels[c_currentChannel+1].empty())
	{
	  c_currentChannel++;
	  g_print("Executing next url = %s\n",c_channels[c_currentChannel].c_str());
	  destroyPipeline();
	  c_url = c_channels[c_currentChannel].c_str();
	  if (!processRequest())
	  {
	    g_print("Failed to processRequest\n");
	    if(c_pSink)
	    {
	      delete c_pSink;
	      c_pSink=0;
	    }
	    if(c_pSource)
	    {
	      delete c_pSource;
	      c_pSource=0;
	    }
	    //destroyPipeline();
	  }
	  else
	  {
	      // Start playback.
	      g_print("started to playback \n");
	      c_pSource->play();
	      g_print("playback is called\n");
	  }
	}
	else
	{
	  g_print("End of list reached, no more urls available.. try revers way\n");
	}
	break;
      }
      case '<' :
      {
	//Go to next channel;
	if(c_currentChannel && !c_channels[c_currentChannel-1].empty())
	{
	  g_print("Tuning to previous url = %s",c_channels[c_currentChannel].c_str());
	  destroyPipeline();
	  c_currentChannel--;
	  c_url = c_channels[c_currentChannel].c_str();
	  if (!processRequest())
	  {
	    g_print("Failed to processRequest\n");
	    if(c_pSink)
	    {
	      delete c_pSink;
	      c_pSink=0;
	    }
	    if(c_pSource)
	    {
	      delete c_pSource;
	      c_pSource=0;
	    }
	    //destroyPipeline();
	  }
	  else{
	      // Start playback.
	      g_print("started to playback \n");
	      c_pSource->play();
	      g_print("playback is called\n");
	  }
	}
	else
	{
	  g_print("End of list reached, no more urls available.. try revers way\n");
	}
	break;

      }
#ifdef USE_MEDIAPLAYERSINK

      case 'p':
      case 'P':
        g_print("-----------Venu: I am here c_tsb= %d and c_isLive = %d\n",c_tsb,c_isLive);
    #ifdef USE_DVR
    if ( c_tsb)
    {
      if ( c_isLive )
      {
        g_print("-----------Venu: Switching to TSB \n");
        switchToTSB( c_pMediaPlayerSink, c_dvrLocator, false );
        RMFMediaInfo info;
        if ( c_PlaySource->getMediaInfo(info) == RMF_RESULT_SUCCESS )
        {
          // Save current live point.  We will resume from this
          // point in the TSB when we unpause
          c_resumePoint= info.m_duration;
        }
        // A problem with the transition from live to pause is that the pipeline for
        // playback from TSB attempts to preroll which sometimes causes the onscreen
        // frame to update when what we want is for it to remain on the last live still.
        c_isPaused= true;
        g_print("\npausing\n" );
      }
      else
      {
        float speed;
        g_print("@ -------------- %s : %d\n",__FUNCTION__,__LINE__);
        c_PlaySource->getSpeed( speed );
        g_print("@ -------------- %s : %d\n",__FUNCTION__,__LINE__);
        if ( c_isPaused || (speed > 1.0) || (speed < -1.0))
        {
          g_print("@ -------------- %s : %d\n",__FUNCTION__,__LINE__);
          //c_PlaySource->play();
          c_pMediaPlayerSink->setMuted(false);
          g_print("@ -------------- %s : %d\n",__FUNCTION__,__LINE__);
          //c_PlaySource->setSpeed( 1.0 );
	  speed = 1.0f;
          c_PlaySource->play(speed, c_resumePoint);
          if ( c_resumePoint != 0.0 )
          {
            g_print("@ -------------- %s : %d\n",__FUNCTION__,__LINE__);
            // Set playback position to the resume point. This will make
            // TSB playback continue from the same point live was paused.
            //c_PlaySource->setMediaTime(c_resumePoint);
            c_resumePoint= 0.0;
          }
          c_isPaused= false;
          g_print("\nresuming\n" );
        }
        else
        {
          c_PlaySource->pause();
          c_isPaused= true;
          g_print("\npausing\n" );
        }
      }
    }
    else
    #endif
    {
      if (c_source == HN_SOURCE)
      {
        
        RMFState cur_state;
        int state;
        state = c_pSource->getState(&cur_state, NULL);
        g_print("Failed to get state: %d and return = %d\n", (int) cur_state,(int)state);
        if ( state != RMF_STATE_CHANGE_SUCCESS)
          g_print("Failed to get state: %d\n", (int) cur_state);

        if (cur_state != RMF_STATE_PLAYING)
        {
          c_pSource->play();
          c_pSource->setSpeed(1.f);
          g_inTrickMode=false;
        }
        else
        {
          //c_pSource->setSpeed(0.0); // switch from fast forward/rewind to normal playback
          if (g_inTrickMode)
          {
            c_pSource->play();
            c_pSource->setSpeed(1.f);
            g_inTrickMode=false;
          }
          else
            c_pSource->pause();
        }
      }
      else
        g_print(" ......... TSB is not enabled is your request. use -tsb to enable tsb\nrmfApp->");
    }

	break;
	case 's':
	case 'S':
	{
	  #ifdef USE_DVR
	  if(c_tsb)
	  {
	    if ( c_isLive )
	    {
	      switchToTSB( c_pMediaPlayerSink, c_dvrLocator, true );                  
	    }
	    c_PlaySource->setMediaTime(0.0);
	    g_print("\nskipping back to the beginning\n" );
	  }
	  else
	  #endif
	  {
	    g_print(" ......... TSB is not enabled is your request. use -tsb to enable tsb\nrmfApp->");
	  }
	}
	break;
	case 'e':
	case 'E':
	#ifdef USE_DVR
	if (c_tsb)
	{
	  if ( !c_isLive )               
	  {
	    RMFMediaInfo info;
	    if ( c_PlaySource->getMediaInfo(info) == RMF_RESULT_SUCCESS )
	    {                    
	      c_PlaySource->setMediaTime(info.m_duration);
	      g_print("\nskipping to end of recording\n" );
	    }
	  }
	}
	else
	#endif
	{
	  g_print(" ......... TSB is not enabled is your request. use -tsb to enable tsb\nrmfApp->");
	}

	break;
	case 'b':
	case 'B':
	{
	  #ifdef USE_DVR
	  if (c_tsb)
	  {
	    double time;

	    if ( c_isLive )
	    {
		switchToTSB( c_pMediaPlayerSink, c_dvrLocator, true );                  
		RMFMediaInfo info;
		if ( c_PlaySource->getMediaInfo(info) == RMF_RESULT_SUCCESS )
		{                    
		  c_PlaySource->setMediaTime(info.m_duration-SKIPBACK_AMOUNT);
		}
	    }
	    else
	    {
		c_PlaySource->getMediaTime(time);
		c_PlaySource->setMediaTime(time-SKIPBACK_AMOUNT);
	    }
		g_print("\nskipping back %f seconds\n", SKIPBACK_AMOUNT );
	  }
	  else
	  #endif
	  {
	    g_print(" ......... TSB is not enabled is your request. use -tsb to enable tsb\nrmfApp->");
	  }

	  break;
	}
	case 'f':
	case 'F':
	  #ifdef USE_DVR
	  if(c_tsb)
	  {
	    if ( !c_isLive )
	    {
	      float speed;
	      c_PlaySource->getSpeed( speed );
	      c_pMediaPlayerSink->setMuted(true);
	      if ( c_isPaused )
	      {
		  if ( speed == 1.0 )
		  {
		    c_PlaySource->play();
		    c_isPaused= false;
		    speed= 0.5;
		  }
	      }
	      else
	      {
		  if ( speed < 4.0 )
		  {
		    speed= 4.0;
		  }
		  else if ( speed < 15.0 )
		  {
		    speed= 15.0;
		  }
		  else if ( speed < 30.0 )
		  {
		    speed= 30.0;
		  }
		  else if ( speed < 60.0 )
		  {
		    speed= 60.0;
		  }
		  else
		  {
		    speed= 4.0;
		  }
	      }
	      g_print("\nsetting speed to %f\n", speed );
	      c_PlaySource->setSpeed( speed );
	    }
	  }
	  else
	  #endif
	  {
	    g_print ("*-------------- ) Venu : Great...... I am here\n");
	    if (c_source == HN_SOURCE)
	    {
        float speed;
        g_inTrickMode=true;
        c_pMediaPlayerSink->setMuted(true);
        c_pSource->getSpeed( speed );
        g_print ("*-------------- ) Venu : current speed = %f\n",speed);
        //c_pMediaPlayerSink->setMuted(true);
        if ( speed < 4.0 )
        {
          speed= 4.0;
        }
        else if ( speed < 15.0 )
        {
          speed= 15.0;
        }
        else if ( speed < 30.0 )
        {
          speed= 30.0;
        }
        else if ( speed < 60.0 )
        {
          speed= 60.0;
        }
        else
        {
          speed= 4.0;
        }
        g_print(" ......... setting speed to %f\n",speed);
        c_pSource->setSpeed(speed);
      }
      else
        g_print(" ......... TSB is not enabled is your request. use -tsb to enable tsb\nrmfApp->");
    }
    break;
	  case 'r':
	  case 'R':
	  {
	  #ifdef USE_DVR
	    if(c_tsb)
	    {
	      float speed;
	      if ( c_isLive )
	      {
		  switchToTSB( c_pMediaPlayerSink, c_dvrLocator, true );                  
		  RMFMediaInfo info;
		  if ( c_PlaySource->getMediaInfo(info) == RMF_RESULT_SUCCESS )
		  {                    
		    c_PlaySource->setMediaTime(info.m_duration-SKIPBACK_AMOUNT);
		  }
	      }
	      usleep( 200000 );
	      c_PlaySource->getSpeed( speed );
	      if ( !c_isPaused )
	      {
		  c_pMediaPlayerSink->setMuted(true);
		  if ( speed > -4.0 )
		  {
		    speed= -4.0;
		  }
		  else if ( speed > -15.0 )
		  {
		    speed= -15.0;
		  }
		  else if ( speed > -30.0 )
		  {
		    speed= -30.0;
		  }
		  else if ( speed > -60.0 )
		  {
		    speed= -60.0;
		  }
		  else
		  {
		    speed= -4.0;
		  }
	      }
	      g_print("\nsetting speed to %f\n", speed );
	      c_PlaySource->setSpeed( speed );
	    }
	    else
	  #endif
	    {
	      if (c_source == HN_SOURCE)
	      {
          g_inTrickMode=true;
          float speed;
          c_pSource->getSpeed( speed );
          g_print("current speed = %f\n", speed);
          c_pMediaPlayerSink->setMuted(true);
          if ( speed == 1.0 )
          {
            speed = -4.0;
          }
          else if ( (-speed) < 4.0 )
          {
            speed = -4.0;
          }
          else if ( (-speed) < 15.0 )
          {
            speed= -15.0;
          }
          else if ( (-speed) < 30.0 )
          {
            speed= -30.0;
          }
          else if ( (-speed) < 60.0 )
          {
            speed= -60.0;
          }
          else
          {
            speed= -4.0;
          }
           g_print(" ......... setting speed to %f\n",speed);
          c_pSource->setSpeed(speed);
	      }
	      else
          g_print(" ......... TSB is not enabled is your request. use -tsb to enable tsb\nrmfApp->");
	    }
	  }
	  break;
#ifdef USE_DVR      
	case 'l':
	case 'L':
	if(c_tsb)
	{
	  if ( !c_isLive )
	  {
	      switchToLive( c_pMediaPlayerSink );
	  }
	}
	else
	  g_print(" ......... TSB is not enabled is your request. use -tsb to enable tsb\nrmfApp->");
	break;
#endif    
#endif
	default:
	{
	  g_print("Unknown command: [%c]\n", cmd);
	}
      }
    }
}


int pipeline::destroyPipeline()
{
   if ( c_Initialized )
   {
      c_tsbId.clear();
      if ( c_pSource )
      {
        g_print(" ------ pausing source ---------\nrmfApp->");
        c_pSource->removeEventHandler( events );
        c_pSource->pause();
        c_pSource->close();
      }
      if ( c_PlaySource )
      {
        g_print(" ------ pausing PlaySource ---------\nrmfApp->"); 
        //c_PlaySource->setEvents(NULL);
        c_PlaySource->pause();
        c_PlaySource->close();
        delete c_PlaySource;
        c_PlaySource=0;
      }
#ifdef USE_DVR
      if ( c_dvrLocator )
      {
        free( c_dvrLocator );
        c_dvrLocator=0;
      }
      if ( c_RecordSink )
      {
        g_print(" ------ pausing record sink ---------\nrmfApp->"); 
        c_RecordSink->term();
        delete c_RecordSink;
        c_RecordSink= 0;
      }
#endif
      if ( c_PlaySink )
      {
        g_print(" ------ pausing Play sink ---------\nrmfApp->"); 
        if(c_sink == MEDIAPLAYER_SINK)
        pipeline::mplayerInUse=0;
        //c_PlaySink->setEvents( NULL );
         c_PlaySink->term();
//         delete c_PlaySink;
//         c_PlaySink= 0;
      }
      if ( c_pSource )
      {
        g_print(" ------ terminating source ---------\nrmfApp->");
        c_pSource->term();
        delete c_pSource;
        c_pSource= 0;
      }

      if ( c_PlaySink ) { 
        delete c_PlaySink;
        c_PlaySink = 0;
      }
        
      c_isLive = true;
      //c_Initialized= false;
#if defined(ENABLE_CLOSEDCAPTION)
   stopcCaption();
#endif
   }
   return 1;
}

int pipeline::init(launch_param_t *params)
{
  c_source = params->source;
  c_sink = params->sink;
#ifdef USE_DVR
  c_recordingId = params->recordingId;
  c_recordingTitle = params->recordingTitle;
  c_tsb  = params->_tsb;
  c_duration  = params->duration;
#endif
  for(int i=0;i<params->numOfURls;i++)
    c_channels[i]= params->urls[i];
  c_numOfChannels=params->numOfURls;
  c_url = params->urls[0];

  if (!processRequest())
  {
    g_print("Failed to processRequest\n");
    if(c_pSink)
    {
      delete c_pSink;
      c_pSink=0;
    }
    if(c_pSource)
    {
      delete c_pSource;
      c_pSource=0;
    }
    //destroyPipeline();
    return 0;
  }
  return 1;
}

pipeline::pipeline()
{
  c_Initialized=true;
  c_pSource= 0;
  c_pSink= 0;
#ifdef USE_DVR
  c_recordingId= std::string();
  c_recordingTitle= 0;
  c_RecordSink= 0;
  c_isLive= true;
  c_dvrLocator= 0;
  c_useBufferedOnly= false;
  c_tsb=0;
  c_tsbId= std::string();
  c_dvm= 0;
  c_dvrres=0;
  c_work[256]=0;
#endif
  c_numOfChannels=0;
  c_PlaySource= 0;
  c_PlaySink= 0;
  c_resumePoint= 0.0;
  c_isPaused= false;
  c_goLive= false;
#ifdef USE_MEDIAPLAYERSINK
  c_pMediaPlayerSink=0;
#endif
  c_currentChannel=0;

}

void pipeline::reset(void)
{
  //c_Initialized=true;
  c_pSource= 0;
  c_pSink= 0;
#ifdef USE_DVR
  c_recordingId= std::string();
  c_recordingTitle= 0;
  c_tsb=0;
  c_tsbId= std::string();
  c_dvm= 0;
  c_dvrres=0;
  c_work[256]=0;
  c_RecordSink= 0;
  c_isLive= true;
  c_dvrLocator= 0;
  c_useBufferedOnly= false;
#endif
  c_numOfChannels=0;
  c_PlaySource= 0;
  c_PlaySink= 0;
  c_resumePoint= 0.0;
  c_isPaused= false;
  c_goLive= false;
#ifdef USE_MEDIAPLAYERSINK
  c_pMediaPlayerSink=0;
#endif
  c_currentChannel=0;
}
pipeline::~pipeline()
{
   if ( c_Initialized )
   {
#ifdef USE_DVR
      c_tsbId.clear();
      if ( c_dvrLocator )
      {
	free( c_dvrLocator );
	c_dvrLocator=0;
      }
      if ( c_RecordSink )
      {
         delete c_RecordSink;
         c_RecordSink= 0;
      }
#endif
      if ( c_PlaySink )
      {
         delete c_PlaySink;
         c_PlaySink= 0;
      }
      if ( c_PlaySource )
      {
         delete c_PlaySource;
         c_PlaySource= 0;
      }
      if ( c_pSource )
      {
	delete c_pSource;
	c_pSource= 0;
      }
      c_Initialized= false;
   }
}

int pipeline::run(void )
{
    return 1;
}
