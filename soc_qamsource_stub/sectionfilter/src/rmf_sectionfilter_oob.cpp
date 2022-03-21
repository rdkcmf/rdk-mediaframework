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
//  rmf_SectionFilter_OOB.cpp
//  Implementation of the Class rmf_SectionFilter_OOB
//  Created on:      20-Jul-2012 12:35:05 PM
//  Original author: Sunil S
///////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <assert.h>
#include <rdk_debug.h>

#include "include/rmf_sectionfilter_oob.h"

#define UNUSED_VAR(v) ((void)v)

// the port client will be connecting to
#define PORT 1234

// max number of bytes we can get at once
#define MAXDATASIZE 1024

#define  BLOCK_SIZE 1050

#define DEBUG_WARN printf
#define DEBUG_ERROR printf
#define DEBUG_INFO

#define DEFAULT_IP "127.0.0.1"

rmf_SectionFilter_OOB::rmf_SectionFilter_OOB(void* pFilterParam):rmf_SectionFilter(pFilterParam)
{
}

rmf_SectionFilter_OOB::~rmf_SectionFilter_OOB(){
}

void rmf_SectionFilter_OOB::OobClientThread(void *data)
{
    UNUSED_VAR(data);
    return;
}

void rmf_SectionFilter_OOB::OobClientThreadFn()
{
    return; //RegistrationId;
}

unsigned long rmf_SectionFilter_OOB::DsgRegisterClientSim(VL_DSG_CLIENT_CBFUNC_t pfClientCB,
                                  unsigned long       nAppData,
                                  VL_DSG_CLIENT_ID_ENC_TYPE eClientType,
                                  unsigned long long  nClientId)

{
    UNUSED_VAR(pfClientCB);
    UNUSED_VAR(eClientType);
    UNUSED_VAR(nAppData);
    UNUSED_VAR(nClientId);

    return 0;
}

void rmf_SectionFilter_OOB::AddPidTotheList(uint8_t pid, unsigned long RegistrationId)
{
    UNUSED_VAR(pid);
    UNUSED_VAR(RegistrationId);

    return;
}


void rmf_SectionFilter_OOB::RemovePidFromTheList(uint8_t pid, unsigned long RegistrationId)
{
    UNUSED_VAR(pid);
    UNUSED_VAR(RegistrationId);

    return;
}

void rmf_SectionFilter_OOB::AddTunnelSessionTotheList(DsgTunnelSession* pSession)
{
    UNUSED_VAR(pSession);

    return; 
}


DsgTunnelSession* rmf_SectionFilter_OOB::getTunnelSessionForAppId(int appId)
{
    UNUSED_VAR(appId);

    return NULL;    
}

DsgTunnelSession* rmf_SectionFilter_OOB::getTunnelSessionForClientId(int clientId)
{
    UNUSED_VAR(clientId);

    return NULL;
}

DsgTunnelSession* rmf_SectionFilter_OOB::InitializeTunnel()
{
    return NULL;
}

bool rmf_SectionFilter_OOB::IsARegisteredPid(uint8_t pid, unsigned long RegistrationId)
{

    UNUSED_VAR(pid);
    UNUSED_VAR(RegistrationId);

    return true;
}

int rmf_SectionFilter_OOB::sw_filter(unsigned char *pData, int dataLen, SectionFilterParams     *pFilterParams)
{
    UNUSED_VAR(pData);
    UNUSED_VAR(dataLen);
    UNUSED_VAR(pFilterParams);

    return true;
}

void rmf_SectionFilter_OOB::DispatchSectionData(unsigned char *pData, int dataLen, unsigned long RegistrationId)
{
    UNUSED_VAR(pData);
    UNUSED_VAR(dataLen);
    UNUSED_VAR(RegistrationId);

    return;
}

void rmf_SectionFilter_OOB::LockPidList(DsgTunnelSession* pDsgTunnelSession)
{
    UNUSED_VAR(pDsgTunnelSession);

    return;
}

void rmf_SectionFilter_OOB::UnLockPidList(DsgTunnelSession* pDsgTunnelSession)
{
    UNUSED_VAR(pDsgTunnelSession);

    return;
}

void rmf_SectionFilter_OOB::LocksfList(DsgTunnelSession* pDsgTunnelSession)
{
    UNUSED_VAR(pDsgTunnelSession);

    return;
}

void rmf_SectionFilter_OOB::UnLocksfList(DsgTunnelSession* pDsgTunnelSession)
{
    UNUSED_VAR(pDsgTunnelSession);

    return;
}

