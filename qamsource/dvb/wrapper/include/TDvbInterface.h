/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2015 RDK Management
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
* ============================================================================
* Contributed by ARRIS Group, Inc.
* ============================================================================
*/


#ifndef TDVBINTERFACE_H
#define TDVBINTERFACE_H

#include "IDvbSectionParserObserver.h"
#include "IDvbStorageObserver.h"
#include "TDvbConfig.h"
#include "TDvbSiStorage.h"
#include "TSectionParser.h"
#include "TDvbStorageNamespace.h"
#include "TDvbTuner.h"
#include "rmf_osal_event.h"
#include "rmf_osal_thread.h"
#include "rmfcore.h"

#include <map>
#include <iterator>

class IDvbStorageObserver;
class IDvbSectionParserObserver;

class TDvbInterface : public IDvbSectionParserObserver,
                      public IDvbStorageObserver,
                      public IRMFMediaEvents
{
private:
  bool IsEventThreadAlive;
  bool IsTunerThreadAlive;
  bool IsScanThreadAlive;
  bool IsFastScan;
  TDvbConfig* DvbConfig;
  TDvbSiStorage* DvbSiStorage;
  TDvbTuner* DvbTuner;
  TSectionParser*  SectionParser;
  rmf_osal_eventqueue_handle_t EventQueueId;
  rmf_osal_eventqueue_handle_t TunerEventQueueId; 
  rmf_osal_ThreadId EventThreadId;
  rmf_osal_ThreadId TunerEventThreadId;
  rmf_osal_ThreadId ScanThreadId;
  rmf_osal_eventmanager_handle_t EventManagerHandle;
  rmf_osal_eventmanager_handle_t SiEventManagerHandle;
  rmf_osal_Mutex TunerRegistrationMutex;

  void RegiterToStorageNotifications();
  void UnRegiterToStorageNotifications();
  void InitSiStorage();
  static void EventMonitorThread(void*);
  static void TunerEventMonitorThread(void* arg);
  void InitializeDvbComponent();
  bool CreateSiTableSectionMessageQueue();
  bool CreateTunerEventQueue();
  bool CreateEventMonitorThread();
  bool CreateTunerEventThread();
  bool CreateScanThread();
  void HandleEvent(const TSiTable& tableData);
  rmf_osal_eventqueue_handle_t GetTunerEventQueueId();
  inline bool GetTunerThreadStatus() { return IsTunerThreadAlive;}
  void HandleTunerEvent(const uint32_t& event);
public:
  TDvbInterface();
  ~TDvbInterface();

  TDvbConfig* GetDvbConfiguration();
  rmf_osal_eventqueue_handle_t GetEventQueueId();
  std::vector<std::shared_ptr<TDvbStorageNamespace::InbandTableInfoStruct>> GetInbandTableInfo(std::string& profile);
  std::vector<std::shared_ptr<TDvbStorageNamespace::TStorageTransportStreamStruct>> GetTsListByNetId(uint16_t nId);
  std::vector<std::shared_ptr<TDvbStorageNamespace::ServiceStruct>> GetServiceListByTsId(uint16_t nId, uint16_t tsId);
  std::vector<std::shared_ptr<TDvbStorageNamespace::EventStruct>> GetEventListByServiceId(uint16_t nId, uint16_t tsId, uint16_t sId);
  TDvbStorageNamespace::TDvbScanStatus GetScanStatus();
  std::string GetProfiles();
  bool SetProfiles(std::string& profiles);
  bool SetEventHandlerInstance(const rmf_osal_eventmanager_handle_t& eventHandler);
  //bool SetEventHandlerInstance(rmf_osal_eventmanager_handle_t eventHandler);
  rmf_ModulationMode GetMappedModulation(const TDvbStorageNamespace::TModulationMode& mod);
  void ParseSiData(uint8_t *data, uint32_t size);
  void RegiterToSectionParserNotifications(TSectionParser* parser);
  void UnRegiterToSectionParserNotifications();
 
  // IDvbStorageObserver
  void Tune(const uint32_t& freq, const TDvbStorageNamespace::TModulationMode& mod, const uint32_t& symbol);
  void UnTune();

  // IDvbSectionParserObserver
  void SendEvent(uint32_t eventType, void *eventData, size_t dataSize);

  // IRMFMediaEvents Methods
  virtual void status(const RMFStreamingStatus& status);
  virtual void error(RMFResult err, const char* msg);
};

#endif  // TDVBINTERFACE_H
