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

#undef BUILD_DVRSOURCE
#undef BUILD_DVRSINK 
#undef BUILD_QAMSOURCE
#include "hnsource.h"
#include "mediaplayersink.h"
#ifdef BUILD_DVRSOURCE
#include "DVRSource.h"
#endif
#ifdef BUILD_DVRSINK
#include "dvrmanager.h"
#include "DVRSink.h"
#endif
#ifdef BUILD_QAMSOURCE
#include "rmfqamsrc.h"
#endif
#include <glib.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <sys/time.h>

#ifdef RMF_STREAMER // remove streamer-dependent code
#include "rmf_osal_init.h"
#endif

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

static int gInitialized = false;
static RMFMediaSourceBase* g_pSource= 0;
//static RMFMediaSinkBase* g_pSink= 0;
static MediaPlayerSink* g_pSink= 0;
static GMainLoop* gMainLoop= 0;
static long long recordingId= 0;
static char *recordingTitle= 0;

static long long getCurrentTime()
{
   struct timeval tv;
   long long currentTime;

   gettimeofday( &tv, 0 );

   currentTime= (((unsigned long long)tv.tv_sec) * 1000 + ((unsigned long long)tv.tv_usec) / 1000);

   return currentTime;
}

//When adding a new source or sink you need to:
//0) Add the #include for your source/sink header above
//1) Add the #define to the list of defines below for the name as it should appear on command line
//2) Update createSource/createSink to do the actually allocation of the source/sink
//3) Update processCommandLine to check for the new source/sink name
//4) Update the Makefile and include your player.mk

#define HN_SOURCE           "hnsource"
#define MEDIAPLAYER_SINK    "mediaplayersink"
#define DVR_SOURCE          "dvrsource"
#define DVR_SINK            "dvrsink"
#define QAM_SOURCE          "qamsource"

void printUsage();

RMFMediaSourceBase* createSource(const std::string& source, const std::string& url)
{
    if( source == HN_SOURCE)
    {
        return new HNSource();
    }
#ifdef BUILD_DVRSOURCE
    else
    if( source == DVR_SOURCE)
    {
        return new DVRSource();
    }
#endif
#ifdef BUILD_QAMSOURCE
    else
    if( source == QAM_SOURCE)
    {
        return new RMFQAMSrc();
    }
#endif
    else
    {
        return 0;
    }
}

RMFMediaSinkBase* createSink(std::string& sink, RMFMediaSourceBase* pSource)
{
    if( sink == MEDIAPLAYER_SINK)
    {
        MediaPlayerSink* pSink = new MediaPlayerSink();
        pSink->init();
        // disabled b/c we want full screen, which may be larger than 720p
        //pSink->setVideoRectangle(0, 0, 1280, 720);
        pSink->setSource(pSource);
        return pSink;
    }
#ifdef BUILD_DVRSINK
    else
    if( sink == DVR_SINK)
    {
        g_print("createSink: recordingId=%s\n", recordingId );
        DVRSink* pSink = new DVRSink(recordingId);
        pSink->init();
        pSink->setSource(pSource);
        return pSink;
    }
#endif
    else
    {
        return 0;
    }
}

