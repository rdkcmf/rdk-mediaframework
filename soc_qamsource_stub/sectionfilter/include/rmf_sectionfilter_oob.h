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

///////////////////////////////////////////////////////////
//  rmf_SectionFilter_OOB.h
//  Implementation of the Class rmf_SectionFilter_OOB
//  Created on:      20-Jul-2012 12:35:04 PM
//  Original author: Sunil S
///////////////////////////////////////////////////////////

#if !defined __RMF_SECTIONFILTER_DSG_APPID__
#define __RMF_SECTIONFILTER_DSG_APPID__

#include "rmf_sectionfilter.h"

#define MAX_OOB_SESSION                         5
#define DSG_MAX_PID_FILTER              100
#define DSG_MAX_SECITON_FILTER          32


typedef enum tag_VL_DSG_CLIENT_ID_ENC_TYPE
{
    VL_DSG_CLIENT_ID_ENC_TYPE_NONE  = 0x00,
    VL_DSG_CLIENT_ID_ENC_TYPE_BCAST = 0x01,
    VL_DSG_CLIENT_ID_ENC_TYPE_WKMA  = 0x02,
    VL_DSG_CLIENT_ID_ENC_TYPE_CAS   = 0x03,
    VL_DSG_CLIENT_ID_ENC_TYPE_APP   = 0x04,

    VL_DSG_CLIENT_ID_ENC_TYPE_INVALID= 0xFF

}VL_DSG_CLIENT_ID_ENC_TYPE;

typedef struct tagPidList
{
        int                     PidListStatus;
        uint8_t                 pid;
}PidList;

typedef struct tagSectionFilterParams
{
        uint8_t* pos_mask;
        uint8_t* pos_value;
        uint16_t pos_length;
        uint8_t* neg_mask;
        uint8_t* neg_value;
        uint16_t neg_length;
}SectionFilterParams;
typedef struct tagSectionFilterInfo
{
        int                             SectionFilterHandle;
        SectionFilterParams             filterParams;
        int                             filterStatus;
//        vl_SectionFilter_t              *pSectionFilterInstance;
}SectionFilterInfo;
typedef struct tagDsgTunnelSession
{
        int                             m_dsgAppId;
        unsigned long                   m_nRegistrationId;
        PidList                         m_pidList[DSG_MAX_PID_FILTER];
//      unsigned short                  m_dsmccPid;
//      int                                     m_numFilters;           // if it become zero, kill the thread.
        SectionFilterInfo               m_sectionFilterList[DSG_MAX_SECITON_FILTER];
        rmf_osal_Mutex          m_sfListMutex;
//      int                                             m_stFilterCnt;
        rmf_osal_Mutex          m_pidListMutex;
       void*                            m_pData;

}DsgTunnelSession;


typedef struct DSG_SectionFilter_Info_s
{
        uint32_t                    m_ulPid;
        unsigned long                           m_nRegistrationId;
        unsigned long                           m_SectionFilterHandle;

}Dsg_SectionFilter_Info_t;


typedef int (*VL_DSG_CLIENT_CBFUNC_t)(unsigned long             nRegistrationId,
                                      unsigned long             nAppData,
                                      VL_DSG_CLIENT_ID_ENC_TYPE eClientType,
                                      unsigned long long        nClientId,
                                      unsigned long             nBytes,
                                      unsigned char *           pData);

class rmf_SectionFilter_OOB : public rmf_SectionFilter
{

public:
	rmf_SectionFilter_OOB(void* pFilterParm);
	virtual ~rmf_SectionFilter_OOB();

private:
	RMF_SECTFLT_RESULT Create(uint16_t pid, uint8_t* pos_mask, uint8_t* pos_value, uint16_t pos_length, uint8_t* neg_mask, uint8_t* neg_value, uint16_t neg_length, void **pSectionFilterInfo, uint32_t* pFilterID);
	RMF_SECTFLT_RESULT Start(uint32_t filterID);
	RMF_SECTFLT_RESULT Release(uint32_t filterID);
	RMF_SECTFLT_RESULT Stop(uint32_t filterID);
	RMF_SECTFLT_RESULT Read(uint32_t filterID, rmf_Section_Data** ppSectionData);