int rmf_SectionFilter_OOB::stOobCallbackfn(unsigned long             RegistrationId,
                            unsigned long             AppData,
                            VL_DSG_CLIENT_ID_ENC_TYPE eClientType,
                            unsigned long long        nClientId,
                            unsigned long             nBytes,
                            unsigned char*            pData)
{
    UNUSED_VAR(RegistrationId);
    UNUSED_VAR(AppData);
    UNUSED_VAR(eClientType);
    UNUSED_VAR(nClientId);
    UNUSED_VAR(nBytes);
    UNUSED_VAR(pData);

    return 0;
}

RMF_SECTFLT_RESULT rmf_SectionFilter_OOB::DSG_OpenTunnelFlow(Dsg_SectionFilter_Info_t *pDsgFilterInfo)
{
    UNUSED_VAR(pDsgFilterInfo);
    
    return RMF_SECTFLT_Success;
}

RMF_SECTFLT_RESULT rmf_SectionFilter_OOB::DSG_StartPidFilter(Dsg_SectionFilter_Info_t *pDsgFilterInfo, uint8_t pid)
{
    UNUSED_VAR(pDsgFilterInfo);
    UNUSED_VAR(pid);

    return RMF_SECTFLT_Success;
}

RMF_SECTFLT_RESULT rmf_SectionFilter_OOB::DSG_AddFilter(Dsg_SectionFilter_Info_t *pDsgFilterInfo, uint8_t* pos_mask, uint8_t* pos_value,    uint16_t pos_length,
                                        uint8_t* neg_mask, uint8_t* neg_value,  uint16_t neg_length)
{
    UNUSED_VAR(pDsgFilterInfo);
    UNUSED_VAR(pos_mask);
    UNUSED_VAR(pos_value);
    UNUSED_VAR(pos_length);
    UNUSED_VAR(neg_mask);
    UNUSED_VAR(neg_value);
    UNUSED_VAR(neg_length);

    return RMF_SECTFLT_Success;
}

RMF_SECTFLT_RESULT rmf_SectionFilter_OOB::DSG_RemoveFilter(Dsg_SectionFilter_Info_t *pDsgFilterInfo)
{
    UNUSED_VAR(pDsgFilterInfo);

    return RMF_SECTFLT_Success;

}

RMF_SECTFLT_RESULT rmf_SectionFilter_OOB::DSG_StopPidFilter(Dsg_SectionFilter_Info_t *pDsgFilterInfo)
{
    UNUSED_VAR(pDsgFilterInfo);
    
    return RMF_SECTFLT_Success;
}

RMF_SECTFLT_RESULT rmf_SectionFilter_OOB::DSG_CloseTunnelFlow(Dsg_SectionFilter_Info_t *pDsgFilterInfo)
{
    UNUSED_VAR(pDsgFilterInfo);

    return RMF_SECTFLT_Success;
}


RMF_SECTFLT_RESULT rmf_SectionFilter_OOB::Create(uint16_t pid, uint8_t* pos_mask, uint8_t* pos_value, uint16_t pos_length, uint8_t* neg_mask, uint8_t* neg_value, uint16_t neg_length, void **pSectionFilterInfo, uint32_t* pFilterID){
    UNUSED_VAR(pid);
    UNUSED_VAR(pos_mask);
    UNUSED_VAR(pos_value);
    UNUSED_VAR(pos_length);
    UNUSED_VAR(neg_mask);
    UNUSED_VAR(neg_value);
    UNUSED_VAR(neg_length);
    UNUSED_VAR(pSectionFilterInfo);
    UNUSED_VAR(pFilterID);

    return RMF_SECTFLT_Success;
}

RMF_SECTFLT_RESULT rmf_SectionFilter_OOB::Start(uint32_t filterID){
    UNUSED_VAR(filterID);

    return RMF_SECTFLT_Success;
}


RMF_SECTFLT_RESULT rmf_SectionFilter_OOB::Release(uint32_t filterID){
    UNUSED_VAR(filterID);

    return RMF_SECTFLT_Success;
}


RMF_SECTFLT_RESULT rmf_SectionFilter_OOB::Stop(uint32_t filterID){  
    UNUSED_VAR(filterID);

    return RMF_SECTFLT_Success;
}


RMF_SECTFLT_RESULT rmf_SectionFilter_OOB::Read(uint32_t filterID, rmf_Section_Data** ppSectionData){
    UNUSED_VAR(filterID);
    UNUSED_VAR(ppSectionData);

    return RMF_SECTFLT_Success;
}
