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

#include "TDvbInterface.h"
#include "TDvbHelper.h"
#include "rdk_debug.h"
#include "rmfbase.h"
#include "rmfqamsrc.h"
#include "rmf_qamsrc_common.h"

using namespace TDvbStorageNamespace;

// private method implementations.

void FreeEventData(void *eventData)
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> \n", __FUNCTION__);
  if (!eventData) {
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> event data is null\n", __FUNCTION__);
    return;
  }
#ifdef DVB_SECTION_OUTPUT
    RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.DVBINTERFACE", "<%s> event data = %p\n", __FUNCTION__, eventData);
    delete eventData;
#else
  // event data is supposed to point to an SiTable object
  TSiTable* tbl = reinterpret_cast<TSiTable*>(eventData);
  RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.DVBINTERFACE", "<%s> table_id = 0x%x\n", __FUNCTION__, tbl->GetTableId());
  delete tbl;
  RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.DVBINTERFACE", "<%s> tabled freed\n", __FUNCTION__);
  tbl = NULL;
#endif
}

void TDvbInterface::RegiterToStorageNotifications()
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> \n", __FUNCTION__);
  DvbSiStorage->RegisterDvbStorageObserver(this);
}

void TDvbInterface::UnRegiterToStorageNotifications()
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> \n", __FUNCTION__);
  DvbSiStorage->RemoveDvbStorageObserver(this);
}

void TDvbInterface::RegiterToSectionParserNotifications(TSectionParser* parser)
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> \n", __FUNCTION__);
  if (parser != NULL) {
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> SectionParser is set good to go\n", __FUNCTION__);
    SectionParser = parser;
    SectionParser->RegisterDvbSectionParserObserver(this);
  }
}

void TDvbInterface::UnRegiterToSectionParserNotifications()
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> \n", __FUNCTION__);
  if (SectionParser != NULL) {
    SectionParser->RemoveDvbSectionParserObserver(this);
  }
}

void TDvbInterface::InitializeDvbComponent()
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> \n", __FUNCTION__);
  if (CreateSiTableSectionMessageQueue()) {
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.DVBINTERFACE", "<%s> - event queue created successfuly.\n", __FUNCTION__);
    
    TFileStatus status = DvbSiStorage->CreateDatabase();
    if (status != TFileStatus::FILE_STATUS_OPENED && status != TFileStatus::FILE_STATUS_CREATED) {
      RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> - Unable to open db\n", __FUNCTION__);
    }
    else {
      RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.DVBINTERFACE", "<%s> - Db file %s created successfuly.\n",
        __FUNCTION__, DvbConfig->GetDatabaseFilePath().c_str());
      if (CreateEventMonitorThread()) {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> CreateEventMonitorThread successful\n", __FUNCTION__);
        RegiterToStorageNotifications();
        // DvbDb::open() calls sanityCheck() that in turn calls contentCheck(),
        // so we don't have to make the sanity and content checks here.
        // Let's skip the fast scan only if status == OPENED
        if (status == TFileStatus::FILE_STATUS_OPENED) {
          IsFastScan = true;
        }
        if (CreateTunerEventQueue()) {
          RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.DVBINTERFACE", "<%s> - CreateTunerEventQueue Success .\n", __FUNCTION__);
          if (CreateTunerEventThread()) {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.DVBINTERFACE", "<%s> - CreateTunerEventThread Success .\n", __FUNCTION__);
            DvbSiStorage->UpdateScanType(true);
            if (CreateScanThread()) {
              RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.DVBINTERFACE", "<%s> - CreateScanThread Success .\n", __FUNCTION__);
            }
            else {
              RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> - CreateScanThread Failed! Aborting.\n", __FUNCTION__);
            }
          }
          else {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> - CreateTunerEventThread Failed! Aborting.\n", __FUNCTION__);
            //std::abort();
          }
        }
      }
      else {
        // Thread creation failed. Abort!
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> - CreateEventMonitorThread Failed! Aborting.\n", __FUNCTION__);
        //std::abort();
      }
    }
  }
  else {
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.DVBINTERFACE", "<%s> - event queue creation filed!.\n", __FUNCTION__);
    //std::abort();
  }
}

