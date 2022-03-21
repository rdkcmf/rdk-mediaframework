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
#include <stdio.h>
#include "rmf_error.h"

extern int rmfApp_init (void);
extern gboolean stdinCallback( GIOChannel *source, GIOCondition condition, gpointer data);
extern void start_loop( GMainLoop* loop);
extern int rmfApp_start(void);
extern int rmfApp_close(void);

int main(int argc, char** words)
{
  GIOChannel* stdin_ch;
 
  rmfApp_init();

  //bring up rmfApp shell
  g_print("rmfApp->");
  
  // Start watching stdin for commands.
  rmfApp_start();
  
  stdin_ch = g_io_channel_unix_new(fileno(stdin));
  g_io_add_watch(stdin_ch, G_IO_IN, stdinCallback, NULL);
  g_io_channel_ref(stdin_ch);
  
  GMainLoop* loop = g_main_loop_new(NULL, FALSE);
  start_loop(loop);

  g_print("------------ Bye --------------\n");

  rmfApp_close();
  
  g_main_loop_unref(loop);
  g_io_channel_unref(stdin_ch);
  g_print("------------ closing rmfApp --------------\n");
  
  return 0;
}


