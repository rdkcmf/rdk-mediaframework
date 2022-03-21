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
 

#include "hnsource.h"
#include "mediaplayersink.h"

#include <glib.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <sys/time.h>
#include <string.h>
#include "rmf_osal_init.h"

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

static RMFMediaSourceBase* g_pSource= 0;
static RMFMediaSinkBase* g_pSink= 0;
static GMainLoop* gMainLoop= 0;
static int grun =1;


//When adding a new source or sink you need to:
//0) Add the #include for your source/sink header above
//1) Add the #define to the list of defines below for the name as it should appear on command line
//2) Update createSource/createSink to do the actually allocation of the source/sink
//3) Update processCommandLine to check for the new source/sink name
//4) Update the Makefile and include your player.mk

#define HN_SOURCE			"hnsource"
#define MEDIAPLAYER_SINK	"mediaplayersink"
#define DVR_SOURCE			"dvrsource"
#define DVR_SINK			"dvrsink"
#define QAM_SOURCE			"qamsource"

static RMFMediaSinkBase* createSink(RMFMediaSourceBase* pSource)
{
	MediaPlayerSink* pSink = new MediaPlayerSink();
	pSink->init();
	pSink->setVideoRectangle(0, 0, 1280, 720);
	pSink->setSource(pSource);
	return pSink;
}


class EventHandler : public IRMFMediaEvents
{
public:
	EventHandler() 
	{
	}
	void complete()
	{
		g_print("Completed Event\n");
	}
	void error(RMFResult err, const char* msg)
	{
		(void)err;
		g_print("Error Event: %s\n", msg);
	}
private:
};

static void shutDown()
{
   {
	  if ( gMainLoop )
	  {
		 g_main_loop_quit( gMainLoop );
		 gMainLoop= 0;
	  }
	  if ( g_pSource )
	  {
		 g_pSource->pause();
		 g_pSource->close();
	  }
	  if ( g_pSink )
	  {
		 g_pSink->term();
		 delete g_pSink;
		 g_pSink= 0;
	  }
	  if ( g_pSource )
	  {
		 g_pSource->term();
		 delete g_pSource;
		 g_pSource= 0;
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
		ONCASE(SIGINT); 	/* CTRL-C	*/
		ONCASE(SIGQUIT);	/* CTRL-\	*/
		ONCASE(SIGILL);
		ONCASE(SIGABRT);	/* abort()	*/
		ONCASE(SIGFPE);
		ONCASE(SIGSEGV);
		ONCASE(SIGTERM);
		ONCASE(SIGPIPE);
#undef ONCASE
		default: g_print("Caught unknown signal %d in %s:%d\n", signum, __FILE__, __LINE__); break;
	}
	signal(signum, SIG_DFL);	// restore default handler

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
	   grun = 0;
	}
	else
	{
	   // perform a graceful cleanup
	   {
		   shutDown();
	   }
	   kill(getpid(), signum);	   // invoke default handler
	}
}

static void setupExit(void)
{
	// cannot catch SIGKILL or SIGSTOP
	signal(SIGINT,	sighandler); // CTRL-C
	signal(SIGQUIT, sighandler);
	signal(SIGILL,	sighandler);
	signal(SIGABRT, sighandler);
	signal(SIGFPE,	sighandler);
	signal(SIGSEGV, sighandler);
	signal(SIGTERM, sighandler);
	signal(SIGPIPE, sighandler);
	atexit(shutDown);
}

//#define USE_APACHE_SERVER
//#define DTCP_SUP

#ifdef USE_APACHE_SERVER
#define NUMBER_OF_LOCATORS 3
#define URI_FORMAT_STRING "http://%s:8080/stream%d.ts"
#define SERVER_IP "192.168.161.29"
#else


#ifndef DTCP_SUP
#define URI_FORMAT_STRING "http://%s:8080/vldms/tuner?ocap_locator=ocap://0x%x"
#else
#define URI_FORMAT_STRING "http://%s:8080/vldms/tuner?ocap_locator=ocap://0x%x&CONTENTPROTECTIONTYPE=DTCP1&DTCP1HOST=%s&DTCP1PORT=5000"
#endif
#if 0
//Locators for testing at Offshore

