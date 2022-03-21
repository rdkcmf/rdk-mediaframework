/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2011 RDK Management
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




/****************************************************************************/
#ifndef __VL_TRACE_STACK_H__
#define __VL_TRACE_STACK_H__
/****************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus
/****************************************************************************/
typedef enum tag_VL_TRACE_STACK_
{
    VL_TRACE_STACK_TAG_NORMAL,
    VL_TRACE_STACK_TAG_BEGIN,
    VL_TRACE_STACK_TAG_END
    
} VL_TRACE_STACK_TAG_TYPES;

/****************************************************************************/
void vlTraceStack(int nLevel, VL_TRACE_STACK_TAG_TYPES nTagType,
                    const char * lpszTag, const char * lpszFormat, ...);
char * vlPrintBytesToString(int             nBytes,
                             const char  *   pBytes,
                             int             nStrSize,
                             char        *   pszString);
                             
char * vlPrintLongToString(int             nBytes,
                           unsigned long long nLong,
                           int             nStrSize,
                           char       *    pszString);
                             
#define VL_PRINT_BUF_TO_STR(array, string)          vlPrintBytesToString(sizeof(array), (const char*)(array), sizeof(string), (string))
#define VL_PRINT_LONG_TO_STR(long, bytes, string)   vlPrintLongToString((bytes), (long), sizeof(string), (string))

/****************************************************************************/
#ifdef __cplusplus
}
#endif //__cplusplus
/****************************************************************************/
#endif // __VL_TRACE_STACK_H__
/****************************************************************************/
