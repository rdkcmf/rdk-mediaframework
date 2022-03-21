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
#ifndef __VL_BUF_PARSE_UTILITIES_H__
#define __VL_BUF_PARSE_UTILITIES_H__

#include "rdk_debug.h"
#include "string.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus

//-----------------------------------------------------------------------------------------------//
#define VL_SBS_PARSE_MEMBER(bs, member, bits)           member = bs.get_bits (bits);

//-----------------------------------------------------------------------------------------------//
#define VL_SBS_SKIP_MEMBER(bs, member, bits)                     bs.skip_bits(bits);

//-----------------------------------------------------------------------------------------------//
#endif // __cplusplus

///////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct tag_VL_BYTE_STREAM
{
	int nCapacity;
	int iPosition;
	int iLine;
	const char * pszBufName;
	const char * pszFile;
	const char * pszFunction;
	unsigned char * pBuffer;
}VL_BYTE_STREAM;

///////////////////////////////////////////////////////////////////////////////////////////////////
#define VL_BYTE_STREAM_INST(name, stream, buf, len)                                 \
    VL_BYTE_STREAM stream =                                                         \
    { len, 0, __LINE__, #name, __FILE__, __FUNCTION__, ((unsigned char *)(buf)) };  \
    VL_BYTE_STREAM * p##stream = &(stream)

//-----------------------------------------------------------------------------------------------//
#define VL_BYTE_STREAM_PARENT_CHILD_INST(name, child_stream, parent_stream, len)   \
    VL_BYTE_STREAM_INST(name, child_stream, VL_BYTE_STREAM_GET_CURRENT_BUF(parent_stream), len);                                  \
    VL_BYTE_STREAM_ADVANCE_BUF(parent_stream, len)

//-----------------------------------------------------------------------------------------------//
#define VL_BYTE_STREAM_ARRAY_INST(array, stream)                \
    VL_BYTE_STREAM_INST(array, stream, ((unsigned char *)(array)), sizeof(array))

//-----------------------------------------------------------------------------------------------//
#define VL_BYTE_STREAM_OBJECT_INST(obj, stream)                 \
    VL_BYTE_STREAM_INST(obj, stream, ((unsigned char *)(&(obj))), sizeof(obj))

//-----------------------------------------------------------------------------------------------//
#define VL_BYTE_STREAM_CAN_USE(pStream, len)    ((((pStream)->iPosition)+len) <= (pStream)->nCapacity)

//-----------------------------------------------------------------------------------------------//
#define VL_BYTE_STREAM_CAPACITY(pStream)	    ((pStream)->nCapacity)

//-----------------------------------------------------------------------------------------------//
#define VL_BYTE_STREAM_USED(pStream)	        ((pStream)->iPosition)

//-----------------------------------------------------------------------------------------------//
#define VL_BYTE_STREAM_REMAINDER(pStream)	    (((pStream)->nCapacity) - ((pStream)->iPosition))

//-----------------------------------------------------------------------------------------------//
#define VL_BYTE_STREAM_SMALLER(pStream, len)    (((len) <= VL_BYTE_STREAM_REMAINDER(pStream))?(len):VL_BYTE_STREAM_REMAINDER(pStream))

//-----------------------------------------------------------------------------------------------//
#define VL_BYTE_STREAM_GET_CURRENT_BUF(pStream)                 \
    ((pStream)->pBuffer + (pStream)->iPosition)

//-----------------------------------------------------------------------------------------------//
#define VL_BYTE_STREAM_ADVANCE_BUF(pStream, len)                \
    (pStream)->iPosition+=(len)

//-----------------------------------------------------------------------------------------------//
#define VL_BYTE_STREAM_LOG_PARSE_ERROR(pStream, len)            \
    VL_BYTE_STREAM_LOG_ERROR(Parse, (pStream), (len))
//-----------------------------------------------------------------------------------------------//
#define VL_BYTE_STREAM_LOG_WRITE_ERROR(pStream, len)            \
    VL_BYTE_STREAM_LOG_ERROR(Write, (pStream), (len))

///////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
extern "C" {
#endif
    ///////////////////////////////////////////////////////////////////////////////////////////////

    /* parses and extracts the specified value from the buffer */
    /* returns the extracted value */
    unsigned long  vlParseCcApduLengthFieldWorker(VL_BYTE_STREAM * pStream, const char * pszFile, int iLine, const char * pszFunc);
    unsigned char  vlParseByteWorker(VL_BYTE_STREAM * pStream, const char * pszFile, int iLine, const char * pszFunc);
    unsigned short vlParseShortWorker(VL_BYTE_STREAM * pStream, const char * pszFile, int iLine, const char * pszFunc);
    unsigned long  vlParse3ByteLongWorker(VL_BYTE_STREAM * pStream, const char * pszFile, int iLine, const char * pszFunc);
    unsigned long  vlParseLongWorker(VL_BYTE_STREAM * pStream, const char * pszFile, int iLine, const char * pszFunc);
    unsigned long long vlParse5ByteLongWorker(VL_BYTE_STREAM * pStream, const char * pszFile, int iLine, const char * pszFunc);
    unsigned long long vlParse6ByteLongWorker(VL_BYTE_STREAM * pStream, const char * pszFile, int iLine, const char * pszFunc);
    unsigned long long vlParse7ByteLongWorker(VL_BYTE_STREAM * pStream, const char * pszFile, int iLine, const char * pszFunc);
    unsigned long long vlParseLongLongWorker(VL_BYTE_STREAM * pStream, const char * pszFile, int iLine, const char * pszFunc);
    unsigned long long vlParseNByteLongWorker(VL_BYTE_STREAM * pStream, int nBytes, const char * pszFile, int iLine, const char * pszFunc);
    void           vlParseBufferWorker(VL_BYTE_STREAM * pStream, unsigned char * pResult, int nBytes, int nBufCapacity, const char * pszFile, int iLine, const char * pszFunc);

    /* writes the specified value to the buffer */
    /* returns number of bytes written */
    int vlWriteCcApduLengthFieldWorker(VL_BYTE_STREAM * pStream, unsigned long nLength, const char * pszFile, int iLine, const char * pszFunc);
    int vlWriteNByteLongWorker(VL_BYTE_STREAM * pStream, int nBytes, unsigned long long nLongLong, const char * pszFile, int iLine, const char * pszFunc);
    int vlWriteBufferWorker(VL_BYTE_STREAM * pStream, unsigned char * pBuffer, int nBytes, int nBufCapacity, const char * pszFile, int iLine, const char * pszFunc);

    ///////////////////////////////////////////////////////////////////////////////////////////////
#define vlParseCcApduLengthField(pStream)   vlParseCcApduLengthFieldWorker((pStream), __FILE__, __LINE__, __FUNCTION__)
#define vlParseByte(pStream)                vlParseByteWorker((pStream), __FILE__, __LINE__, __FUNCTION__)
#define vlParseShort(pStream)               vlParseShortWorker((pStream), __FILE__, __LINE__, __FUNCTION__)
#define vlParse3ByteLong(pStream)           vlParse3ByteLongWorker((pStream), __FILE__, __LINE__, __FUNCTION__)
#define vlParseLong(pStream)                vlParseLongWorker((pStream), __FILE__, __LINE__, __FUNCTION__)
#define vlParse5ByteLong(pStream)           vlParse5ByteLongWorker((pStream), __FILE__, __LINE__, __FUNCTION__)
#define vlParse6ByteLong(pStream)           vlParse6ByteLongWorker((pStream), __FILE__, __LINE__, __FUNCTION__)
#define vlParse7ByteLong(pStream)           vlParse7ByteLongWorker((pStream), __FILE__, __LINE__, __FUNCTION__)
#define vlParseLongLong(pStream)            vlParseLongLongWorker((pStream), __FILE__, __LINE__, __FUNCTION__)
#define vlParseNByteLong(pStream, nBytes)   vlParseNByteLongWorker((pStream), (nBytes), __FILE__, __LINE__, __FUNCTION__)
#define vlParseBuffer(pStream, pResult, nBytes, nBufCapacity) vlParseBufferWorker((pStream), (pResult), (nBytes), (nBufCapacity), __FILE__, __LINE__, __FUNCTION__)

    /* writes the specified value to the buffer */
    /* returns number of bytes written */
#define vlWriteCcApduLengthField(pStream, nLength)      vlWriteCcApduLengthFieldWorker((pStream), (nLength), __FILE__, __LINE__, __FUNCTION__)
#define vlWriteByte(pStream, nLongLong)                 vlWriteNByteLongWorker((pStream), 1, (nLongLong), __FILE__, __LINE__, __FUNCTION__)
#define vlWriteShort(pStream, nLongLong)                vlWriteNByteLongWorker((pStream), 2, (nLongLong), __FILE__, __LINE__, __FUNCTION__)
#define vlWrite3ByteLong(pStream, nLongLong)            vlWriteNByteLongWorker((pStream), 3, (nLongLong), __FILE__, __LINE__, __FUNCTION__)
#define vlWriteLong(pStream, nLongLong)                 vlWriteNByteLongWorker((pStream), 4, (nLongLong), __FILE__, __LINE__, __FUNCTION__)
#define vlWrite6ByteLong(pStream, nLongLong)            vlWriteNByteLongWorker((pStream), 6, (nLongLong), __FILE__, __LINE__, __FUNCTION__)
#define vlWriteNByteLong(pStream, nBytes, nLongLong)    vlWriteNByteLongWorker((pStream), (nBytes), (nLongLong), __FILE__, __LINE__, __FUNCTION__)
#define vlWriteBuffer(pStream, pBuffer, nBytes, nBufCapacity) vlWriteBufferWorker((pStream), (pBuffer), (nBytes), (nBufCapacity), __FILE__, __LINE__, __FUNCTION__)
    ///////////////////////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
}
#endif
///////////////////////////////////////////////////////////////////////////////////////////////////
#endif //__VL_BUF_PARSE_UTILITIES_H__
///////////////////////////////////////////////////////////////////////////////////////////////////