bool processCommandLine(int argc, char** argv, std::string& source, std::string& sink, std::string& url)
{
    int idx;

    for(int i = 0; i < argc; ++i)
    {
        if(std::string(argv[i]) == "-help")
        {
            printUsage();
            return false;
        }
    }

    idx = 1;
    for(int i = 0; i < argc; ++i)
    {
        if(std::string(argv[i]) == "-source")
        {
            if(i < argc-1 && argv[i+1][0] != '-')
                source = argv[++i];
            //verify the source matches one of our source names
            if(source != HN_SOURCE
#ifdef BUILD_DVRSOURCE
            && source != DVR_SOURCE
#endif
#ifdef BUILD_QAMSOURCE
            && source != QAM_SOURCE
#endif
              )
            {
                g_print("rmfplayer: invalid source option '%s'.  Try rfmplayer -help for more info.\n", source.c_str());
                return false;
            }
            if(i+1 > idx)
                idx = i+1;
        }
        else
        if(std::string(argv[i]) == "-sink")
        {
            if(i < argc-1 && argv[i+1][0] != '-')
                sink = argv[++i];
            //verify the sink matches one of our sink names
            if(sink != MEDIAPLAYER_SINK
#ifdef BUILD_DVRSINK
            && sink != DVR_SINK
#endif
              )
            {
                g_print("rmfplayer: invalid sink option '%s'.  Try rfmplayer -help for more info.\n", sink.c_str());
                return false;
            }
            if(i+1 > idx)
                idx = i+1;
        }
#ifdef BUILD_DVRSINK
        else if(std::string(argv[i]) == "-recordingId")
        {
            if(i < argc-1 && argv[i+1][0] != '-')
               recordingId= ( argv[++i] );
            if(i+1 > idx)
                idx = i+1;
        }
        else if(std::string(argv[i]) == "-recordingTitle")
        {
            if(i < argc-1 && argv[i+1][0] != '-')
               recordingTitle= argv[++i];
            if(i+1 > idx)
                idx = i+1;
        }
#endif
        else
        if(argv[i][0] == '-')
        {
            g_print("rmfplayer: unrecognized option '%s'.  Try rfmplayer -help for more info.\n", argv[i]);
            return false;
        }
    }

    if(idx == argc-1)
    {
        url = argv[idx];
    }

    if (url.empty())
    {
        g_print("rmfplayer: url parameter is missing.  Try rfmplayer -help for more info.\n");
        return false;
    }

    if(source.empty())
    {
        g_print("source: unset -- default to %s\n", HN_SOURCE);
        source = HN_SOURCE;
    }
    else
    {
        g_print("source: %s\n", source.c_str());
    }

    if(sink.empty())
    {
        g_print("sink:   unset -- default to %s\n", MEDIAPLAYER_SINK);
        sink = MEDIAPLAYER_SINK;
    }
    else
    {
        g_print("sink:   %s\n", sink.c_str());
    }

    g_print("url:    %s\n", url.c_str());

    return true;
}

void printUsage()
{
    g_print("Usage: rmfplayer [-source SOURCE] [-sink SINK] [other options] url\n");
    g_print("Play a video stream located at url through a source and sink\n");
    g_print("The default source and sink is hnsource and mediaplayersink\n");
    g_print("SOURCE can be one of the following:\n");
    g_print("\thnsource\n");
#ifdef BUILD_DVRSOURCE
    g_print("\tdvrsource\n");
#endif
#ifdef BUILD_QAMSOURCE
    g_print("\tqamsource\n");
#endif
    g_print("SINK can be one of the following:\n");
    g_print("\tmediaplayersink\n");
#ifdef BUILD_DVRSINK
    g_print("\tdvrsink\n");
#endif
    g_print("\n\nOther options include:\n");
#ifdef BUILD_DVRSINK
    g_print("\t-recordingId <id> : id of recording to create where <id> is a decimal number\n");
    g_print("\t-recordingTitle <title> : title of recording\n");
#endif
}

static void shutDown();
class EventHandler : public IRMFMediaEvents
{
public:
    void complete()
    {
		g_print("EOS Received\n");
		float speed;
		g_pSource->getSpeed(speed);
		if(speed < 0)
			g_pSource->setMediaTime(0);
		else
		{
			shutDown();
		//g_pSource->term();
		//g_pSink->term();
		//delete g_pSource;
		//delete g_pSink;
        //exit(0);
		}
    }
    void error(RMFResult err, const char* msg)
    {
        (void)err;
        g_print("error: %s\n", msg);
        exit(1);
    }
};