bool TDvbInterface::CreateTunerEventThread()
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> \n", __FUNCTION__);
  if (!IsTunerThreadAlive) {
    if (RMF_SUCCESS != rmf_osal_threadCreate(&TDvbInterface::TunerEventMonitorThread, (void*)this, RMF_OSAL_THREAD_PRIOR_DFLT,
        RMF_OSAL_THREAD_STACK_SIZE, &TunerEventThreadId, "DvbTuner_Thread")) {
      RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> - unable to create Event monitor thread \n", __FUNCTION__);
    }
    else {
      RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> CreateTunerEventThread  Done\n", __FUNCTION__);
      IsTunerThreadAlive = true;
      // Register for the tuner events.
      if (RMF_SUCCESS != rmf_osal_eventmanager_create(&EventManagerHandle)) {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> - unable to create event manager\n",
           __FUNCTION__);
      }
      RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> rmf_osal_eventmanager_create for EventManagerHandle  Done\n", __FUNCTION__);
      rmf_osal_mutexAcquire(TunerRegistrationMutex);
      if (RMF_SUCCESS != rmf_osal_eventmanager_register_handler(EventManagerHandle, TunerEventQueueId, RMF_OSAL_EVENT_CATEGORY_TUNER)) {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> - unable to do Register EventManager for RMF_OSAL_EVENT_CATEGORY_TUNER \n", __FUNCTION__);
        // std::abort();
      }
      RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> rmf_osal_eventmanager_register_handler for EventManagerHandle  Done\n", __FUNCTION__);
      rmf_osal_mutexRelease(TunerRegistrationMutex);

      RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> CreateTunerEventThread  Done\n", __FUNCTION__);
      IsTunerThreadAlive = true;
    }
  }
  else  {
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> -Tuner Event monitor thread already exists! \n", __FUNCTION__);
  }
  return IsTunerThreadAlive;
}

bool TDvbInterface::CreateTunerEventQueue()
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> \n", __FUNCTION__);
  if (RMF_SUCCESS != rmf_osal_eventqueue_create ((const uint8_t* )"DvbTunerEventQueue", &TunerEventQueueId)) {
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> - unable to create event queue\n", __FUNCTION__);
    return false;
  }
  RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.DVBINTERFACE", "<%s> - event queue created\n", __FUNCTION__);
  return true;
}

void TDvbInterface::HandleTunerEvent(const uint32_t& event)
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> \n", __FUNCTION__);
  bool process(false);
  switch (event) {
  case RMF_QAMSRC_EVENT_TUNE_SYNC:
    RDK_LOG(RDK_LOG_INFO,"LOG.RDK.DVBINTERFACE", "RMF_QAMSRC_EVENT_TUNE_SYNC\n");
    process = true;
    break;
  case RMF_QAMSRC_EVENT_PAT_ACQUIRED:
    RDK_LOG(RDK_LOG_INFO,"LOG.RDK.DVBINTERFACE", "RMF_QAMSRC_EVENT_PAT_ACQUIRED\n");
    process = true;
    break;
  case RMF_QAMSRC_EVENT_PMT_ACQUIRED:
    RDK_LOG(RDK_LOG_INFO,"LOG.RDK.DVBINTERFACE", "RMF_QAMSRC_EVENT_PMT_ACQUIRED\n");
    process = true;
    break;
  case RMF_QAMSRC_EVENT_STREAMING_STARTED:
    RDK_LOG(RDK_LOG_INFO,"LOG.RDK.DVBINTERFACE", "RMF_QAMSRC_EVENT_STREAMING_STARTED\n");
    process = true;
    break;
  default:
    RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.DVBINTERFACE", "Unknown event from Tuner\n");
    break;
  }
  DvbSiStorage->UpdateTuneStatus(process);
}

