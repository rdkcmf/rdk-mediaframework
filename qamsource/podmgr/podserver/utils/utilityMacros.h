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



///////////////////////////////////////////////////////////////////////////////////////////////////
// CAUTION: Only primitive types like char, short, long, etc. used in this file
//          DO NOT add complex types.
///////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef __VL_UTILITY_MACROS_H__
#define __VL_UTILITY_MACROS_H__
///////////////////////////////////////////////////////////////////////////////////////////////////
#define vlMax(a, b)   (((a)>(b))?(a):(b))
///////////////////////////////////////////////////////////////////////////////////////////////////
#define vlMin(a, b)   (((a)<(b))?(a):(b))
///////////////////////////////////////////////////////////////////////////////////////////////////
#define vlAbs(v)      (((v)<0)?-(v):(v))
///////////////////////////////////////////////////////////////////////////////////////////////////
#define     VL_BRING_TO_BOUNDS(val, minval, maxval)                 \
    {                                                               \
        (val) = vlMax(val, minval);                                 \
        (val) = vlMin(val, maxval);                                 \
    }

///////////////////////////////////////////////////////////////////////////////////////////////////
#define     VL_ARRAY_ITEM_COUNT(array)      ((sizeof(array))/(sizeof((array)[0])))
#define     VL_SAFE_ARRAY_INDEX(ix, array)  (vlMin(vlAbs(ix), VL_ARRAY_ITEM_COUNT(array)-1))
#define     SAFE_ARRAY_INDEX_UN(ix, array)  (vlMin(ix, VL_ARRAY_ITEM_COUNT(array)-1))

///////////////////////////////////////////////////////////////////////////////////////////////////
//#define     VL_ZERO_MEMORY(object)          (vlMemSet(&(object), 0, sizeof(object), sizeof(object)))
///////////////////////////////////////////////////////////////////////////////////////////////////
// use VL_ZERO_STRUCT if you are not including cMisc.h which contains vlMemSet
///////////////////////////////////////////////////////////////////////////////////////////////////
#define     VL_ZERO_STRUCT(object)          (memset(&(object), 0, sizeof(object)))

///////////////////////////////////////////////////////////////////////////////////////////////////
#define     VL_COPY_MEMORY(lhs, rhs)        (memcpy(&(lhs), &(rhs), sizeof(rhs), sizeof(lhs)))
///////////////////////////////////////////////////////////////////////////////////////////////////
// use VL_COPY_STRUCT if you are not including cMisc.h which contains memcpy
///////////////////////////////////////////////////////////////////////////////////////////////////
#define     VL_COPY_STRUCT(lhs, rhs)        (memmove(&(lhs), &(rhs), sizeof(lhs)))

///////////////////////////////////////////////////////////////////////////////////////////////////
#define     VL_COND_STR(cond, tcase, fcase)     ((cond)?(#tcase):(#fcase))
#define     VL_TRUE_COND_STR(cond)              VL_COND_STR(cond, TRUE   , FALSE )
#define     VL_SUCCESS_COND_STR(cond)           VL_COND_STR(cond, SUCCESS, FAILED)
#define     VL_ON_COND_STR(cond)                VL_COND_STR(cond, ON     , OFF   )
#define     VL_YES_COND_STR(cond)               VL_COND_STR(cond, YES    , NO    )
#define     VL_OK_COND_STR(cond)                VL_COND_STR(cond, OK     , NOT_OK)
///////////////////////////////////////////////////////////////////////////////////////////////////
#define VL_VALUE_1_FROM_ARRAY(pArray)                                   \
   (((unsigned char)((pArray)[0]&0xFF) <<  0) )

//----------------------------------------------------------------------------
#define VL_VALUE_2_FROM_ARRAY(pArray)                                   \
   (((unsigned char)((pArray)[0]&0xFF) <<  8) |                         \
    ((unsigned char)((pArray)[1]&0xFF) <<  0) )

