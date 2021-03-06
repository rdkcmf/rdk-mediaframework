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
#include "logger.h"
#include <gst/gst.h>
#include <cstdarg>
#include <cstdlib>
#include <ctime>

/**
 * Map LogLevel to the actual logging levels of diff. loggers.
 */
template<typename T>
struct tLogLevelMap {
  LogLevel from;
  T to;
};

#if defined(USE_RDK_LOGGER)
  #include "rdk_debug.h"

  static tLogLevelMap<rdk_LogLevel> levelMap[] = {
     { FATAL_LEVEL,   RDK_LOG_FATAL }
    ,{ ERROR_LEVEL,   RDK_LOG_ERROR }
    ,{ WARNING_LEVEL, RDK_LOG_WARN }
    ,{ METRIC_LEVEL,  RDK_LOG_INFO }
    ,{ INFO_LEVEL,    RDK_LOG_INFO }
    ,{ VERBOSE_LEVEL, RDK_LOG_DEBUG }
    ,{ TRACE_LEVEL,   RDK_LOG_TRACE1 }
  };

#else
  static tLogLevelMap<const char*> levelMap[] = {
     { FATAL_LEVEL,   "Fatal" }
    ,{ ERROR_LEVEL,   "Error" }
    ,{ WARNING_LEVEL, "Warning" }
    ,{ METRIC_LEVEL,  "Metric" }
    ,{ INFO_LEVEL,    "Info" }
    ,{ VERBOSE_LEVEL, "Verbose" }
    ,{ TRACE_LEVEL,   "Trace" }
  };

  static int default_log_level = DEFAULT_LEVEL;
#endif

#if defined(USE_RDK_LOGGER)
  static rdk_LogLevel convertGstDebugLevelToRdkLogLevel(GstDebugLevel level)
  {
      switch (level)
      {
      case GST_LEVEL_ERROR :
          return RDK_LOG_ERROR;
      case GST_LEVEL_WARNING :
          return RDK_LOG_WARN;
      case GST_LEVEL_FIXME :
          return RDK_LOG_NOTICE;
      case GST_LEVEL_INFO :
          return RDK_LOG_INFO;
      case GST_LEVEL_DEBUG :
          return RDK_LOG_DEBUG;
      default :;
      }
      return RDK_LOG_TRACE1;
  }
#endif

  static void gstLogFunction(GstDebugCategory*, GstDebugLevel level, const gchar* file,
                      const gchar* function, gint, GObject*, GstDebugMessage* message, gpointer)
  {
      const gchar* text = gst_debug_message_get(message);
#if defined(USE_RDK_LOGGER)
      rdk_LogLevel rll = convertGstDebugLevelToRdkLogLevel(level);
      RDK_LOG(rll, "LOG.RDK.GSTREAMER", "%s %s\n", function, text);
#else
      (void)level;
      fprintf(stdout, format, text);
      fflush(stdout);
#endif
  }

  void log_init() {
#if defined(USE_RDK_LOGGER)
  rdk_logger_init("/etc/debug.ini");
#else
  char* value = getenv("LOG_RMF_DEFAULT");
  if (value != 0) {
    default_log_level = atoi(value);
  }
#endif

  if (gst_debug_get_default_threshold() < GST_LEVEL_WARNING)
      gst_debug_set_default_threshold(GST_LEVEL_WARNING);
  gst_debug_remove_log_function(gst_debug_log_default);
  gst_debug_add_log_function(&gstLogFunction, 0, 0);
}

#define FMT_MSG_SIZE       (4096)

#if defined(USE_RDK_LOGGER)

void log(LogLevel level,
         const char* func,
         const char* file,
         int line,
         const char* format, ...){

  // Rdklogger is backed with log4c which has its own default level
  // for filtering messages. Therefore, we don't check level here.
  char formatted_string[FMT_MSG_SIZE];
  char formatted_string2[FMT_MSG_SIZE];
  va_list argptr;
  va_start(argptr, format);
  vsnprintf(formatted_string, FMT_MSG_SIZE, format, argptr);
  snprintf(formatted_string2, FMT_MSG_SIZE, "%s:%s:%d %s", func,
                                            basename(file),
                                            line,
                                            formatted_string);
  va_end(argptr);

  // Currently, we use customized layout 'dated_nocr' in log4c.
  // This layout doesn't have trailing carriage return, so we need
  // to add it explicitly.
  // Once the default layout is used, this addition should be deleted.
  const char* log_str = (METRIC_LEVEL == level)
    ? formatted_string
    : formatted_string2;
  RDK_LOG(levelMap[static_cast<int>(level)].to,
          "LOG.RDK.RMFBASE",
          "%s\n",
          log_str);

  if (FATAL_LEVEL == level)
    std::abort();
}

#else

static char* timestamp(char* buff);

void log(LogLevel level,
         const char* func,
         const char* file,
         int line,
         const char* format, ...){

  if (default_log_level >= level) {

    char formatted_string[FMT_MSG_SIZE];
    va_list argptr;
    va_start(argptr, format);
    vsnprintf(formatted_string, FMT_MSG_SIZE, format, argptr);
    va_end(argptr);

    char buff[0xFF] = {0};
    fprintf(stdout, "%s [%s] %s:%s:%d %s",
            timestamp(buff),
            levelMap[static_cast<int>(level)].to,
            func, basename(file), line,
            formatted_string);

    fprintf(stdout, "\n");
    fflush(stdout);

    if (FATAL_LEVEL == level)
      std::abort();
  }
}

static char* timestamp(char* buff) {
  struct timespec spec;
  struct tm tm;

  clock_gettime(CLOCK_REALTIME, &spec);
  gmtime_r(&spec.tv_sec, &tm);
  long ms = spec.tv_nsec / 1.0e6;

  sprintf(buff, "%02d%02d%02d-%02d:%02d:%02d.%03ld",
                tm.tm_year + (1900-2000),
                tm.tm_mon + 1,
                tm.tm_mday,
                tm.tm_hour,
                tm.tm_min,
                tm.tm_sec,
                ms);

  return buff;
}

#endif
