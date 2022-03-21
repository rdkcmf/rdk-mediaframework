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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef USE_SYSRES_MLT
#include "sysResource.h"
#endif
#include "rmf_osal_thread.h"
#include "rmf_osal_event.h"
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

#ifdef USE_VODSRC  
  #include "rmfvodsource.h"
#endif

  #ifdef TEST_WITH_PODMGR
    #include "rmf_pod.h"
  #endif
  
#endif

#if defined(ENABLE_CLOSEDCAPTION)
#include "closedcaption.h"
#endif

#include <glib.h>
// #include <string.h>
// #include <stdlib.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <sys/time.h>
#include "rmf_osal_init.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include "rdk_debug.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef USE_SOC_INIT
void soc_uninit();
void soc_init(int , char *, int );
#endif

#if 0
#ifdef USE_FRONTPANEL
//#include "manager.hpp"
#include "libIBus.h"
//#include "frontpanel.hpp"
#endif
#endif

#ifdef USE_DVR
  #include "dvrmanager.h"
#endif

#include "init.h"
#include "pipeline.h"

#define __DEBUG__

int pipeline::numofPipelines = 0;
int pipeline::mplayerInUse=0;
GMainLoop* gMainLoop = 0;

typedef struct process_table
{
  int processId;
  gchar processName[255];
  pipeline *process;
  EventHandler *events;
  bool playingProcess;
} process_table_t;

typedef struct thread_param
{
  pipeline process;
  launch_param_t params;
  rmf_osal_event_params_t eventParams;
   std::string commandName;
  int threadStarted;
  
} thread_param_t;

process_table_t *processList;

class EventHandler : public IRMFMediaEvents
{
private:
  static gboolean delayedDeletePipeline(pipeline *p)
  {
	  p->destroyPipeline();
	  delete p;
	  return FALSE;
  }
  void deletePipeline()
  {
    for(int i=0;i<MAX_PIPELINES;i++)
    {
      if(c_pipeline == processList[i].process)
      {
	pipeline::numofPipelines--;
	if (processList[i].playingProcess)
	{
          g_idle_add ((GSourceFunc)delayedDeletePipeline, c_pipeline);
	}
	processList[i].process=0;
	break;
      }
    }
    
  }
public:
    void complete()
    {
      deletePipeline();
    }
    void error(RMFResult err, const char* msg)
    {
      (void)err;
      g_print("error: %s\n", msg);
      deletePipeline();
    }
    EventHandler(pipeline *ptr)
    {
      c_pipeline = ptr;
    }
    pipeline *c_pipeline;
};


rmf_osal_eventqueue_handle_t rmfApp_eventqueue_handle;
static rmf_osal_eventmanager_handle_t eventmanager_handle =  0;
rmf_osal_ThreadId g_main_thId;
bool gInitialized = false;
extern int printAll(void);
extern int processLaunchCommand(std::vector<std::string>& words,launch_param_t &param);
#ifdef USE_DVR
extern int deleteRecording(std::string recordingId);
extern bool checkRecordingId(std::string recordingId);
extern int listDVR(int withDetails);
#endif
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

