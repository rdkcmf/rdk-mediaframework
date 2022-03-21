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
#ifdef USE_SYSRES_MLT
#include "sysResource.h"
#endif
#include <glib.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <sys/time.h>
#include <iostream>
#include <fstream>
#include <sstream>
#ifdef USE_HNSOURCE
#include "hnsource.h"
#endif 

#ifdef USE_MEDIAPLAYERSINK
  #include "mediaplayersink.h"
#endif

#ifdef USE_DVR
  #include "DVRSource.h"
  #include "dvrmanager.h"
#endif

#include "init.h"
#include "pipeline.h"
#include "rmf_platform.h"
#include "rmf_osal_init.h"

using namespace std;

extern void shutDown(void);
void handleKeys(gchar *str);

gboolean stdinCallback( GIOChannel *source, GIOCondition condition, gpointer data)
{
    gchar* str = NULL;
    gsize str_len = 0;
    gsize terminator_pos = 0;
    GError* error = NULL;
    //g_print("------------- waiting for key press.............\nrmfApp->");
    GIOStatus rc = g_io_channel_read_line(source, &str, &str_len, &terminator_pos, &error);
    if (rc == G_IO_STATUS_EOF)
    {
        shutDown();
        return FALSE;
    }
    g_print("rmfApp->");
    handleKeys(str);
    return TRUE;
}

