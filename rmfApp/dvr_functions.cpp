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

#include "dvrmanager.h"

#define __DEBUG__

using namespace std;

#define printf g_print

#ifdef USE_DVR

bool checkRecordingId(std::string recordingId)
{
    DVRManager *dvm= DVRManager::getInstance();
    bool found = false;
   int count= dvm->getRecordingCount();
   for( int ii= 0; ii < count; ++ii )
   {
      RecordingInfo *pRecInfo= dvm->getRecordingInfoByIndex( ii );
      if(recordingId == pRecInfo->recordingId)
      {
	found = true;
	break;
      }
   }
   return found;
}
int listDVR(int withDetails)
{
   DVRManager *dvm= DVRManager::getInstance();
   //g_print("dvm=%p\n", dvm );
   
   struct timeval tm;
   gettimeofday( &tm, 0 );
   //g_print("currentTime: %llu\n", ((long long)tm.tv_sec)*1000 );

   long long totalSpace= dvm->getTotalSpace();
   long long freeSpace= dvm->getFreeSpace();
   g_print( "\n\ntotal Space: %lld bytes\n", totalSpace );
   g_print( "free Space: %lld bytes\n\n\n", freeSpace );
   
   int count= dvm->getRecordingCount();
   g_print( "number of recordings= %d\n", count );
   g_print("-------------------------------------\n");
   for( int ii= 0; ii < count; ++ii )
   {
      RecordingInfo *pRecInfo= dvm->getRecordingInfoByIndex( ii );
      g_print( "recording %d id %s title \"%s\"\n", ii, pRecInfo->recordingId.c_str(), pRecInfo->title );
      
      if (withDetails)
      {
	for( int i= 0; i < pRecInfo->segmentCount; ++i )
	{
	  RecordingSegmentInfo *pSegInfo= pRecInfo->segments[i];

	  const char *segmentLocator= dvm->getRecordingSegmentLocator( pSegInfo->segmentName );
	  long long segmentSize= dvm->getRecordingSegmentSize( pSegInfo->segmentName );
	  long long segmentDuration= dvm->getRecordingSegmentDuration( pSegInfo->segmentName );
	  
	  g_print( " segment %d: name %lld locator=(%s)\n", i, pSegInfo->segmentName, segmentLocator );
	  g_print(" size %lld bytes duration %lld seconds\n", segmentSize, segmentDuration );
	  g_print( "   pcrPID: %04x\n", pSegInfo->pcrPid );
	  g_print( "   %d video components\n", pSegInfo->videoComponentCount );
	  for( int j= 0; j < pSegInfo->videoComponentCount; ++j )
	  {
	    g_print( "      video %d: pid %04x type %d siType %d elmStrmType %d name (%s) locator (%s)\n",
		    j,
		    pSegInfo->videoComponents[j].pid,
		    pSegInfo->videoComponents[j].streamType,
		    pSegInfo->videoComponents[j].siType,
		    pSegInfo->videoComponents[j].elemStreamType,
		    pSegInfo->videoComponents[j].name,
		    pSegInfo->videoComponents[j].locator );
	  }
	  g_print( "   %d audio components\n", pSegInfo->audioComponentCount );
	  for( int j= 0; j < pSegInfo->audioComponentCount; ++j )
	  {
	    g_print( "      audio %d: pid %04x type %d siType %d elmStrmType %d name (%s) locator (%s) language (%s)\n",
		    j,
		    pSegInfo->audioComponents[j].pid,
		    pSegInfo->audioComponents[j].streamType,
		    pSegInfo->audioComponents[j].siType,
		    pSegInfo->audioComponents[j].elemStreamType,
		    pSegInfo->audioComponents[j].name,
		    pSegInfo->audioComponents[j].locator,
		    pSegInfo->audioComponents[j].associatedLanguage );
	  }
	  g_print( "   %d data components\n", pSegInfo->dataComponentCount );
	  for( int j= 0; j < pSegInfo->dataComponentCount; ++j )
	  {
	    g_print( "      data %d: pid %04x type %d siType %d elmStrmType %d name (%s) locator (%s)\n",
		    j,
		    pSegInfo->dataComponents[j].pid,
		    pSegInfo->dataComponents[j].streamType,
		    pSegInfo->dataComponents[j].siType,
		    pSegInfo->dataComponents[j].elemStreamType,
		    pSegInfo->dataComponents[j].name,
		    pSegInfo->dataComponents[j].locator );
	  }
      } //end of for (int i= 0;
    } //end of if(withDetails)
  } //end of for(int ii=0 ..
  g_print("-------------------------------------\n");
  return DVRResult_ok;
}

int deleteRecording(std::string recordingId)
{
  long long totalSpace, freeSpace;
  int result = 0;
  
  do 
  {
    DVRManager *dvm= DVRManager::getInstance();
    if ( !dvm )
    {
	g_print( "Error: unable to DVRManager instance\nrmfApp->" );
	result = -1;
	break;
    }

    totalSpace= dvm->getTotalSpace();
    freeSpace= dvm->getFreeSpace();
    g_print( "total Space: %lld bytes\nrmfApp->", totalSpace );
    g_print( "free Space: %lld bytes\nrmfApp->", freeSpace );
    
    g_print( "Deleting recording id %s...\nrmfApp->", recordingId.c_str() );
    
    result = dvm->deleteRecording( recordingId );
    //g_print( "deleteRecording rc=%x\nrmfApp->", rc);
    
    totalSpace= dvm->getTotalSpace();
    freeSpace= dvm->getFreeSpace();
    g_print( "total Space: %lld bytes\nrmfApp->", totalSpace );
    g_print( "free Space: %lld bytes\nrmfApp->", freeSpace );
  } while(0);
  
  return result;
}

#endif
