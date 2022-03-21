/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2018 RDK Management
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
#include <unistd.h>
#include <stdio.h>
#include <sys/prctl.h>

#include <glib.h>
#include <gst/gst.h>

#include <rtRemote.h>
#include <rtLog.h>

#include "rtrmfplayer.h"
#include "logger.h"
#include "glib_tools.h"
#include "hangdetector_utils.h"

GMainLoop *gMainLoop = 0;

#ifdef ENABLE_BREAKPAD
#include "breakpad_wrapper.h" 
#endif

static void rtRemoteLogHandler(rtLogLevel level, const char* file, int line, int threadId, char* message) {
 LogLevel logLevel;
 switch (level) {
   case RT_LOG_DEBUG: logLevel = VERBOSE_LEVEL; break;
   case RT_LOG_INFO:  logLevel = INFO_LEVEL; break;
   case RT_LOG_WARN:  logLevel = WARNING_LEVEL; break;
   case RT_LOG_ERROR: logLevel = ERROR_LEVEL; break;
   case RT_LOG_FATAL: logLevel = FATAL_LEVEL; break;
   default: logLevel = INFO_LEVEL; break;
 }
 #ifdef USE_RDK_LOGGER
 log(logLevel, "rtLog", file, line, "%s", message);
 #else
 log(logLevel, "rtLog", file, line, "Thread=%d %s", threadId, message);
 #endif
}

int gPipefd[2];

void rtMainLoopCb(void*)
{
  rtError err;
  err = rtRemoteProcessSingleItem();
  if (err == RT_ERROR_QUEUE_EMPTY)
   LOG_TRACE("queue was empty upon processing event");
  else if (err != RT_OK)
   LOG_WARNING("rtRemoteProcessSingleItem() returned %d", err);
}

void rtRemoteCallback(void*)
{
  LOG_TRACE("queueReadyHandler entered");
  static char temp[1];
  int ret = HANDLE_EINTR_EAGAIN(write(gPipefd[PIPE_WRITE], temp, 1));
  if (ret == -1)
    perror("can't write to pipe");
}

int main(int argc, char *argv[]) {
  Utils::HangDetector hangDetector;
  hangDetector.start();

  prctl(PR_SET_PDEATHSIG, SIGHUP);

  #if 0
  dup2(STDOUT_FILENO, STDERR_FILENO);
  setenv("RT_LOG_LEVEL", "debug", 1);
  setenv("LOG_RMF_DEFAULT", "4", 1);
  #endif

  #ifdef ENABLE_BREAKPAD
  breakpad_ExceptionHandler();
  #endif

  gst_init(0, 0);
  log_init();

  rtLogSetLogHandler(rtRemoteLogHandler);

  LOG_INFO("Init rtRemote");
  rtError rc;
  gMainLoop = g_main_loop_new(0, FALSE);

  auto *source = pipe_source_new(gPipefd, rtMainLoopCb, nullptr);
  g_source_attach(source, g_main_loop_get_context(gMainLoop));

  rtRemoteRegisterQueueReadyHandler( rtEnvironmentGetGlobal(), rtRemoteCallback, nullptr );

  rc = rtRemoteInit();
  ASSERT(rc == RT_OK);

  const char* objectName = getenv("PX_WAYLAND_CLIENT_REMOTE_OBJECT_NAME");
  if (!objectName) objectName = "RMF_PLAYER";
  LOG_INFO("Register: %s", objectName);

  rtObjectRef playerRef = new rtRMFPlayer;
  rc = rtRemoteRegisterObject(objectName, playerRef);
  ASSERT(rc == RT_OK);

  {
    LOG_INFO("Start main loop");
    g_main_loop_run(gMainLoop);
    g_main_loop_unref(gMainLoop);
    gMainLoop = 0;
  }

  playerRef = nullptr;

  LOG_INFO("Shutdown");
  rc = rtRemoteShutdown();
  ASSERT(rc == RT_OK);

  gst_deinit();
  g_source_unref(source);

  return 0;
}
