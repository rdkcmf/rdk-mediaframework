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
#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <cstdio>
#include <cassert>
#include <cstring>

/**
 * Enable rdk_logger. If not defined, fallback to stdout logging.
 */
// #define USE_RDK_LOGGER

#define DEFAULT_LEVEL INFO_LEVEL

/**
 * @brief Logging level with an increasing order of refinement
 * (TRACE_LEVEL = Finest logging).
 * It is essental to start with 0 and increase w/o gaps as the value
 * can be used for indexing in a mapping table.
 */
enum LogLevel {
  FATAL_LEVEL = 0,
  ERROR_LEVEL,
  WARNING_LEVEL,
  METRIC_LEVEL,
  INFO_LEVEL,
  VERBOSE_LEVEL,
  TRACE_LEVEL,
};

/**
 * @brief Init logging
 * Should be called once per program run before calling log-functions
 */
void log_init();

/**
 * @brief Log a message
 * The function is defined by logging backend.
 * Currently 2 variants are supported: rdk_logger (USE_RDK_LOGGER),
 *                                     stdout(default)
 */
void log (LogLevel level,
          const char* func,
          const char* file,
          int line,
          const char* format, ...);

#define LOG(LEVEL, FORMAT, ...)                                     \
            log( LEVEL,                                             \
                 __func__, __FILE__, __LINE__,                      \
                 FORMAT,                                            \
                 ##__VA_ARGS__)

#define LOG_INIT                log_init
#define LOG_TRACE(FMT, ...)     LOG(TRACE_LEVEL, FMT, ##__VA_ARGS__)
#define LOG_VERBOSE(FMT, ...)   LOG(VERBOSE_LEVEL, FMT, ##__VA_ARGS__)
#define LOG_METRIC(FMT, ...)    LOG(METRIC_LEVEL, FMT, ##__VA_ARGS__)
#define LOG_INFO(FMT, ...)      LOG(INFO_LEVEL, FMT, ##__VA_ARGS__)
#define LOG_WARNING(FMT, ...)   LOG(WARNING_LEVEL, FMT, ##__VA_ARGS__)
#define LOG_ERROR(FMT, ...)     LOG(ERROR_LEVEL, FMT, ##__VA_ARGS__)
#define LOG_FATAL(FMT, ...)     LOG(FATAL_LEVEL, FMT, ##__VA_ARGS__)

#ifndef NDEBUG
#define LOG_ASSERT(FMT, ...)    LOG(FATAL_LEVEL, FMT, ##__VA_ARGS__)
#else
#define LOG_ASSERT(FMT, ...)    LOG(ERROR_LEVEL, FMT, ##__VA_ARGS__)
#endif

#define ASSERT(x)                               \
  ((x)                                          \
   ? (void)(0)                                  \
   : LOG_ASSERT("'%s' check failed.", #x) )     \


#endif  // _LOGGER_H_
