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
#include "utilityMacros.h"
#include "bufParseUtilities.h"
#include "rdk_debug.h"
#include "traceStack.h"

#include "string.h"
#include "coreUtilityApi.h"
#include <stdio.h>
#define vlMin(a, b)   (((a)<(b))?(a):(b))

///////////////////////////////////////////////////////////////////////////////////////////////////
#define VL_BYTE_STREAM_LOG_ERROR(type, pStream, len)            \
    RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.POD", "%s: %s ERROR at: %s:%d:%s\n    for Stream: '%s', Created in %s:%d:%s\n    Size %d, Exhausted at %d, Remainder %d, Cannot %s %d Bytes.\n", \
    __FUNCTION__, #type,                                        \
    pszFile,                                                    \
    iLine,                                                      \
    pszFunc,                                                    \
    (pStream)->pszBufName,                                      \
    (pStream)->pszFile,                                         \
    (pStream)->iLine,                                           \
    (pStream)->pszFunction,                                     \
    (pStream)->nCapacity,                                       \
    (pStream)->iPosition,                                       \
    VL_BYTE_STREAM_REMAINDER(pStream),                          \
#type, (len));

///////////////////////////////////////////////////////////////////////////////////////////////////
unsigned long  vlParseCcApduLengthFieldWorker(VL_BYTE_STREAM * pStream, const char * pszFile, int iLine, const char * pszFunc)
{
    int nBytes = 1;
    if(VL_BYTE_STREAM_CAN_USE(pStream, nBytes))
    {
        unsigned char * pBuf = VL_BYTE_STREAM_GET_CURRENT_BUF(pStream);
        int bSizeIndicator = pBuf[0]>>7;
        int nSize = ((pBuf[0])&0x7F);
        VL_BYTE_STREAM_ADVANCE_BUF(pStream, nBytes);

        if(0 == bSizeIndicator)
        {
            // single-byte encoding
            return nSize;
        }

        // multi-byte encoding
        return vlParseNByteLongWorker(pStream, nSize, pszFile, iLine, pszFunc);
    }
    else
    {
        VL_BYTE_STREAM_LOG_PARSE_ERROR(pStream, nBytes);
        VL_BYTE_STREAM_ADVANCE_BUF(pStream, nBytes);
    }
    return 0;
}

#define VL_IMPL_READ_N_BYTE_LONG(N)                                     \
{                                                                       \
    if(VL_BYTE_STREAM_CAN_USE(pStream, (N)))                            \
    {                                                                   \
        unsigned char * pBuf = VL_BYTE_STREAM_GET_CURRENT_BUF(pStream); \
        pStream->iPosition += (N);                                      \
        return VL_VALUE_##N##_FROM_ARRAY(pBuf);                         \
    }                                                                   \
    else                                                                \
    {                                                                   \
        VL_BYTE_STREAM_LOG_PARSE_ERROR(pStream, (N));                   \
        VL_BYTE_STREAM_ADVANCE_BUF(pStream, (N));                       \
    }                                                                   \
    return 0;                                                           \
}

unsigned char  vlParseByteWorker(VL_BYTE_STREAM * pStream, const char * pszFile, int iLine, const char * pszFunc)
    VL_IMPL_READ_N_BYTE_LONG(1)

unsigned short vlParseShortWorker(VL_BYTE_STREAM * pStream, const char * pszFile, int iLine, const char * pszFunc)
    VL_IMPL_READ_N_BYTE_LONG(2)

unsigned long  vlParse3ByteLongWorker(VL_BYTE_STREAM * pStream, const char * pszFile, int iLine, const char * pszFunc)
    VL_IMPL_READ_N_BYTE_LONG(3)

unsigned long  vlParseLongWorker(VL_BYTE_STREAM * pStream, const char * pszFile, int iLine, const char * pszFunc)
    VL_IMPL_READ_N_BYTE_LONG(4)

#define VL_IMPL_PARSE_N_BYTE_LONG(N)                                    \
unsigned long long vlParse##N##ByteLongWorker(VL_BYTE_STREAM * pStream, const char * pszFile, int iLine, const char * pszFunc)       \
    VL_IMPL_READ_N_BYTE_LONG(N)

VL_IMPL_PARSE_N_BYTE_LONG(5)
VL_IMPL_PARSE_N_BYTE_LONG(6)
VL_IMPL_PARSE_N_BYTE_LONG(7)

unsigned long long vlParseLongLongWorker(VL_BYTE_STREAM * pStream, const char * pszFile, int iLine, const char * pszFunc)
    VL_IMPL_READ_N_BYTE_LONG(8)