static int locators[] = {
			0xA0,
			0x9E,
			0x9D,
			0x9F,
			0xA1,
			0xA2,
			0xA3,
			0xA4,
			0xA5,
			0xA6,
			0xA7,
			0xA8,
			0xA9,
			0xAA,
			};
#define NUMBER_OF_LOCATORS (sizeof(locators) / sizeof(int))
#else
#if 0
//Locators for testing at Onsite
#define NUMBER_OF_LOCATORS 74
static int locators[] = { 0x4F3C, 0x236A, 0x4EDF,
						0xF9FA,
						0x473B,
						0x31C7,
						0x3F53,
						0x182B,
						0x2DC5,
						0x4174,
						0x3ECF,
						0x40A3,
						0x3F70,
						0x3ECE,
						0x3F0D, 0x3FCC, 0x3FDA, 0x43CA, 0x444F, 0x3EF9, 0x3EF8, 0x3EE2, 0x4065, 0x41B1, 0X419E, 0X402E, 0X3F36, 0X4426, 0X444E, 0X444D, 0X43CC, 0X43CB, 0X401D, 0X412E, 0X4169, 0X4149, 0X40A4, 0X4123, 0X4013, 0X409F, 0X401E, 0X3F70, 0X4120, 0X3EFF,0x236A, 
						0xF9FA,
						0x473B,
						0x31C7,
						0x3F53,
						0x182B,
						0x2DC5,
						0x4174,
						0x3ECF,
						0x40A3,0x236A, 
						0xF9FA,
						0x473B,
						0x31C7,
						0x3F53,
						0x182B,
						0x2DC5,
						0x4174,
						0x3ECF,
						0x40A3,0x236A, 
						0xF9FA,
						0x473B,
						0x31C7,
						0x3F53,
						0x182B,
						0x2DC5,
						0x4174,
						0x3ECF,
			 			0x40A3,
						};
#else
#if 0
static int locators[] = {
0x00321d,  0x00236a,  0x003f53,  0x00f9fa,  0x00473b,  0x00106b,  0x00100d,  0x002f79,  0x003ef4,  0x001056,  0x00174c,  0x001010,  0x001e73,  0x002f8f,  
0x002f58,  0x002f13,  0x001020,  0x00205d,  0x0036ad,  0x002f12,  0x00101b,  0x001afe,  0x002f71,  0x001008,  0x002ace,  0x001015,  0x004d95,  0x00101c,  
0x00182b,  0x001185,  0x002acf,  0x0023f1,  0x002753,  0x00273d,  0x00273e,  0x004ea2,  0x004e95,  0x004e96,  0x004f73,  0x004e88,  0x004f3d,  0x004e89,  
0x004ea1,  0x004e87,  0x004e90,  0x004f71,  0x004f41,  0x004e91,  0x004e8e,  0x004e8f,  0x004f85,  0x004f72,  0x004e86,  0x004e94,  0x003a21,  0x004e85,  
0x004e9c,  0x004f88,  0x004ea0,  0x004f40,  0x004f3a,  0x004e92,  0x004e93,  0x004e8c,  0x004ea3,  0x004e8b,  0x004e8a,  0x004e8d,  0x004e97,  0x004e99,  
0x004e98,  0x004f87,  0x004e9d,  0x004e9e,  0x004e9a,  0x004e9b,  0x004f3c,  0x004e9f,  0x004f3f,  0x003006,  0x004f86,  0x0033ad,  0x002739,  0x001053,  
0x00000e,  0x003232,  0x003477,  0x003310,  0x002ef4,  0x0011c1,  0x003476,  0x002772,  0x0044c3,  0x004174,  0x003f0d,  0x003fcc,  0x003fda,  0x0040a3,  
0x0043ca,  0x00444f,  0x003ef9,  0x003ef8,  0x004065,  0x0041b1,  0x00419e,  0x004015,  0x00402e,  0x003f36,  0x004426,  0x00444e,  0x00444d,  0x0043cc,  
0x0043cb,  0x00401d,  0x00412e,  0x004169,  0x004149,  0x0040a4,  0x004134,  0x004123,  0x004013,  0x00409f,  0x00401e,  0x003f70,  0x004120,  0x003eff,  
0x00401c,  0x0031c7,  0x002dc5,  0x003ece,  0x003ecf,  0x00c571,  0x003e20,  0x0028b3,  0x00c650,  0x0032fd,  0x0031a8,  0x003332,  0x00c38a,  0x003182,  
0x00c479,  0x0031a9,  0x00318b,  0x003429,  0x00c372,  0x002971,  0x002646,  0x004376,  0x00c6c6,  0x002249,  0x003ea3,  0x002c09,  0x002ca5,  0x0010a2,  
0x0032c4,  0x004531,  0x0038ea
};
#endif
static int locators[] = {
0x002d57,  0x002d03,  0x00320e,  0x002d9d,  0x00453b,  0x002d58,  0x002d9e,  0x002d57, 0x004673, 0x004672, 0x004739};


