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

#if !defined(_RMF_OSAL_TYPES_H_)
#define _RMF_OSAL_TYPES_H_

#ifndef __ASSEMBLY__

#include <time.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h> /* uint32_t etc. */

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
#define OS_LIBEXPORT(type, symbol) extern "C" type symbol
#else
#define OS_LIBEXPORT(type, symbol) extern type symbol
#endif

/***
 *** 20051007 - put these here to resolve a preprocessor issue
 ***/
typedef pthread_t       	os_ThreadId;
typedef pthread_mutex_t *	os_Mutex;


typedef int32_t rmf_osal_Bool;

#define os_ThreadId_Invalid (0)

#ifndef TRUE
#define TRUE (1==1)
#endif


#ifndef FALSE
#define FALSE (1!=1)
#endif

/*
 * Define macro for calculating the offset of a field in a struct:
 *
 * @param Struc is the structure pointer type
 * @param field is the field of interest in the struct
 */
#define RMF_OSAL_OFFSET(Struc, field) ((uint32_t) &((Struc *)0)->field)

/*
 * Define macro for construction of a fake list head pointer:
 *
 * @param Struc is the structure pointer type
 * @param head is the address location to offset from (usually the first fake link)
 * @param link is the link field name within the struct
 */
#define RMF_OSAL_FAKEHEAD(Struc, head, link) ((Struc *)((char *)&head - RMF_OSAL_OFFSET(Struc, link)))


#if !defined(RMF_OSAL_BIG_ENDIAN) && !defined(RMF_OSAL_LITTLE_ENDIAN)
#error Either RMF_OSAL_BIG_ENDIAN or RMF_OSAL_LITTLE_ENDIAN must be defined
#endif


#ifndef RMF_OSAL_UNUSED_PARAM
#define RMF_OSAL_UNUSED_PARAM(var)   { (void)var; }
#endif

#ifdef __cplusplus
}
#endif

#endif /* _RMF_OSAL_TYPES_H_ */

#endif /*__ASSEMBLY__*/ /*FD-20090114*/