unsigned long long vlParseNByteLongWorker(VL_BYTE_STREAM * pStream, int nBytes, const char * pszFile, int iLine, const char * pszFunc)
{
    if(VL_BYTE_STREAM_CAN_USE(pStream, nBytes))
    {
        unsigned char * pBuf = VL_BYTE_STREAM_GET_CURRENT_BUF(pStream);
        int i = 0; unsigned long long val = 0;
        VL_BYTE_STREAM_ADVANCE_BUF(pStream, nBytes);

        for(i = 0; i < nBytes; i++)
        {
            val = (val<<8) | pBuf[i];
        }

        return val;
    }
    else
    {
        VL_BYTE_STREAM_LOG_PARSE_ERROR(pStream, nBytes);
        VL_BYTE_STREAM_ADVANCE_BUF(pStream, nBytes);
    }
    return 0;
}

void vlParseBufferWorker(VL_BYTE_STREAM * pStream, unsigned char * pResult, int nBytes, int nBufCapacity, const char * pszFile, int iLine, const char * pszFunc)
{
    if(!VL_BYTE_STREAM_CAN_USE(pStream, nBytes))
    {
        VL_BYTE_STREAM_LOG_PARSE_ERROR(pStream, nBytes);
    }
    
    // special case for buffer parsing: allow the parsing to progress
    nBytes = VL_BYTE_STREAM_SMALLER(pStream, nBytes);
    {
        unsigned char * pBuf = VL_BYTE_STREAM_GET_CURRENT_BUF(pStream);
        VL_BYTE_STREAM_ADVANCE_BUF(pStream, nBytes);
    
        if(NULL != pResult)
        {
            memcpy(pResult, pBuf, nBytes);
        }
    }
}

int vlWriteCcApduLengthFieldWorker(VL_BYTE_STREAM * pStream, unsigned long nLength, const char * pszFile, int iLine, const char * pszFunc)
{
    int nBytes = 1;
    if(VL_BYTE_STREAM_CAN_USE(pStream, nBytes))
    {
        unsigned char * pBuf = VL_BYTE_STREAM_GET_CURRENT_BUF(pStream);
        VL_BYTE_STREAM_ADVANCE_BUF(pStream, nBytes);

        if(nLength <= 127)
        {
            // single-byte encoding
            pBuf[0] = (unsigned char)nLength;
            // return number of bytes returned
            return nBytes;
        }
        else
        {
            // multi-byte encoding
            int nSize = 0, i = 0;
            // find number of bytes needed
            for(i = 0; i < 4; i++)
            {
                // check if byte needs encoding
                if((nLength>>(i*8))&0xFF)
                {
                    // register the new number of bytes required for encoding
                    nSize = i+1;
                }
            }

            // indicate multi-byte encoding
            pBuf[0] = (unsigned char)(0x80 | nSize);
            // write bytes for encoding length
            nBytes += vlWriteNByteLongWorker(pStream, nSize, nLength, pszFile, iLine, pszFunc);
        }

        // return number of bytes written
        return nBytes;
    }
    else
    {
        VL_BYTE_STREAM_LOG_WRITE_ERROR(pStream, nBytes);
        VL_BYTE_STREAM_ADVANCE_BUF(pStream, nBytes);
    }
    return 0;
}

int vlWriteNByteLongWorker(VL_BYTE_STREAM * pStream, int nBytes, unsigned long long nLongLong, const char * pszFile, int iLine, const char * pszFunc)
{
    if(VL_BYTE_STREAM_CAN_USE(pStream, nBytes))
    {
        unsigned char * pBuf = VL_BYTE_STREAM_GET_CURRENT_BUF(pStream);
        int i = 0;
        VL_BYTE_STREAM_ADVANCE_BUF(pStream, nBytes);

        for(i = 0; i < nBytes; i++)
        {
            pBuf[i] = (unsigned char)((nLongLong>>(8*(nBytes-i-1)))&0xFF);
        }

        // return number of bytes written
        return nBytes;
    }
    else
    {
        VL_BYTE_STREAM_LOG_WRITE_ERROR(pStream, nBytes);
        VL_BYTE_STREAM_ADVANCE_BUF(pStream, nBytes);
    }
    return 0;
}

int vlWriteBufferWorker(VL_BYTE_STREAM * pStream, unsigned char * pBuffer, int nBytes, int nBufCapacity, const char * pszFile, int iLine, const char * pszFunc)
{
    if(VL_BYTE_STREAM_CAN_USE(pStream, nBytes))
    {
        unsigned char * pBuf = VL_BYTE_STREAM_GET_CURRENT_BUF(pStream);
        VL_BYTE_STREAM_ADVANCE_BUF(pStream, nBytes);

        if(NULL != pBuffer)
        {
            memcpy(pBuf, pBuffer, nBytes);
        }
        else
        {
            memset(pBuf, 0, nBytes);
        }

        // return number of bytes written
        return vlMin(nBytes, nBufCapacity);
    }
    else
    {
        VL_BYTE_STREAM_LOG_WRITE_ERROR(pStream, nBytes);
        VL_BYTE_STREAM_ADVANCE_BUF(pStream, nBytes);
    }
    return 0;
}