void shutDown()
{
   if ( gInitialized )
   {
      for(int i=0;i<pipeline::numofPipelines;i++)
      {
	if(processList[i].process)
	{
	  processList[i].process->destroyPipeline();
	  //processList[i].process->reset();
	  delete processList[i].process;
	}
      }
      gInitialized= false;
      if ( gMainLoop )
      {
         g_main_loop_quit( gMainLoop );
         gMainLoop= 0;
      }
   }
#if defined(ENABLE_CLOSEDCAPTION)
    ClosedCaptions::instance().stop();//before calling shutdown ensure cc is stopped first
    ClosedCaptions::instance().shutdown();
#endif
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
      g_print (" -----perform a graceful cleanup ------\n");
      if (gInitialized)
      {
	  g_print("-------------Calling shutDown()\n");
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

void gchar_freefn(void *pData)
{
    if(pData)
    {
#ifdef __DEBUG__
      //g_print("Freeing up data pData\n");
#endif
      g_free(pData);
    }
}

void handleKeys(gchar *str)
{
  rmf_Error  ret;
  rmf_osal_event_handle_t event_handle;
  rmf_osal_event_params_t event_params = {0};
  std::vector<std::string> words;
  
//   size_t nargs = args.size();
//   if (nargs == 0) // empty command
//     return;
// 
//   const std::string& cmd = args[0];

  event_params.data = (void *) str;
  splitString(g_strchomp(str), words, " ");
  if (words.size() == 0 )
  {
      gchar_freefn(str);
      return;
  }

  if ((words[0] == "quit") || words[0] == "q")
  {
      event_params.priority  = 1;
  }
  else
  {
      event_params.priority = 0;
  }
  event_params.data_extension = 0;
  event_params.free_payload_fn = gchar_freefn;
  //g_print("------------- DEBG1\nrmfApp->");
  ret = rmf_osal_event_create(RMF_OSAL_EVENT_CATEGORY_INPUT ,KEY_AVAILABLE, &event_params, &event_handle );
  if (RMF_SUCCESS != ret)
  {
      RDK_LOG(RDK_LOG_FATAL, "LOG.RDK.TEST", "%s: event create failed - %d \n", __FUNCTION__, ret);
      return;
  }
  //g_print("------------- DEBG2\nrmfApp->");
  ret = rmf_osal_eventmanager_dispatch_event( eventmanager_handle , event_handle );
  if (RMF_SUCCESS != ret)
  {
      RDK_LOG(RDK_LOG_FATAL, "LOG.RDK.TEST", "%s: event dispatch failed - %d \n", __FUNCTION__, ret);
      rmf_osal_event_delete( event_handle);
  }
  //g_print("------------- DEBG3\nrmfApp->");
}

static void print_error (const GError *error)
{
	if (error)
	{
		g_print ("ERROR: %s\n", error->message);
	}
}

static void mainThread( void * arg )
{
  // wait for Key
  
  rmf_Error ret;
  uint32_t event_type;
  rmf_osal_event_handle_t event_handle;
  rmf_osal_event_params_t eventParams;
  rmf_osal_event_category_t event_category;
  launch_param_t launchParams;
  thread_param_t *threadParams;
  rmf_Error rmf_error;
  rmf_osal_ThreadId pipe_thId;
  process_table_t *tmp;
  pipeline *activePipeline;
  std::string stringName;
  pipeline *process;
  int playingProcess=-1;
  bool NotTerminated = 1;
  EventHandler *events;

  launchParams._tsb=0;
  launchParams.duration = 0;
  launchParams.numOfURls=0;
/*
  processList = new process_table_t[MAX_PIPELINES];
  for(int i;i<MAX_PIPELINES;i++)
  {
    processList[i].processId = -1;
    strcpy(processList[i].processName,"");
    processList[i].process=0;
    processList[i].playingProcess=false;
  }
  */

  if((processList = new(std::nothrow) process_table_t[MAX_PIPELINES]) == NULL) 	
	{
     g_print("Unable to Alloc Memory for processList");
     return;
   	}

   for(int i=0;i<MAX_PIPELINES;i++)
   {
     processList[i].processId = -1;
     strcpy(processList[i].processName,"");
     processList[i].process=0;
     processList[i].playingProcess=false;
   }

  while(NotTerminated)
  {

    ret = rmf_osal_eventqueue_get_next_event( rmfApp_eventqueue_handle, &event_handle,&event_category, &event_type, &eventParams);
    if (ret)
    {
      g_print( "rmf_osal_eventqueue_get_next_event failed");
      continue;
    }
    if (event_type == KEY_AVAILABLE )
    {
      std::vector<std::string> words;
      splitString(g_strchomp((gchar*)eventParams.data), words, " ");
    
       size_t nargs = words.size();
      if (nargs == 0) // empty command
        continue;
      //g_print("Received following command\nrmfApp->");
      
      if((words[0] == "help") || words[0] == "?")
      {
	printAll();
      }
      else if ((words[0] == "quit") || words[0] == "q")
      {
	g_print("------- terminating --------\n");

	shutDown();
	NotTerminated = 0;
	}
	else if (words[0] == "script") 
	{
		g_print("------- script --------\n");
		if (words.size() == 1 )
		{
			g_print("Error : script name not provided\n");
		}
		else
		{
			int loopcount  =1;
			if (words.size() > 2 )
			{
				loopcount = atoi(words[2].c_str());
			}
			const char* filename = words[1].c_str();
			g_print("script name %s loopcount %d\n", filename, loopcount);
			int fd = open(filename, O_RDONLY);
			if (fd == -1)
			{
				perror("Error in opening script file \n");
			}
			else
			{
				GError* error = NULL;
				GIOChannel* channel = g_io_channel_unix_new (fd);
				if (NULL != channel )
				{
					for (int i = 0; i < loopcount; i++ )
					{
						gchar* str = NULL;
						gsize str_len = 0;
						gsize terminator_pos = 0;
						do
						{
							GIOStatus rc = g_io_channel_read_line(channel, &str, &str_len, &terminator_pos, &error);
							print_error(error);
							if (rc == G_IO_STATUS_EOF)
							{
								break;
							}
							if (str_len > 0 )
							{
								handleKeys(str);
							}
						}
						while (1);
						g_io_channel_seek_position( channel, 0, G_SEEK_SET, &error);
						print_error(error);
					}
				}
				else
				{
					g_print("Error in g_io_channel_unix_new");
					close (fd);
					break;
				}
				g_io_channel_shutdown (channel, FALSE, &error );
				print_error(error);
				close (fd);
			}
		}
	}
	else if (words[0] == "sleep")
	{
		g_print("------- sleep --------\n");
		if (words.size() == 1 )
		{
			g_print("Error : sleep time not provided\n");
		}
		else
		{
			int sleeptime = atoi(words[1].c_str());
			g_print("Sleeping for %d seconds\n", sleeptime);
			sleep (sleeptime);
		}
	}
	else if (words[0] == "avcheck")
	{
		g_print("------- avcheck --------\n");
		system ("HOMIE.sh 0 0");
	}
      else if(words[0] == "launch" || words[0] == "la" || words[0] == "play" || words[0] == "record")
      {
	//g_print("rmfApp-> launch command not implemented yet");
	if (processLaunchCommand(words,launchParams))
	{
	  g_print(" ----------- launch params are ---------------\nrmfApp->");
	  g_print("    source:   %s\nrmfApp->", launchParams.source.c_str());
	  g_print("    sink:   %s\nrmfApp->", launchParams.sink.c_str());
#ifdef USE_DVR
	  if (launchParams.sink == "dvrsink")
	  {
	    if(!checkRecordingId(launchParams.recordingId))
	    {
	      g_print("    recordingTitle:   %s\nrmfApp->", (launchParams.recordingTitle ? launchParams.recordingTitle : "Delia test recording"));
	      g_print("    recordingId:   %s\nrmfApp->", launchParams.recordingId.c_str());
	    }
	    else
	    {
	      g_print("This recordingId is already present.. please either delete or use different Id\n");
	      continue;
	    }
	  }
	  else if (launchParams._tsb)
	  {
	    g_print(" --- TSB is enabled and duration =  %d\nrmfApp->", launchParams.duration);
	  }
#endif
	  g_print(" ------- urls--------\nrmfApp->");
	  for (int i=0;i<launchParams.numOfURls;i++)
	    g_print("url[%d] = %s \nrmfApp->",i,launchParams.urls[i].c_str());
	  g_print(" -------------------------------------------------------\nrmfApp->");
	  
	  //get curren pipeline status
	  if(pipeline::numofPipelines > MAX_PIPELINES )
	  {
	    g_print("Maximum allowed pipelines are %d.. please kill some process before trying. For help type help or ? \nrmfApp->",MAX_PIPELINES);
	    continue;
	  }
	  if(launchParams.sink == MEDIAPLAYER_SINK )
	  {
	    if (pipeline::mplayerInUse)
	    {
	      processList[playingProcess].process->destroyPipeline();
	      delete processList[playingProcess].events;
	      delete processList[playingProcess].process;
	      processList[playingProcess].process=0;
	      pipeline::numofPipelines--;
	      playingProcess=-1;
	      //g_print("Only single instance of mediaplayersink can be run.. its already in use.. please kill the process and try\nrmfApp->");
	      //continue;
	    }
	  }
	  if (pipeline::numofPipelines < MAX_PIPELINES)
	  {
	    process = new pipeline;
	    if(process->init(&launchParams))
	    {
	      events = new EventHandler(process);
	      process->c_pSource->addEventHandler(events);
        process->events=events;
	      g_print("Events are set \n");
	      // Start playback.
	      g_print("started to playback \n");
	      if ( RMF_RESULT_SUCCESS != process->c_pSource->play() )
	      {
		  g_print( "[%s:%d] Source->play failed\n", __FUNCTION__, __LINE__ );
		  process->reset();
		  delete events;
		  delete process;
		  continue;
	      }
	      //process->pipelineActive=true;
	      if(launchParams.sink == MEDIAPLAYER_SINK)
		pipeline::mplayerInUse = 1;
	      for(int i=0; i < MAX_PIPELINES;i++)
	      {
		  if (!processList[i].process)
		  {
		    g_print("--------- process %d entered------------\nrmfApp->",pipeline::numofPipelines);
		    processList[i].process = process;
		    //events = new EventHandler(&processList[i]);
		    processList[i].events = events;
		    memset(processList[i].processName, 0, sizeof(processList[i].processName));
		    strncpy(processList[i].processName,(const char*)eventParams.data, sizeof(processList[i].processName)-1);
		    if(launchParams.sink == MEDIAPLAYER_SINK )
		    {
		      playingProcess = i;
		      processList[i].playingProcess = true;
		    }
		    pipeline::numofPipelines++;
		    
		    break;
		  } //end of if (!processList[i].process)
	      } // end of for(int i=0;--
	    } //end of if(process->init(&launchParams)) 
	    else
	    {
		g_print("Failed to process request ..\nrmfApp->");
		delete process;
	    }
	  } // end of  if (pipeline::numofPipelines < MAX_PIPELINES)
	  else
	  {
	    g_print(" ---- More than %d pipelines are not supp ored. Kill/destroy some pipelines using kill command and try\nrmfApp->", MAX_PIPELINES);
	  }

	} //end of if(processLaunchCommand(words,launchParams))
      }
      else if((words[0] == "listrecordings") || (words[0] == "list")  || (words[0] == "ls")  )
      {
#ifdef USE_DVR
	if ((words.size()>1) && ((words[1] == "detail" || words[1] == "full" || words[1] == "details")))
	  listDVR(1);
	else
	  listDVR(0);
#endif
      }
      else if((words[0] == "ps"))
      {
	  g_print("List of active pipelines running..  \nrmfApp->"); 
	  g_print("------------------------------\nrmfApp->");
	  for(int i=0;i<MAX_PIPELINES;i++)
	  {
	    if(processList[i].process)
	    {
	      g_print("    %d       `%s`\nrmfApp->",i+1,processList[i].processName);
	    }
	  }
      }
      else if((words[0] == "kill") || words[0] == "k")
      {
	if(words.size() > 1)
	{
	  int processNum=atoi(words[1].c_str());
	  if ( processNum > MAX_PIPELINES || !processList[processNum-1].process)
	  {
 	    g_print("Invalid process number passed\nrmfApp->");
	    continue;
	  }
	  processList[processNum-1].process->destroyPipeline();
	  g_print("------ pipeline destroyed ---------\nrmfApp->");
	  g_print("------  destroyed process %d's event ---------\nrmfApp->",processNum);	  
	  delete processList[processNum-1].events;
	  g_print("------  destroyed process %d ---------\nrmfApp->",processNum);	  
	  delete processList[processNum-1].process;
	  
	  processList[processNum-1].process=0;
	  if (processList[processNum-1].playingProcess)
	  {
	    processList[processNum-1].playingProcess = false;
	    playingProcess = -1;
	  }
	  pipeline::numofPipelines--;
	  g_print("Process #%d is killed\nrmfApp->",processNum);
	}
	else
	  g_print("Noting to be killed (no process given) \nrmfApp->");
	
      }
#if defined(ENABLE_CLOSEDCAPTION)
	else if(words[0] == "enable" || words[0] == "e")
	{
	    if(playingProcess >= 0 && playingProcess < MAX_PIPELINES &&
            processList[playingProcess].process)
            {
             g_print("control %s being passed to playing process\nrmfApp->");
             processList[playingProcess].process->startcCaption();
           }
	}
	else if(words[0] == "disable" || words[0] == "d")
	{
	  if(playingProcess >= 0 && playingProcess < MAX_PIPELINES &&
            processList[playingProcess].process)
          {
            g_print("control %s being passed to playing process\nrmfApp->");
            processList[playingProcess].process->stopcCaption();
          }
	}
	else if(words[0] == "show" || words[0] == "s")
	{
	    if(playingProcess >= 0 && playingProcess < MAX_PIPELINES &&
            processList[playingProcess].process)
            {
              g_print("control %s being passed to playing process\nrmfApp->");
              ClosedCaptions::instance().setVisible(true); 
            }
	}
	else if(words[0] == "hide" || words[0] == "h")
	{
	    if(playingProcess >= 0 && playingProcess < MAX_PIPELINES &&
            processList[playingProcess].process)
            {
              g_print("control %s being passed to playing process\nrmfApp->");
              ClosedCaptions::instance().setVisible(false); 
            }
	}
#endif
      else if(  words[0] == "<"
		|| words[0] == ">"
		|| words[0] == "p"
		|| words[0] == "s"
		|| words[0] == "e"
		|| words[0] == "b"
		|| words[0] == "f"
		|| words[0] == "r"
		|| words[0] == "l"
		|| words[0] == "a"
	      )
      {
	if(playingProcess >= 0 && playingProcess < MAX_PIPELINES &&
	    processList[playingProcess].process)
	{
        if ((words.size()>1))
        {
            processList[playingProcess].process->handleCommand(words[0].c_str()[0], words[1].c_str());
        } else {
            processList[playingProcess].process->handleCommand(words[0].c_str()[0], NULL);
        }
	}
      }
      else if((words[0] == "delete") || words[0] == "rm")
      {
#ifdef USE_DVR
	if ((words.size()>1))
	{
	  if( deleteRecording( words[1].c_str()) )
	    g_print("deleting id = %s failed .. please make sure id is correct. For more info please type help\nrmfApp->");
	}
	else
	  g_print("-id is missing. For more info please type help\nrmfApp->");
#endif
      }
      else
      {
	g_print("rmfApp-> unknown command.. type help to see available commands");
      }
      g_print("\nrmfApp->");
    } //end of if (event_type == KEY_AVAILABLE )
  } // end of while(NotTerminated)

} // end of mainThread()

void start_loop( GMainLoop* loop)
{
   gMainLoop = loop;
   //gMainLoop= 1;
   g_main_loop_run(loop);

#ifdef USE_SOC_INIT
   soc_uninit();
#endif
}

int rmfApp_init (void)
{
  rmf_Error ret;
  rmf_Error rmf_error;
  
  setupExit();
  // Initialize the threading library.
  g_thread_init(NULL);

#ifdef USE_SOC_INIT
  soc_init(1, "rmf_app", 1);
#endif
  if ( rmf_osal_init( "/etc/rmfconfig.ini", "/etc/debug.ini" ) != RMF_SUCCESS )
  {
      /* Unhandled error */
      g_print("Error: rmf_osal_init failed");
  }
  return 1;
}

int rmfApp_start(void)
{
  rmf_Error ret;
  rmf_Error rmf_error;

#if 0
  #ifdef USE_FRONTPANEL
      // Initialize IARM to receive IARM bus events 
      IARM_Bus_Init("rmfAppClient");
      IARM_Bus_Connect();
//      device::Manager::Initialize();
      // Initialize front panel
  #endif
#endif
  g_print("starting RMF App --------------------\n");

  ret = rmf_osal_eventqueue_create( (const uint8_t*)"RMFApp_keys", &rmfApp_eventqueue_handle);
  if (ret)
    g_print("rmf_osal_eventQueue_create failed");

  ret = rmf_osal_eventmanager_register_handler(
		  eventmanager_handle,
		  rmfApp_eventqueue_handle,
		  RMF_OSAL_EVENT_CATEGORY_INPUT);

  rmf_error = rmf_osal_threadCreate(mainThread, NULL, RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE, &g_main_thId, "mainThread");
  if ( RMF_SUCCESS != rmf_error )
  {
      g_print( "%s - rmf_osal_threadCreate failed.\n", __FUNCTION__);
  }

  g_print(" RMFApp started --------------------\n");
  return 1;
}

int rmfApp_close(void)
{
#if 0
#ifdef USE_FRONTPANEL  
  // De-Initialize IARM
//  device::Manager::DeInitialize();
  IARM_Bus_Disconnect();
  IARM_Bus_Term();
#endif
#endif
    return 1;
}
