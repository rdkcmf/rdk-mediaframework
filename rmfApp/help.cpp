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
#include <glib.h>

void playHelp(void)
{
 g_print(" \
\nplay\n\
===\n\
play command used to play the entered url. Check trickplay keys section to see available keys for trickplays \n \
  \n \
usage : play <options> <url> \n \
where options are \n \
     -tsb: to enable local tsb (works only for qamsource)\n \
     -duration : TSB duration in minutes. Default duration : 60 mins.\n \
\n \
url: valid urls should start with\n \
    ocap://<source id>\n \
    http://<url> \n \
    vod://<url>\n \
    dtv://<original network id>.<transport id>.<service id> - DVB triplet of service to be viewed\n \
\n \
Examples:\n \
play ocap://0x236A\n \
play http://127.0.0.1:8080/vldms/tuner?ocap_locator=ocap://0x236A\n \
play vod://indemand.com/INTL0312000005367027\n \
play dtv://9018.4161.1234\n\
\
\n\
");
}
void trickHelp(void)
{
 g_print(" \
trickplay keys\n\
===============\n\
\n\
   p - pause/resume\n\
   s - skip back to the beginning\n\
   e - skip to end of recording\n\
   b - replay (skip back 15 seconds)\n\
   f - fast forward (cycle between 4x, 15x, 30x, 60x)\n\
   r - fast reverse (cycle between -4x, -15x, -30x, -60x)\n\
   l - switch to live (in TSB mode, not valid on playing recorded content)\n\
\n\
");

}
void recordHelp(void)
{
 g_print("\
record\n\
=====\n\
record command used to record given url. Each record comamnd should be passed with a unique Id to indetify the recording. It will alert if id is already present.\n\
\n\
usage: record -id <unique Id> <options> <url>\n\
\n\
other options are: \n\
     -duration <time in mins> Default duration : 30 mins.\n\
     -title <content name> : Title name to your recording. If not given assumes DeliaTest as the name.\n\
url: valid urls should start with\n\
    ocap://<source id>\n\
    http://<url>  Not supported in this version\n\
    vod://<url>  Not supported in this version\n\
\n\
");
}
void listHelp()
{
g_print("\
ls, list\n\
========\n\
lists available recordings\n\
\n\
To list detailed info. enter\n\
ls details or \n\
list details\n\
\n\
");
}

void deleteHelp(void)
{
g_print("\
delete, rm\n\
=========\n\
To delete a recording, use either delete or rm\n\
rm <recordingId>\n\
delete <recording Id>\n\
\n\
");
}

void psHelp(void)
{
g_print("\
ps\n\
====\n\
ps lists current running process/pipelines\n\
\n\
");

}

void launchHelp(void)
{
g_print("\
launch\n\
=======\n\
launch command used to launch a desired pipeline.\n\
usage:\n\
launch -source <RMF source> -sink <RMF Sink> <options> <url>\n\
    supported sources are \"qamsource\", \"hnsource\", \"dvrsource\", \"vodsource\"\n\
    suppoted sinks are \"mediaplayersink\", \"dvrsink\"\n\
options: \n\
     -tsb: to enable local tsb (works only for qamsource)\n\
for recording following options are needed\n\
     -id <unique Id>\n\
     -title <content name> : Title name to your recording. If not given assumes DeliaTest as the name.\n\
     -duration <time in mins> Default duration : 30 mins for recording and 60 mins for TSB\n\
\n\
");

}
void killHelp(void)
{
g_print("\
kill\n\
======\n\
Kill command can be used to kill/stop running commands/pipeline. \n\
Usage: Kill <process number>\n\
Use command ps to lists currently running processes\n\
\n\
");

}
void sleepHelp(void)
{
g_print("\
sleep\n\
======\n\
sleep command can be used to add delay in between operations. \n\
Usage: sleep <time in seconds>\n\
\n\
");

}
void scriptHelp(void)
{
g_print("\
script\n\
======\n\
script command can be used to execute commands in a file. \n\
Usage: script <filename> [ optional number of iterations] \n\
\n\
");

}
void avcheckHelp(void)
{
g_print("\
avcheck\n\
======\n\
avcheck command can be used to check av status if supported by the platform. \n\
Usage: avcheck\n\
\n\
");

}

#if defined(ENABLE_CLOSEDCAPTION)
void closedcaptionhelp()
{
  g_print("\
  Closedcaption commands: \n\
  enable or e: To enable closedcaption\n\
  disable or d: To disable closedcaption\n\
  show or s: To show the closedcaption on screen if hidden\n\
  hide or h: To hide the closedcaption on screen if showing\n\ 
\n\
");
}
#endif

int printAll(void)
{
  playHelp();
  recordHelp();
  launchHelp();
  listHelp();
#if defined(ENABLE_CLOSEDCAPTION)
  closedcaptionhelp();
#endif
  deleteHelp();
  psHelp();
  killHelp();
  trickHelp();
  avcheckHelp();
  sleepHelp();
  scriptHelp();
  return 0;
}