void TDvbInterface::TunerEventMonitorThread(void* arg)
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> \n", __FUNCTION__);
  TDvbInterface* tunerInstance = static_cast<TDvbInterface*>(arg);
  rmf_osal_event_handle_t eventHandle;
  rmf_osal_event_params_t eventParams;
  uint32_t eventType;
  rmf_Error err;
 
  while (tunerInstance->GetTunerThreadStatus()) {
    err = rmf_osal_eventqueue_get_next_event(tunerInstance->GetTunerEventQueueId(), &eventHandle, NULL, &eventType, &eventParams);
    if (err != RMF_SUCCESS) {
      RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE",
      "<%s:> - unable to get event,... terminating thread. err = 0x%x\n", __FUNCTION__, err);
      break;
    }
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> Got some events \n", __FUNCTION__);
    tunerInstance->HandleTunerEvent(eventType);
    rmf_osal_event_delete(eventHandle);
  }
}

rmf_osal_eventqueue_handle_t TDvbInterface::GetTunerEventQueueId()
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> \n", __FUNCTION__);
  return TunerEventQueueId;
}

void TDvbInterface::EventMonitorThread(void* arg)
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> \n", __FUNCTION__);
  TDvbInterface* dvbInterface = static_cast<TDvbInterface*>(arg);
  rmf_osal_event_handle_t eventHandle;
  rmf_osal_event_params_t eventParams;
  uint32_t eventType;
  rmf_Error err;

  while(true) {
    err = rmf_osal_eventqueue_get_next_event(dvbInterface->GetEventQueueId(), &eventHandle, NULL, &eventType, &eventParams);
    if (err != RMF_SUCCESS) {
      RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE",
      "<%s:> - unable to get event,... terminating thread. err = 0x%x\n", __FUNCTION__, err);
      break;
    }
    // DvbSiStorage does not support raw sections as an input
    TSiTable *tbl = static_cast<TSiTable*>(eventParams.data);
    dvbInterface->HandleEvent(*tbl);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.DVBINTERFACE", "<%s> Received table event, id = 0x%x\n", __FUNCTION__, tbl->GetTableId());
    rmf_osal_event_delete(eventHandle);
  }
}

void TDvbInterface::HandleEvent(const TSiTable& tableData)
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> \n", __FUNCTION__);
  DvbSiStorage->HandleTableEvent(tableData);
}

// public method implementations.
TDvbInterface::TDvbInterface() 
{
  DvbConfig = new TDvbConfig();
  DvbSiStorage = new TDvbSiStorage(
                 DvbConfig->GetHomeTsFrequency(),
                 static_cast<TModulationMode>(DvbConfig->GetHomeTsModulation()),
                 DvbConfig->GetHomeTsSymolRate(),
                 DvbConfig->GetPreferredNetworkId(),
                 DvbConfig->GetDatabaseFilePath(),
                 DvbConfig->GetNetworkConfigFilePath());
    DvbTuner = new TDvbTuner();
    SectionParser = NULL;
    IsEventThreadAlive = false;
    IsTunerThreadAlive = false;
    IsScanThreadAlive = false;
    IsFastScan = DvbConfig->GetFastScanStatus();
    EventQueueId = 0;
    TunerEventQueueId = 0; 
    EventThreadId = 0; 
    TunerEventThreadId = 0; 
    ScanThreadId = 0; 
    EventManagerHandle = 0; 
    SiEventManagerHandle = 0; 
  DvbSiStorage->SetBarkerInfo(
    DvbConfig->GetBarkerFrequencey(),
    static_cast<TModulationMode>(DvbConfig->GetBarkerModulation()),
    DvbConfig->GetBarkerSymbolRate());
  InitializeDvbComponent();
}