void vlStrNSet(char * pszStr, char ch, int nLength)
{
    /* terminate the string */
    pszStr[nLength] = ('\0');

    /* start from last index */
    do
    {
        /* decrement to index */
        --nLength;
        /* set element */
        pszStr[nLength] = ch;
    }
    while(nLength > 0); /* until done */
}
/****************************************************************************/
void vlTraceStack(int nLevel, VL_TRACE_STACK_TAG_TYPES nTagType, const char * lpszTag, const char * lpszFormat, ...)
{
    /* static declarations */
    enum { STACK_TRACE_MAX_MSG = 1000, STACK_TRACE_MAX_DEPTH = 1000, STACK_TRACE_INDENT = 2, STACK_TRACE_MAX_TAG = 50};
    static char m_strStackTraceBuffer[STACK_TRACE_MAX_DEPTH*STACK_TRACE_INDENT + STACK_TRACE_MAX_MSG + 10];
    static char m_strStackTraceTag[STACK_TRACE_MAX_MSG+10];
    static int m_nPrevLevel = 0;

    /* check for buffer over-flow */
    if((VL_TRACE_STACK_TAG_END != nTagType) && (nLevel >= 0) && (nLevel < STACK_TRACE_MAX_DEPTH))
    {
        va_list args;
        /* start variable arg list */
        va_start(args, lpszFormat);
        /* copy the tag */
        strncpy(m_strStackTraceTag, lpszTag, STACK_TRACE_MAX_TAG);
        /* terminate the tag */
        m_strStackTraceTag[STACK_TRACE_MAX_TAG] = ('\0');
        /* initialize buffer */
        vlStrNSet(m_strStackTraceBuffer, (' '), nLevel*STACK_TRACE_INDENT);
        /* opening ? */
        if(VL_TRACE_STACK_TAG_BEGIN == nTagType)
        {
            /* open the tag */
            strcpy(m_strStackTraceBuffer + nLevel*STACK_TRACE_INDENT, ("{-->> "));
            strcpy(m_strStackTraceBuffer + nLevel*STACK_TRACE_INDENT, m_strStackTraceTag);
        }

        /* add a space */
        strcat(m_strStackTraceBuffer + nLevel*STACK_TRACE_INDENT, (" "));
        /* print the arguments */
        vsnprintf(m_strStackTraceBuffer + strlen(m_strStackTraceBuffer), sizeof(m_strStackTraceBuffer) - strlen(m_strStackTraceBuffer), lpszFormat, args);
        /* print the buffer */
        strcat(m_strStackTraceBuffer, "\n");
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", m_strStackTraceBuffer);
        /* terminate the variable arg list */
        va_end(args);

        /* register current level */
        m_nPrevLevel = nLevel;
    }

    /* check for buffer over-flow */
    if((VL_TRACE_STACK_TAG_END == nTagType) && (m_nPrevLevel >= 0) && (m_nPrevLevel < STACK_TRACE_MAX_DEPTH))
    {
        /* initialize buffer */
        vlStrNSet(m_strStackTraceBuffer, (' '), m_nPrevLevel*STACK_TRACE_INDENT);
        /* closing ? */
        if(VL_TRACE_STACK_TAG_END == nTagType)
        {
            /* close the tag */
            strcpy(m_strStackTraceBuffer + m_nPrevLevel*STACK_TRACE_INDENT, ("}<<-- "));
            strcat(m_strStackTraceBuffer + m_nPrevLevel*STACK_TRACE_INDENT, lpszTag);
        }
        /* print the buffer */
        strcat(m_strStackTraceBuffer, "\n");
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", m_strStackTraceBuffer);
    }

}

/****************************************************************************/
/* trimming generic implementation(commented) to the specific conversion( just below ) */
//#define ToHex(c) ((c) <= 9) ? ((c) + 48) : (((c) <=15)? ((c) + 55): c)
#define ToHex(c) ((c) <= 9) ? ((c) + 48) : ((c) + 55)

/****************************************************************************/
char * vlPrintBytesToString(int             nBytes,
                             const char  *   pBytes,
                             int             nStrSize,
                             char       *   pszString)
{
    /* initialize index */
    int ix = 0;

    /* add the prefix */
    strcpy(pszString, ("0x"));

    /* for each char until string limit */
    for(ix = 0; (ix < nBytes) && (ix*2 < (nStrSize-3)); ix++)
    {
        /* get character */
        char c = pBytes[ix];

        /* translate character */
        pszString[ix * 2 + 2] = ToHex(((c >> 4) & 0x0F));
        pszString[ix * 2 + 3] = ToHex(((c >> 0) & 0x0F));
    }

    /* terminate string */
    pszString[ix * 2 + 2] = ('\0');

    /* return string */
    return pszString;
}