static void shutDown()
{
   if ( gInitialized )
   {
      gInitialized= false;
      if ( g_pSource )
      {
         //g_pSource->pause();
         //g_pSource->close();
      }
      if ( g_pSink )
      {
         g_pSink->term();
      }
      if ( g_pSource )
      {
         g_pSource->term();
      }
      if ( g_pSink )
         delete g_pSink;
      if ( g_pSource )
         delete g_pSource;

      g_pSink= 0;
      g_pSource= 0;
      if ( gMainLoop )
      {
		 g_print("QUITTING the pipeline\n");
         g_main_loop_quit( gMainLoop );
         gMainLoop= 0;
      }
   }
}

static void sighandler(int signum)
{
    g_print("-----------------------------------------------------------\n");
    sleep(1); // allow other threads to print
    switch (signum)
    {
#define ONCASE(x) case x: g_print("Caught signal %s in %s:%d\n", #x, __FILE__,__LINE__); break
        ONCASE(SIGINT);     /* CTRL-C   */
        ONCASE(SIGQUIT);    /* CTRL-\   */
        ONCASE(SIGILL);
        ONCASE(SIGABRT);    /* abort()  */
        ONCASE(SIGFPE);
        ONCASE(SIGSEGV);
        ONCASE(SIGTERM);
        ONCASE(SIGPIPE);
#undef ONCASE
        default: g_print("Caught unknown signal %d in %s:%d\n", signum, __FILE__, __LINE__); break;
    }
    signal(signum, SIG_DFL);    // restore default handler

    // send the signal to the default signal handler, to allow a debugger
    // to trap it
    if (signum == SIGINT)
    {
       g_print("SIGINT - don't call kill\n");
       if ( gMainLoop )
       {
          g_main_loop_quit( gMainLoop );
          gMainLoop= 0;
       }
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
           g_print("Signal caught during shutDown()\n");
       }

       kill(getpid(), signum);     // invoke default handler
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

static int splitString(const std::string& text, std::vector<std::string>& words, const std::string& separators)
{
    size_t textLen = text.length();
    size_t start = text.find_first_not_of(separators, 0);
    while (start < textLen)
    {
        size_t stop = text.find_first_of(separators,start);

        if (stop > textLen)
            stop = textLen;

        words.push_back(text.substr(start, stop-start));
        start = text.find_first_not_of(separators, stop+1);
    }
    return (int)words.size();
}

static void handleCommand(const std::vector<std::string>& args)
{
    size_t nargs = args.size();

    if (nargs == 0) // empty command
        return;

    const std::string& cmd = args[0];

    if (cmd == "seek" || cmd == "s") // e.g. "seek 55" will seek to 55s
    {
        unsigned pos = nargs > 1 ? atoi(args[1].c_str()) : 0;
        g_pSource->setMediaTime(pos);
    }
    else if (cmd == "pause" || cmd == "u")
    {
        RMFState state;
        RMFStateChangeReturn rc = g_pSource->getState(&state, NULL);
        if (rc != RMF_STATE_CHANGE_SUCCESS)
            g_print("Failed to get state: %d\n", (int) state);
        else if (state == RMF_STATE_PLAYING)
            g_pSource->pause();
        else
            g_pSource->play();
    }
    else if (cmd == "play" || cmd == "p")
    {
        RMFState cur_state;
        if (g_pSource->getState(&cur_state, NULL) != RMF_STATE_CHANGE_SUCCESS)
            g_print("Failed to get state: %d\n", (int) cur_state);
        else if (cur_state != RMF_STATE_PLAYING)
            g_pSource->play();
        else
            g_pSource->setSpeed(1.f); // switch from fast forward/rewind to normal playback
    }
    else if (cmd == "ffwd" || cmd == "f")
    {
        float speed;
        g_pSource->getSpeed( speed );
        g_print ("current speed = %f\n",speed);
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
        g_pSource->setSpeed(speed);
    }
    else if (cmd == "rewind" || cmd == "rew" || cmd == "r")
    {
          float speed;
          g_pSource->getSpeed( speed );
          g_print("current speed = %f\n", speed);
          if ( speed >= 1.0 )
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
          g_pSource->setSpeed(speed);
    }
    else if (cmd == "position" || cmd == "pos" || cmd == "o")
    {
        double pos;
        g_pSource->getMediaTime(pos);
        g_print("Position: %f\n", pos);
    }
    else if (cmd == "lang" || cmd == "lan" || cmd == "l")
    {
        double pos;
		std::string lan = nargs > 1 ? (args[1].c_str()) : "eng";
        g_print("language = %s\n", lan.c_str());
        g_pSink->setAudioLanguage(lan.c_str());
    }
    else if (cmd == "quit" || cmd == "q")
    {
        shutDown();
    }
    else
    {
        g_print("Unknown command: [%s]\n", cmd.c_str());
    }
}

static gboolean stdinCallback(
    GIOChannel *source,
    GIOCondition condition,
    gpointer data)
{
    gchar* str = NULL;
    gsize str_len = 0;
    gsize terminator_pos = 0;
    GError* error = NULL;

    GIOStatus rc = g_io_channel_read_line(source, &str, &str_len, &terminator_pos, &error);
    if (rc == G_IO_STATUS_EOF)
    {
        shutDown();
        return FALSE;
    }

    std::vector<std::string> words;
    splitString(g_strchomp(str), words, " ");
    handleCommand(words);
    return TRUE;
}

int main(int argc, char** argv)
{
    std::string source;
    std::string sink;
    std::string url;
#ifdef RMF_STREAMER // remove streamer-dependent code
    rmf_osal_init( NULL, NULL );
#endif
    RMFMediaSourceBase* pSource;
    //RMFMediaSinkBase* pSink;
    MediaPlayerSink* pSink;

    if(!processCommandLine(argc, argv, source, sink, url))
        return 1;

    setupExit();

    // Instantiate source and sink.
    pSource = new HNSource();//createSource(source, url);

    if(pSource == 0)
    {
        g_print("rmfplayer: source '%s' is invalid.  Try rfmplayer -help for more info.\n", source.c_str());
        return 0;
    }
    g_pSource= pSource;

    pSource->init();

    pSource->open(url.c_str(), 0);

#ifdef BUILD_DVRSINK
    if( sink == DVR_SINK)
    {
        char properties[256];
        snprintf( properties, sizeof(properties), "{\"title\":\"%s\"}", (recordingTitle ? recordingTitle : "Delia test recording") );
        RecordingSpec spec;
        spec.setRecordingId(recordingId);
        spec.setStartTime( getCurrentTime() );
        spec.setDuration(30*60*1000);
        spec.setDeletePriority("P3");
        spec.setBitRate( RecordingBitRate_high );
        spec.setProperties( properties );
        spec.addLocator( url );
        int result= DVRManager::getInstance()->createRecording( spec );
        if ( result != DVRResult_ok )
        {
           printf( "rmfplayer: unable to create recording for id %s (error %X)\n", recordingId, result );
           return 0;
        }
    }
#endif
    //pSink = createSink(sink, pSource);
        pSink = new MediaPlayerSink();
        pSink->init();
        pSink->setSource(pSource);

    if(pSink == 0)
    {
        g_print("rmfplayer: sink '%s' is invalid.  Try rfmplayer -help for more info.\n", sink.c_str());
        delete pSource;
        return 0;
    }
    g_pSink= pSink;

    // Start watching stdin for commands.
    GMainLoop* loop = g_main_loop_new(NULL, FALSE);
    gMainLoop = loop;
    GIOChannel* stdin_ch = g_io_channel_unix_new(fileno(stdin));
    g_io_add_watch(stdin_ch, G_IO_IN, stdinCallback, NULL);

    // Start playback.
    EventHandler events;
    pSource->setEvents(&events);
    pSource->play();

    // Execute typed commands until playback is complete.
    g_main_loop_run(loop);

    g_io_channel_unref(stdin_ch);
    g_main_loop_unref(loop);

    return 0;
}