TDvbInterface::~TDvbInterface()
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> Destructor \n", __FUNCTION__);
  IsTunerThreadAlive = false;
  IsEventThreadAlive = false;
  delete DvbConfig;
  delete DvbSiStorage;
  delete DvbTuner;
  rmf_osal_threadDestroy(EventThreadId);
  rmf_osal_threadDestroy(TunerEventThreadId);
  rmf_osal_threadDestroy(ScanThreadId);
  rmf_osal_eventqueue_delete(EventQueueId);
 // Unrigister tuner events before deleting it 
  rmf_osal_mutexAcquire(TunerRegistrationMutex);
  rmf_osal_eventmanager_unregister_handler(EventManagerHandle, TunerEventQueueId);
  rmf_osal_mutexRelease(TunerRegistrationMutex);

  rmf_osal_eventqueue_delete(TunerEventQueueId);
  rmf_osal_eventmanager_delete(EventManagerHandle);
}
  
TDvbConfig* TDvbInterface::GetDvbConfiguration()
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> \n", __FUNCTION__);
  return DvbConfig;
}
 
bool TDvbInterface::CreateSiTableSectionMessageQueue()
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> \n", __FUNCTION__);
  // Consider sending a randomly generated string instead of hard coded "DvbSiQueue".
  // This may cause problems if many instances of TDvbInterface class is instantiated in threaded environment
  // With static functions in place.
  if (RMF_SUCCESS != rmf_osal_eventqueue_create ((const uint8_t* )"DvbSiQueue", &EventQueueId)) {
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> - unable to create event queue\n", __FUNCTION__);
    return false;
  }
  RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.DVBINTERFACE", "<%s> - event queue created\n", __FUNCTION__);
  return true;
}

bool TDvbInterface::CreateScanThread()
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> \n", __FUNCTION__);
  if (!IsScanThreadAlive) {
    if (RMF_SUCCESS != rmf_osal_threadCreate(&TDvbSiStorage::ScanThreadInit, (void*)DvbSiStorage, RMF_OSAL_THREAD_PRIOR_DFLT,
      RMF_OSAL_THREAD_STACK_SIZE, &ScanThreadId, "DvbScan_Thread")) {
      RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> - unable to create Event monitor thread \n", __FUNCTION__);
    }
    else {
      RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> CreateScanThread Done \n", __FUNCTION__);
      IsScanThreadAlive = true;
    }
  }
  else  {
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> - Scan thread already exists! \n", __FUNCTION__);
  }
  return IsScanThreadAlive;
}

bool TDvbInterface::CreateEventMonitorThread()
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> \n", __FUNCTION__);
  if (!IsEventThreadAlive) {
    if (RMF_SUCCESS != rmf_osal_threadCreate(&TDvbInterface::EventMonitorThread, (void*)this, RMF_OSAL_THREAD_PRIOR_DFLT,
        RMF_OSAL_THREAD_STACK_SIZE, &EventThreadId, "DvbSi_Thread")) {
      RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> - unable to create Event monitor thread \n", __FUNCTION__);
    }
    else {
      RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> - create Event monitor thread Done.\n", __FUNCTION__);
      IsEventThreadAlive = true;
    }
  }
  else  {
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> - Event monitor thread already exists! \n", __FUNCTION__);
  }
  return IsEventThreadAlive;
}

rmf_osal_eventqueue_handle_t TDvbInterface::GetEventQueueId()
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> \n", __FUNCTION__);
  return EventQueueId;
}

void TDvbInterface::Tune(const uint32_t& freq, const TModulationMode& mod, const uint32_t& symbol)
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> \n", __FUNCTION__);
  DvbSiStorage->UpdateTuneStatus(false);
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> freq = %d mod = %d symbol = %d \n", __FUNCTION__, freq, mod, symbol);
  DvbTuner->Tune(freq, static_cast<TModulationMode>(16), symbol);
}

void TDvbInterface::UnTune()
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> \n", __FUNCTION__);
  DvbTuner->UnTune();
}

// IRMFMediaEvents implementations
void TDvbInterface::error(RMFResult err, const char* msg)
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "%s:%d: KSS error() Error 0x%x\n", __FUNCTION__, __LINE__, err);
}