        static void OobClientThread(void *data);
       void OobClientThreadFn(void);

       static int stOobCallback(unsigned long RegistrationId,
                                unsigned long AppData,
                                VL_DSG_CLIENT_ID_ENC_TYPE eClientType,
                                unsigned long long        nClientId,
                                unsigned long             nBytes,
                                unsigned char*            pData);
        
       int stOobCallbackfn(unsigned long RegistrationId,
				unsigned long AppData,
				VL_DSG_CLIENT_ID_ENC_TYPE eClientType,
				unsigned long long        nClientId,
				unsigned long             nBytes,
				unsigned char*            pData);

	void AddTunnelSessionTotheList(DsgTunnelSession* pSession);
	DsgTunnelSession* getTunnelSessionForAppId(int appId);
	DsgTunnelSession* getTunnelSessionForClientId(int appId);
	void LockPidList(DsgTunnelSession* pDsgTunnelSession);
	void UnLockPidList(DsgTunnelSession* pDsgTunnelSession);
	void LocksfList(DsgTunnelSession* pDsgTunnelSession);
	void UnLocksfList(DsgTunnelSession* pDsgTunnelSession);

	void AddPidTotheList(uint8_t pid, unsigned long RegistrationId);
	void RemovePidFromTheList(uint8_t pid, unsigned long RegistrationId);
	bool IsARegisteredPid(uint8_t pid, unsigned long RegistrationId);

	void DispatchSectionData(unsigned char *pData, int dataLen, unsigned long RegistrationId);
	int sw_filter(unsigned char *pData, int dataLen, SectionFilterParams	*pFilterParams);

	RMF_SECTFLT_RESULT DSG_OpenTunnelFlow(Dsg_SectionFilter_Info_t *pDsgFilterInfo);
	RMF_SECTFLT_RESULT DSG_StartPidFilter(Dsg_SectionFilter_Info_t *pDsgFilterInfo, uint8_t pid);
	RMF_SECTFLT_RESULT DSG_AddFilter( Dsg_SectionFilter_Info_t *pDsgFilterInfo, uint8_t* pos_mask, uint8_t* pos_value, uint16_t pos_length,
					  uint8_t* neg_mask, uint8_t* neg_value,	uint16_t neg_length);
	RMF_SECTFLT_RESULT DSG_RemoveFilter(Dsg_SectionFilter_Info_t *pDsgFilterInfo);
	RMF_SECTFLT_RESULT DSG_StopPidFilter(Dsg_SectionFilter_Info_t *pDsgFilterInfo);
	RMF_SECTFLT_RESULT DSG_CloseTunnelFlow(Dsg_SectionFilter_Info_t *pDsgFilterInfo);

	
	unsigned long DsgRegisterClientSim(VL_DSG_CLIENT_CBFUNC_t    pfClientCB,
	                                  unsigned long             nAppData,
	                                  VL_DSG_CLIENT_ID_ENC_TYPE eClientType,
	                                  unsigned long long        nClientId);
	
	
	DsgTunnelSession* InitializeTunnel(void);
	
private:
	uint32_t appId;
	
	/* static */ DsgTunnelSession*  st_pDsgTunnelSessionList[MAX_OOB_SESSION];
	static VL_DSG_CLIENT_CBFUNC_t dsgCallback;
	static unsigned long dsgclientAppData;
	static unsigned long long       dsgClientId;
	static VL_DSG_CLIENT_ID_ENC_TYPE dsgeClientType;
        bool bOOBCopied;
        FILE *pOOBFile;
        static int nSect_Count;

};
#endif // !defined(__RMF_SECTIONFILTER_DSG_APPID__)