int processLaunchCommand(std::vector<std::string>& words,launch_param_t &param)
{
  
    int idx;
    int argc;
    std::string source;
    std::string sink;
    std::string url;
    int _tsb=0;
    std::string recordingId;
    char *recordingTitle= 0;
    param.numOfURls=0;
    int duration=0;
    std::string channelListFile;
    std::string channels[50];
    ifstream channelFile;
    
    channelListFile.clear();
    argc = words.size();
    g_print("argc= %d\n",argc);
    if(argc == 1)
    {
      g_print(" .. no param provided.. please type help for more options ..\nrmfApp->");
      return false;
    }
    param.duration = 0;
    idx = 1;
    for(int i = 1; i < argc; ++i)
    {
      //g_print("param = %d\n",i);
      if(std::string(words[i]) == "-source")
      {
	if(i < argc-1 && words[i+1][0] != '-')
	    source = words[++i];
	//verify the source matches one of our source names
	if(source != HN_SOURCE  && source != DVR_SOURCE  && source != QAM_SOURCE && source != VOD_SOURCE && source != DTV_SOURCE)
	{
	  g_print("invalid source option %s.  help for more info \nrmfApp->", source.c_str());
	  return false;
	}
	if(i+1 > idx)
	  idx = i+1;
      } //end of if(std::string(words[i]) == "-source")
      else if (std::string(words[i]) == "-sink")
      {
	if(i < argc-1 && words[i+1][0] != '-')
	  sink = words[++i];
	//verify the sink matches one of our sink names
	if(sink != MEDIAPLAYER_SINK && sink != DVR_SINK)
	{
	  g_print("invalid sink option '%s'.  Try rmfApp -help for more info.\nrmfApp->", sink.c_str());
	  return false;
	}
	if(i+1 > idx)
	  idx = i+1;
      }
      else if(std::string(words[i]) == "-recordingId" || words[i] == "-id" )
      {
	if(i < argc-1 && words[i+1][0] != '-')
	  recordingId= words[++i].c_str();
	if(i+1 > idx)
	  idx = i+1;
      }
      else if(std::string(words[i]) == "-recordingTitle" || words[i] == "-title")
      {
	if(i < argc-1 && words[i+1][0] != '-')
	  recordingTitle= (char *)words[++i].c_str();
	if(i+1 > idx)
	  idx = i+1;
      }
      else if(std::string(words[i]) == "-file")
      {
	if(i < argc-1 && words[i+1][0] != '-')
	  channelListFile= (char *)words[++i].c_str();
	if(i+1 > idx)
	  idx = i+1;
      }
      else if(std::string(words[i]) == "-live")
      {
	source = QAM_SOURCE;
	sink = MEDIAPLAYER_SINK;
	if(i+1 > idx)
	  idx = i+1;
      } //end of if(std::string(words[i]) == "-source")
      else if(std::string(words[i]) == "-tsb")
      {
	//source = QAM_SOURCE;
	sink = MEDIAPLAYER_SINK;
	_tsb=1;
	if(i+1 > idx)
	  idx = i+1;
      } //end of if(std::string(words[i]) == "-source")
      else if (words[i] == "-duration")
      {
	  if(i < argc-1 && words[i+1][0] != '-')
	  {
	    param.duration =atoi( words[++i].c_str());
	    if (param.duration <= 0)
	      param.duration = DEFAULT_RECORD_DURATION;
	  }
 	  else
 	    g_print("duration value is %d missing assuming 30 mins\nrmfApp->",param.duration);
	  if(i+1 > idx)
	    idx = i+1;
       }
      else
      {
	if(words[i][0] == '-')
	{
	  g_print("unrecognized option '%s'.  Try rmfApp -help for more info.\nrmfApp->", words[i].c_str());
	  return false;
	} //end of if
      } //end of else
    } //end of for(int i = 0; i < argc; ++i)

    if(idx == argc-1)
    {
	g_print("words[idx] = %s\n",words[idx].c_str());
        param.urls[0] = words[idx];
	url=words[idx];
	param.numOfURls = 1;
    }

    if (url.empty() && channelListFile.empty())
    {
        g_print("url parameter is missing.  Try rmfApp -help for more info.\nrmfApp->");
        return false;
    }

    if (!channelListFile.empty())
    {
      int j=0;
      channelFile.open(channelListFile.c_str());
      
      if (channelFile.is_open())
      {
	while ( channelFile.good() )
	{
	  getline (channelFile,param.urls[j++]);
	  //cout << param.urls[j-1] << endl;
	}
	channelFile.close();
	url=param.urls[0];
	//url="0x236A";
	param.numOfURls = j-1;
      }
      else
      {
	g_print("file %s  can not be opened.  Try rmfApp -help for more info.\nrmfApp->",channelListFile.c_str());
	return false;
      }
      if (!param.numOfURls || !param.urls[0].length())
      {
	  g_print("file %s  doesnt contain desired urls.  Try rmfApp -help for more info.\nrmfApp->",channelListFile.c_str());
	  return false;
      }
      
    }
//     if (param.source)
    g_print("url = %s\nrmfApp->",url.c_str());
    if( url.find("http://") != std::string::npos)
      param.source = HN_SOURCE;
    else if (url.find("ocap://") != std::string::npos)
    {
      g_print("its a qam source \nrmfApp->");
      param.source = QAM_SOURCE;
    }
    else if (url.find("tune://") != std::string::npos)
    {
      g_print("its a qam tune source \nrmfApp->");
      param.source = QAM_SOURCE;
    }
    else if (url.find("dvr://") != std::string::npos)
      param.source = DVR_SOURCE;
    else if (url.find("vod://") != std::string::npos)
      param.source = VOD_SOURCE;
    else if (url.find("dtv://") != std::string::npos)
    {
      g_print("its a DTV source \nrmfApp->");
      param.source = DTV_SOURCE;
    }
    else
    {
      g_print("Invalid url.. can not be used. Type help to see valid options. \nrmfApp->");
      return false;
    }
    param.urls[0] = url;
    //g_print("url:    %s\nrmfApp->", url.c_str());
//     g_print(" --------- provided options are ----------\nrmfApp->");
//     g_print("    source:   %s\nrmfApp->", source.c_str());
//     g_print("    sink:   %s\nrmfApp->", sink.c_str());
    if (words[0] == "play")
      sink = MEDIAPLAYER_SINK;
    else if (words[0] == "record")
      sink = DVR_SINK;
    
    if (!source.empty())
    {
//       if(param.source == HN_SOURCE && _tsb)
//       {
// 	g_print("TSB is not support on hnsource.. its ignored\nrmfApp->");
// 	_tsb=0;
// 	source=HN_SOURCE;
//       }
//       else
	param.source = source;
    }
    if (!sink.empty())
      param.sink = sink;

    param._tsb=_tsb;
    
    if (sink == "dvrsink")
    {
      if (!recordingId.size() || !recordingTitle )
      {
	g_print(" recording Id or title missing .. usage: record -id <ID> -title <Titel> <url>  \nrmfApp->");
	return false;
      }
      if (!param.duration)
	param.duration = DEFAULT_RECORD_DURATION;
      param.recordingId = recordingId;
      param.recordingTitle = recordingTitle;

      g_print("    recordingTitle:   %s\nrmfApp->", recordingTitle );
      g_print("    recordingId:   %s\nrmfApp->", recordingId.c_str());
      g_print("    duration (in minutes):   %ld\nrmfApp->", param.duration);
    }
    else if (_tsb)
    {
	if (!param.duration)
	  param.duration = TSB_DURATION;
      g_print(" --- TSB is enabled and duration(in minutes) =  %d\nrmfApp->", param.duration);
      //param._tsb=_tsb;
    }
    g_print(" ---------------------------------------\nrmfApp->");
    return true;

}