//----------------------------------------------------------------------------
#define VL_VALUE_3_FROM_ARRAY(pArray)                                   \
   (((unsigned char)((pArray)[0]&0xFF) << 16) |                         \
    ((unsigned char)((pArray)[1]&0xFF) <<  8) |                         \
    ((unsigned char)((pArray)[2]&0xFF) <<  0) )

//----------------------------------------------------------------------------
#define VL_VALUE_4_FROM_ARRAY(pArray)                                   \
   (((unsigned char)((pArray)[0]&0xFF) << 24) |                         \
    ((unsigned char)((pArray)[1]&0xFF) << 16) |                         \
    ((unsigned char)((pArray)[2]&0xFF) <<  8) |                         \
    ((unsigned char)((pArray)[3]&0xFF) <<  0) )

//----------------------------------------------------------------------------
#define VL_VALUE_5_FROM_ARRAY(pArray)                                   \
   (((unsigned long long)((pArray)[0]&0xFF) << 32) |                    \
    ((unsigned long long)((pArray)[1]&0xFF) << 24) |                    \
    ((unsigned long long)((pArray)[2]&0xFF) << 16) |                    \
    ((unsigned long long)((pArray)[3]&0xFF) <<  8) |                    \
    ((unsigned long long)((pArray)[4]&0xFF) <<  0) )

//----------------------------------------------------------------------------
#define VL_VALUE_6_FROM_ARRAY(pArray)                                   \
   (((unsigned long long)((pArray)[0]&0xFF) << 40) |                    \
    ((unsigned long long)((pArray)[1]&0xFF) << 32) |                    \
    ((unsigned long long)((pArray)[2]&0xFF) << 24) |                    \
    ((unsigned long long)((pArray)[3]&0xFF) << 16) |                    \
    ((unsigned long long)((pArray)[4]&0xFF) <<  8) |                    \
    ((unsigned long long)((pArray)[5]&0xFF) <<  0) )

//----------------------------------------------------------------------------
#define VL_VALUE_7_FROM_ARRAY(pArray)                                   \
   (((unsigned long long)((pArray)[0]&0xFF) << 48) |                    \
    ((unsigned long long)((pArray)[1]&0xFF) << 40) |                    \
    ((unsigned long long)((pArray)[2]&0xFF) << 32) |                    \
    ((unsigned long long)((pArray)[3]&0xFF) << 24) |                    \
    ((unsigned long long)((pArray)[4]&0xFF) << 16) |                    \
    ((unsigned long long)((pArray)[5]&0xFF) <<  8) |                    \
    ((unsigned long long)((pArray)[6]&0xFF) <<  0) )

//----------------------------------------------------------------------------
#define VL_VALUE_8_FROM_ARRAY(pArray)                                   \
   (((unsigned long long)((pArray)[0]&0xFF) << 56) |                    \
    ((unsigned long long)((pArray)[1]&0xFF) << 48) |                    \
    ((unsigned long long)((pArray)[2]&0xFF) << 40) |                    \
    ((unsigned long long)((pArray)[3]&0xFF) << 32) |                    \
    ((unsigned long long)((pArray)[4]&0xFF) << 24) |                    \
    ((unsigned long long)((pArray)[5]&0xFF) << 16) |                    \
    ((unsigned long long)((pArray)[6]&0xFF) <<  8) |                    \
    ((unsigned long long)((pArray)[7]&0xFF) <<  0) )

//----------------------------------------------------------------------------
#define VL_PRINT_DEBUG_LINE()           printf("%s : %s() : %d\n", __FILE__, __FUNCTION__, __LINE__)
//----------------------------------------------------------------------------
#define VL_PRINT_SIZEOF(obj)    printf("\n%s: sizeof(%s) = \t%d\n", __FUNCTION__, #obj, sizeof(obj))

///////////////////////////////////////////////////////////////////////////////////////////////////
#endif //__VL_UTILITY_MACROS_H__
///////////////////////////////////////////////////////////////////////////////////////////////////
