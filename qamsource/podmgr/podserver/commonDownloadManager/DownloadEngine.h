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


#ifndef ComDownLEngine_H_
#define ComDownLEngine_H_
#include "cmThreadBase.h"
#include "tablecvt.h"
#include "vl_cdl_types.h"

//forward declaration
class CDataCarouselExtractor;

typedef unsigned long IOResourceHandle;

typedef enum
{
    DOWNLOAD_ENGINE_ERROR_TUNE = 0x100,
    DOWNLOAD_ENGINE_ERROR_ATTACH,
    DOWNLOAD_ENGINE_ERROR_CAROUSEL_READ_NOIMAGE,
    DOWNLOAD_ENGINE_ERROR_TFTP_SETUP,
    DOWNLOAD_ENGINE_ERROR_TFTP_READ
}
DOWLOAD_ENGINE_DWLD_ERROR;

class ComDownLEngine : public CMThread
{
public:
    ComDownLEngine();
    ComDownLEngine(int flag);
    ~ComDownLEngine();
    int initialize(void);
    static void OnInbandTuneComplete(int tuneStatus, IOResourceHandle hIn);
    static void TestDataCarousel(IOResourceHandle hIn, int freq, int program_num, int PID, unsigned char *pFileName);
    static void cdlcb_lock();
    static void cdlcb_unlock();
    void   run();

    static VL_CDL_IMAGE *m_pCDLImage;

private:
    int TuneToInbandChannel(IOResourceHandle *p_hIn, DownLEngine_TuneEventData* pEventData);
    int processCVT_t1v1Download(TableCVT *pCVT1);
    int processCVT_t2vxDownload(TableCVT *pCVT2);
    void dispatchDownLoadDoneEvent(int status, char *pData, unsigned long dataSize,int flag /*for inband*/);
    void dispatchDownLoadCompleteEventToJava();

    void dispatchDownLoadFailureEvent(int erroCode,int flag/*flag for inband*/);
    void dispatchDownLoadStartEvent();
    void dispatchCvt_T2_V2CDLEvent(TableCVT *pCVT);
    bool GetTFTPImageDetails(VL_CDL_IMAGE *pCDLImage, VL_CDL_DOWNLOAD_PARAMS* p_cdlFtpParams, 
                            char **ppFileName, int &FileNameLength, int count);
    
    int GetInbandSpec(int location_type, location_data_t &loc_data, int &freq_vector,  
                                  int &modulation_type,int &program_num, int &PID);

    void GetDownLoadInfoFromCVTData(cvtdownload_data_t *pCVT_Data, int protocol_version, int objCnt, int &downloadtype, 
                                                int &location_type, cvt2v2_object_type_e &objectType,
                                                location_data_t &loc_data, codefile_data_t &codeFileData);

    int UpdateCVT_t2v2_OnSuccess(TableCVT *pCVT2, int objCnt, char* pBuffer, int bufSize);
    
    int DoFTPDownLoad(location_data_t   data, int protocol_version, 
                                  codefile_data_t    codeFileData, cvt2v2_object_type_e objectType,
                                  TableCVT *pCVT2, cvtdownload_data_t *pCVT2_Data, int objCnt, char **ppFileName, int &nFileNameLength);

    rmf_osal_eventqueue_handle_t event_queue;
    CDataCarouselExtractor *m_pDataCarousel;
    static long  m_TuneEventsem;
    static int   m_tuneStatus;
    static long  m_tftpCbMutex;

};

#endif //_CDownloadEngine_H_
