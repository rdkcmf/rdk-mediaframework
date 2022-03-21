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


#ifndef __VL_EXPORT_TYPES_H
#define __VL_EXPORT_TYPES_H

/*
#ifndef VL_NO_WARNINGS
#define VL_NO_WARNINGS		1
#endif
 */

/* BEGIN: vlBYTE, vlBOOL, vlUINT16, vlUINT32 */
#ifndef NULL
#define NULL   0x0
#endif

#ifndef vlTRUE
#define vlTRUE 1
#endif

#ifndef vlFALSE
#define vlFALSE 0
#endif

#ifndef _VLSTATUS_
typedef long vlSTATUS;
#define _VLSTATUS_
#endif

/* Basic Data Types */
typedef unsigned char	vlBYTE;
typedef unsigned short	vlUINT16;
typedef vlUINT16 vlUSHORT;
typedef unsigned int	vlUINT32;
typedef int				vlINT32;
#ifdef  WIN32
	typedef __int64             vlINT64;    /* 64-bit signed data type   */
	typedef unsigned __int64    vlUINT64;    /* 64-bit unsigned data type */
#else
	typedef long long             vlINT64;    /* 64-bit signed data type   */
	typedef unsigned long long	vlUINT64;
#endif // WIN32
typedef unsigned long	vlULONG;
typedef unsigned char   vlBOOL; 
typedef unsigned char   vlBOOLEAN; 
typedef short			vlSHORT;
typedef vlULONG 		CLIENT_HANDLE;
#define INVALID_CLIENT_HANDLE 	NULL

typedef struct _vlbyteArray
{
    vlBYTE    size;
    vlBYTE*   value;
} vlByteArray;

typedef struct _vlbyteArray32
{
    vlBYTE    size;
    vlBYTE    value[32];
} vlByteArray32;

typedef struct _vlbyteArray16
{
    vlBYTE    size;
    vlBYTE    value[16];
} vlByteArray16;

typedef struct _vlbyteArray8
{
    vlBYTE    size;
    vlBYTE    value[8];
} vlByteArray8;

typedef struct _vlbyteArray4
{
    vlBYTE    size;
    vlBYTE    value[4];
} vlByteArray4;

#ifndef NULL
#define NULL   0x0
#endif


#define vlIN 
#define vlOUT
#define vlINOUT 
#endif /*__VL_EXPORT_TYPES_H*/