void TDvbInterface::status(const RMFStreamingStatus& status)
{
  RDK_LOG(RDK_LOG_INFO, "LOG.RDK.DVBINTERFACE", "%s:%d: KSS status() Status 0x%x\n", __FUNCTION__, __LINE__, status.m_status);
}

std::vector<std::shared_ptr<TDvbStorageNamespace::InbandTableInfoStruct>> TDvbInterface::GetInbandTableInfo(std::string& profile)
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> \n", __FUNCTION__);
  return DvbSiStorage->GetInbandTableInfo(profile);
}

std::vector<std::shared_ptr<TDvbStorageNamespace::TStorageTransportStreamStruct>> TDvbInterface::GetTsListByNetId(uint16_t nId)
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> \n", __FUNCTION__);
  return DvbSiStorage->GetTsListByNetId(nId);
}

std::vector<std::shared_ptr<TDvbStorageNamespace::EventStruct>> TDvbInterface::GetEventListByServiceId(uint16_t nId, uint16_t tsId, uint16_t sId)
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> \n", __FUNCTION__);
  return DvbSiStorage->GetEventListByServiceId(nId, tsId, sId);
}

std::vector<std::shared_ptr<TDvbStorageNamespace::ServiceStruct>> TDvbInterface::GetServiceListByTsId(uint16_t nId, uint16_t tsId)
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> \n", __FUNCTION__);
  return DvbSiStorage->GetServiceListByTsId(nId, tsId);
}

TDvbStorageNamespace::TDvbScanStatus TDvbInterface::GetScanStatus()
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> \n", __FUNCTION__);
  return DvbSiStorage->GetScanStatus();
}

std::string TDvbInterface::GetProfiles()
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> \n", __FUNCTION__);
  return DvbSiStorage->GetProfiles();
}

bool TDvbInterface::SetProfiles(std::string& profiles)
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> \n", __FUNCTION__);
  return DvbSiStorage->SetProfiles(profiles);
}

// IDvbSectionParserObserver
void TDvbInterface::SendEvent(uint32_t eventType, void *eventData, size_t dataSize)
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> \n", __FUNCTION__);
  rmf_Error ret;
  rmf_osal_event_handle_t eventHandle;
  rmf_osal_event_params_t eventParams;

  // Let's set the event parameters
  eventParams.data = eventData;
  eventParams.data_extension = dataSize;
  eventParams.free_payload_fn = FreeEventData;

  // Time to create the event itself
  ret = rmf_osal_event_create(RMF_OSAL_EVENT_CATEGORY_INB_FILTER, eventType, &eventParams, &eventHandle);
  if (RMF_SUCCESS != ret) {
    RDK_LOG(RDK_LOG_ERROR,  "<%s:%d> Event creation failed\n", __FUNCTION__, __LINE__);
    return;
  }
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> Done with create event about send.\n", __FUNCTION__);
  // Sending
  ret = rmf_osal_eventmanager_dispatch_event(SiEventManagerHandle, eventHandle);
  if (RMF_SUCCESS != ret) {
    RDK_LOG(RDK_LOG_ERROR,  "<%s:%d> Event dispatch failed\n", __FUNCTION__, __LINE__);
    // clean up
    rmf_osal_event_delete(eventHandle);
  }
}
 
bool TDvbInterface::SetEventHandlerInstance(const rmf_osal_eventmanager_handle_t& eventHandler)
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s>  eventHandler = %#X \n", __FUNCTION__, eventHandler);
  SiEventManagerHandle = eventHandler;
  return true;
}
  
rmf_ModulationMode TDvbInterface::GetMappedModulation(const TDvbStorageNamespace::TModulationMode& mod)
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> \n", __FUNCTION__);
  return DvbTuner->GetMappedModulation(mod);
}

void TDvbInterface::ParseSiData(uint8_t *data, uint32_t size)
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> \n", __FUNCTION__);
  if (SectionParser != NULL) {
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBINTERFACE", "<%s> SectionParser is valid go ahead and parse\n", __FUNCTION__);
    SectionParser->ParseSiData(data, size);
  }
}