#define NUMBER_OF_LOCATORS (sizeof(locators) / sizeof(int))
#endif
#endif
#endif

int main(int argc, char** argv)
{
	std::string source;
	std::string sink;
	std::string url;
	rmf_osal_init( NULL, NULL);
	RMFMediaSourceBase* pSource;
	RMFMediaSinkBase* pSink;
	EventHandler events;
	char uri[1024];
	char fileDump[1024];
	int count =0;
        int loop_count=0;
	bool bTsb = FALSE;
	char * ip = "127.0.0.1";

	if (argc >= 2)
		ip = argv[1];

	if((argc > 2) && (strcmp(argv[2],"WITH_TSB")==0))
		bTsb = TRUE;

	setupExit();
	memset(fileDump,0,1024);
	sprintf(fileDump,"rm -rf 0x*.ts");
	system(fileDump);
	while (grun)
	{
	        memset(fileDump,0,1024);
        	sprintf(fileDump,"rm -rf SRC_OUT_*");
	        system(fileDump);
		
		count ++;
		loop_count++;

		g_print("========================= Channel Change count: %d =================================\n", loop_count);
		if (count >= NUMBER_OF_LOCATORS)
			count =0;
		// Instantiate source and sink.
		pSource = new HNSource();

		if(pSource == 0)
		{
			g_print("rmfplayer: source '%s' is invalid.  Try rfmplayer -help for more info.\n", source.c_str());
			return 0;
		}
		g_pSource= pSource;

		pSource->init();
		//sprintf(uri,"http://%s:8080/vldms/tuner?ocap_locator=ocap://0x%x", ip, locators[count]);
		sprintf(uri,"http://%s:8080/vldms/tuner?ocap_locator=ocap://0x%x&CONTENTPROTECTIONTYPE=DTCP1&DTCP1HOST=127.0.0.1&DTCP1PORT=5000", ip, locators[count]);


#ifndef USE_APACHE_SERVER
#ifndef DTCP_SUP
		sprintf(uri,URI_FORMAT_STRING, ip, locators[count]);
		if(bTsb)
			sprintf(uri, "%s&tsb=1",uri);
#else
		sprintf(uri,URI_FORMAT_STRING, ip, locators[count], ip);
		if(bTsb)
			sprintf(uri, "%s&tsb=1",uri);
#endif
#else
		sprintf(uri,URI_FORMAT_STRING, SERVER_IP, count);
#endif

		g_print("Requesting uri %s\n", uri);
		g_print("TUNING TO SOURCEID: 0x%x\n", locators[count]);
		pSource->open(uri, 0);

		pSink = createSink( pSource);

		if(pSink == 0)
		{
			g_print("rmfplayer: sink '%s' is invalid.  Try rfmplayer -help for more info.\n", sink.c_str());
			delete pSource;
			return 0;
		}
		g_pSink= pSink;

		// Start playback.
		pSource->setEvents(&events);
		pSource->play();
		sleep(6);
		system("cd /opt; HOMIE.sh 0 0");
		sleep(15);
		memset(fileDump,0,1024);
		//FEATURE.ADD.DUMPFILESINK=1 is added in rmfconfig.ini
		sprintf(fileDump,"cd /opt; mv SRC_OUT_* 0x%x_%d.ts",locators[count], loop_count);
		system(fileDump);
		//sleep (10);
		shutDown();

	}
	return 0;
}