#if 0
char * vlPrintLongToString(int             nBytes,
                           unsigned long long nLong,
                           int             nStrSize,
                           char      *    pszString)
{
    /* initialize index */
    int ix = 0;

    /* add the prefix */
    strcpy(pszString, ("0x"));

    /* for each char until string limit */
    for(ix = 0; (ix < nBytes) && (ix < sizeof(nLong)) && (ix*2 < (nStrSize-3)); ix++)
    {
        /* get character */
        char c = ((nLong>>((nBytes-(ix+1))*8))&0xFF);

        /* translate character */
        pszString[ix * 2 + 2] = ToHex(((c >> 4) & 0x0F));
        pszString[ix * 2 + 3] = ToHex(((c >> 0) & 0x0F));
    }

    /* terminate string */
    pszString[ix * 2 + 2] = ('\0');

    /* return string */
    return pszString;
}

/****************************************************************************/
char * vlPrintBytesToStringA(int             nBytes,
                             const char  *   pBytes,
                             int             nStrSize,
                             char        *   pszString)
{
    /* initialize index */
    int ix = 0;

    /* add the prefix */
    strcpy(pszString, "0x");

    /* for each char until string limit */
    for(ix = 0; (ix < nBytes) && (ix*2 < (nStrSize-3)); ix++)
    {
        /* get character */
        char c = pBytes[ix];

        /* translate character */
        pszString[ix * 2 + 2] = ToHex(((c >> 4) & 0x0F));
        pszString[ix * 2 + 3] = ToHex(((c >> 0) & 0x0F));
    }

    /* terminate string */
    pszString[ix * 2 + 2] = '\0';

    /* return string */
    return pszString;
}
#endif





void vlMpeosDumpBuffer(rdk_LogLevel lvl, const char *mod, const void * pBuffer, int nBufSize)
{
    int nPrinted = 0;
    if(!rdk_dbg_enabled(mod, lvl)) return; // Feb-25-2015: PXG2V2-436 : Fix for unwanted code-execution.

    // Changed May-06-2008: Changed to include text
    while(nPrinted < nBufSize)
    {
        // Changed May-06-2008: Changed to include text
        int i = 0; int iLoc = 0;
        #define VL_DUMP_BUFFER_CHARS       16
        #define VL_DUMP_BUFFER_CHAR(ch)    ((((ch)>=32) && ((ch)<=126) && ('%' != ch))?(ch):'.')
        // Changed May-06-2008: Changed to include text
        const unsigned char * buf = ((const unsigned char*)pBuffer) + nPrinted;
        char szFormat[VL_DUMP_BUFFER_CHARS * 10 + 10];
        int nToPrint = vlMin(VL_DUMP_BUFFER_CHARS, nBufSize - nPrinted);

        // Changed May-06-2008: Changed to include text
        iLoc = 0;
        snprintf(szFormat+iLoc, sizeof(szFormat)-iLoc, "Buf: 0x%04x :", nPrinted);

        // Changed May-06-2008: Changed to include text
        for(i = 0; i < nToPrint; i++)
        {
            iLoc = 13+i*3;
            snprintf(szFormat+iLoc, sizeof(szFormat)-iLoc, " %02x", buf[i]);
        }

        // Changed May-06-2008: Changed to include text
        for(i = 0; i < VL_DUMP_BUFFER_CHARS-nToPrint; i++)
        {
            iLoc = 13+(nToPrint+i)*3;
            snprintf(szFormat+iLoc, sizeof(szFormat)-iLoc, "   ");
        }

        // Changed May-06-2008: Changed to include text
        iLoc = 13+(VL_DUMP_BUFFER_CHARS)*3;
        snprintf(szFormat+iLoc, sizeof(szFormat)-iLoc, "  ");

        // Changed May-06-2008: Changed to include text
        for(i = 0; i < nToPrint; i++)
        {
            iLoc = 13+(VL_DUMP_BUFFER_CHARS)*3+2+i;
            snprintf(szFormat+iLoc, sizeof(szFormat)-iLoc,  "%c", VL_DUMP_BUFFER_CHAR(buf[i]));
        }

        // Changed May-06-2008: Changed to include text
        iLoc = 13+(VL_DUMP_BUFFER_CHARS)*3+2+nToPrint;
        snprintf(szFormat+iLoc, sizeof(szFormat)-iLoc, "\n");

        // Changed May-06-2008: Changed to include text
        RDK_LOG(lvl, mod, szFormat);

        // Changed May-06-2008: Changed to include text
        nPrinted += VL_DUMP_BUFFER_CHARS;
    }

}
