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



#ifndef _MMI_MAIN_H_
#define _MMI_MAIN_H_

#define MAX_OPEN_SESSIONS 1
#define MAX_OPEN_DLGS 1

#ifdef __cplusplus
extern "C" {
#endif

// State machine
typedef enum _MMIStateType {
    MMI_UNINITIALIZED = 0,
    MMI_INITIALIZED,
    MMI_STARTED,
    MMI_SESSION_OPEN_REQUESTED,
    MMI_SESSION_OPENED,
    MMI_DLG_OPEN_REQUESTED,
    MMI_DLG_OPENED,
} MMIStateType;

typedef struct _MMIState {
    MMIStateType state;
    int loop;
    int dlgId; //used as busy flag; initially set to -1, during dlg req 0
    int openSessions;
    unsigned short sessionId;
    pthread_mutex_t lock;
} MMIState;

// Externs
extern unsigned long RM_Get_Ver( unsigned long id );

// Prototypes
void mmiProc(void*);
static int _mmiSndLogReq(int level, const char* msg);
static int mmiSetState(MMIStateType st);
static int mmiSndEvent(SYSTEM_EVENTS, unsigned char *, unsigned short,
                        unsigned int x, unsigned int y, unsigned int z);
static void mmiOpenSessionReq( ULONG resId, UCHAR tcId );
static void mmiSessionOpened( unsigned short sessNbr, unsigned long resId, unsigned long tcId );
static void mmiSessionClosed( unsigned long sessNbr );
static void mmiDlgOpenReq(unsigned short, void *, unsigned long);
static void mmiDlgOpenCnf(unsigned char, unsigned char);
static void mmiDlgCloseReq(unsigned short, void *, unsigned long);
static void mmiDlgCloseCnf(unsigned short dlgNum);
/*static */int mmiSndApdu(unsigned long tag, unsigned short dataLen, unsigned char *data);


#ifdef __cplusplus
}
#endif

#endif //_MMI_MAIN_H_

