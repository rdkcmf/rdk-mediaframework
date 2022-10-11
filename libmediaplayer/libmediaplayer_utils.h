#include "rdk_debug.h"

#if 1 //Temporarily switch away from rdklogger till we get it working.

#define INT_FATAL(FORMAT, ...)      RDK_LOG(RDK_LOG_FATAL, "LOG.RDK.LMPLYR", FORMAT, __VA_ARGS__)
#define INT_ERROR(FORMAT, ...)      RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.LMPLYR", FORMAT, __VA_ARGS__)
#define INT_WARNING(FORMAT, ...)    RDK_LOG(RDK_LOG_WARN, "LOG.RDK.LMPLYR", FORMAT,  __VA_ARGS__)
#define INT_INFO(FORMAT, ...)       RDK_LOG(RDK_LOG_INFO, "LOG.RDK.LMPLYR", FORMAT,  __VA_ARGS__)
#define INT_DEBUG(FORMAT, ...)      RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.LMPLYR", FORMAT, __VA_ARGS__)
#define INT_TRACE1(FORMAT, ...)     RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.LMPLYR", FORMAT,  __VA_ARGS__)
#define INT_TRACE2(FORMAT, ...)     RDK_LOG(RDK_LOG_TRACE2, "LOG.RDK.LMPLYR", FORMAT,  __VA_ARGS__)
#define INT_TRACE3(FORMAT, ...)     RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.LMPLYR", FORMAT,  __VA_ARGS__)
#define INT_TRACE4(FORMAT, ...)     RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.LMPLYR", FORMAT,  __VA_ARGS__)

#define FATAL(...)                  INT_FATAL(__VA_ARGS__, "")
#define ERROR(...)                  INT_ERROR(__VA_ARGS__, "")
#define WARNING(...)                INT_WARNING(__VA_ARGS__, "")
#define INFO(...)                   INT_INFO(__VA_ARGS__, "")
#define DEBUG(...)                  INT_DEBUG(__VA_ARGS__, "")
#define TRACE1(...)                 INT_TRACE1(__VA_ARGS__, "")
#define TRACE2(...)                 INT_TRACE2(__VA_ARGS__, "")
#define TRACE3(...)                 INT_TRACE3(__VA_ARGS__, "")
#define TRACE4(...)                 INT_TRACE4(__VA_ARGS__, "")

#else

#include <stdio.h>
//#define ENABLE_DEBUG 1
#define LOG(level, text, ...) do {\
    printf("%s[%d] - %s: " text, __FUNCTION__, __LINE__, level, ##__VA_ARGS__);}while(0);

#define ERROR(text, ...) do {\
    printf("%s[%d] - %s: " text, __FUNCTION__, __LINE__, "ERROR", ##__VA_ARGS__);}while(0);
#define WARN(text, ...) do {\
    printf("%s[%d] - %s: " text, __FUNCTION__, __LINE__, "WARN", ##__VA_ARGS__);}while(0);
#define INFO(text, ...) do {\
    printf("%s[%d] - %s: " text, __FUNCTION__, __LINE__, "INFO", ##__VA_ARGS__);}while(0);

#define DEBUG(text, ...) do {\
    printf("%s[%d] - %s: " text, __FUNCTION__, __LINE__, "DEBUG", ##__VA_ARGS__);}while(0);

#endif
